"""Generate conservative, reviewable source variants from the Clang AST.

Adapted from Gruntz's syntactic variant generator. The result is a normal
``homm3 vc6 hypotheses`` manifest; this command never edits authored source.
"""
from __future__ import annotations

import itertools
import json
import re
from dataclasses import dataclass
from pathlib import Path

import clang.cindex as ci

from homm3.core import common
from homm3.match import status
from homm3.vc6 import tu_state_sweep, tu_state_variants
from homm3.vc6._unit import source_for_unit


FAMILIES = ("commutative_order", "relational_order", "terminal_return_order",
            "declaration_split", "declaration_merge", "identifier_rename")
COMMUTATIVE = {"+", "*", "==", "!=", "&", "|", "^"}
RELATIONAL = {"<": ">", ">": "<", "<=": ">=", ">=": "<="}
INTEGRAL = {getattr(ci.TypeKind, name) for name in (
    "BOOL", "CHAR_U", "UCHAR", "USHORT", "UINT", "ULONG", "CHAR_S",
    "SCHAR", "SHORT", "INT", "LONG", "LONGLONG", "ULONGLONG")
    if hasattr(ci.TypeKind, name)}
SENSITIVE = (b"__COUNTER__", b"__FUNCTION__", b"__FUNCSIG__", b"__LINE__")


@dataclass(frozen=True)
class Edit:
    start: int
    end: int
    replacement: bytes


@dataclass(frozen=True)
class Mutation:
    family: str
    name: str
    edits: tuple[Edit, ...]


def _span(cursor):
    return cursor.extent.start.offset, cursor.extent.end.offset


def _side_effect(cursor):
    calls = {ci.CursorKind.CALL_EXPR, ci.CursorKind.CXX_NEW_EXPR,
             ci.CursorKind.CXX_DELETE_EXPR}
    for node in cursor.walk_preorder():
        if node.kind in calls:
            return True
        if node.kind == ci.CursorKind.UNARY_OPERATOR and any(
                token.spelling in ("++", "--") for token in node.get_tokens()):
            return True
        if node.kind == ci.CursorKind.BINARY_OPERATOR and any(
                token.spelling in ("=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=")
                for token in node.get_tokens()):
            return True
    return False


def _value_type(type_):
    kind = type_.get_canonical().kind
    return not type_.is_volatile_qualified() and kind in (INTEGRAL | {ci.TypeKind.ENUM,
                                                                       ci.TypeKind.POINTER})


def _binary_mutations(fn, source: bytes):
    found = []
    for node in fn.walk_preorder():
        if node.kind != ci.CursorKind.BINARY_OPERATOR:
            continue
        children = list(node.get_children())
        if len(children) != 2 or any(_side_effect(child) for child in children):
            continue
        left, right = children
        a, b = _span(left)
        c, d = _span(right)
        if not a < b <= c < d or not all(_value_type(x.type) for x in children):
            continue
        tokens = [token for token in node.get_tokens()
                  if b <= token.extent.start.offset and token.extent.end.offset <= c
                  and token.spelling in COMMUTATIVE | set(RELATIONAL)]
        if len(tokens) != 1 or any(s in source[a:d] for s in SENSITIVE):
            continue
        operator = tokens[0].spelling
        lkind = left.type.get_canonical().kind
        rkind = right.type.get_canonical().kind
        if operator in ("&", "|", "^", "*") and not (lkind in INTEGRAL | {ci.TypeKind.ENUM}
                                                       and rkind in INTEGRAL | {ci.TypeKind.ENUM}):
            continue
        if operator == "+" and not (
                (lkind in INTEGRAL | {ci.TypeKind.ENUM} and rkind in INTEGRAL | {ci.TypeKind.ENUM})
                or (lkind == ci.TypeKind.POINTER and rkind in INTEGRAL)
                or (rkind == ci.TypeKind.POINTER and lkind in INTEGRAL)):
            continue
        middle = bytearray(source[b:c])
        if operator in RELATIONAL:
            token = tokens[0]
            middle[token.extent.start.offset - b:token.extent.end.offset - b] = \
                RELATIONAL[operator].encode()
            family = "relational_order"
        else:
            family = "commutative_order"
        if source[a:b] == source[c:d] and family == "commutative_order":
            continue
        replacement = source[c:d] + bytes(middle) + source[a:b]
        found.append(Mutation(family, f"line-{source.count(b'\n', 0, a) + 1}-{operator}",
                              (Edit(a, d, replacement),)))
    return found


def _return_mutations(fn, source: bytes):
    found = []
    for compound in fn.walk_preorder():
        if compound.kind != ci.CursorKind.COMPOUND_STMT:
            continue
        statements = list(compound.get_children())
        if len(statements) < 2:
            continue
        guard, fallback = statements[-2:]
        if guard.kind != ci.CursorKind.IF_STMT or fallback.kind != ci.CursorKind.RETURN_STMT:
            continue
        children = list(guard.get_children())
        if len(children) != 2:
            continue
        condition, arm = children
        if arm.kind == ci.CursorKind.COMPOUND_STMT:
            arm_children = list(arm.get_children())
            arm = arm_children[0] if len(arm_children) == 1 else arm
        if arm.kind != ci.CursorKind.RETURN_STMT or not _value_type(condition.type):
            continue
        a, b = _span(condition)
        c, d = _span(arm)
        e, f = _span(fallback)
        if not a < b <= c < d <= e < f or any(s in source[a:f] for s in SENSITIVE):
            continue
        if source[c:d] == source[e:f]:
            continue
        found.append(Mutation("terminal_return_order", f"line-{source.count(b'\n', 0, a) + 1}",
                              (Edit(a, b, b"!(" + source[a:b] + b")"),
                               Edit(c, d, source[e:f]), Edit(e, f, source[c:d]))))
    return found


def _declaration_mutations(fn, source: bytes):
    found = []
    for compound in fn.walk_preorder():
        if compound.kind != ci.CursorKind.COMPOUND_STMT:
            continue
        statements = list(compound.get_children())
        for statement in statements:
            if statement.kind != ci.CursorKind.DECL_STMT:
                continue
            variables = [v for v in statement.get_children() if v.kind == ci.CursorKind.VAR_DECL]
            if len(variables) < 2:
                continue
            start, end = _span(statement)
            prefix = source[start:variables[0].location.offset]
            if any(token in prefix for token in (b"*", b"&", b",")) or \
                    any(s in source[start:end] for s in SENSITIVE):
                continue
            if len({v.type.spelling for v in variables}) != 1 or sum(
                    t.spelling == "," for t in statement.get_tokens()) != len(variables) - 1:
                continue
            declarators = [source[v.location.offset:v.extent.end.offset] for v in variables]
            if not all(declarators):
                continue
            indent = source[source.rfind(b"\n", 0, start) + 1:start]
            replacement = (b";\n" + indent).join(prefix + d for d in declarators) + b";"
            found.append(Mutation("declaration_split", f"line-{source.count(b'\n', 0, start) + 1}",
                                  (Edit(start, end, replacement),)))
        for first, second in zip(statements, statements[1:]):
            if first.kind != ci.CursorKind.DECL_STMT or second.kind != ci.CursorKind.DECL_STMT:
                continue
            a, b = _span(first)
            c, d = _span(second)
            if not a < b <= c < d or source[b:c].strip():
                continue
            variables = [tuple(v for v in stmt.get_children() if v.kind == ci.CursorKind.VAR_DECL)
                         for stmt in (first, second)]
            if any(len(v) != 1 for v in variables):
                continue
            left, right = variables[0][0], variables[1][0]
            prefix = source[a:left.location.offset]
            if (prefix != source[c:right.location.offset]
                    or any(token in prefix for token in (b"*", b"&", b","))
                    or left.type.spelling != right.type.spelling
                    or any(s in source[a:d] for s in SENSITIVE)):
                continue
            replacement = (prefix + source[left.location.offset:left.extent.end.offset]
                           + b", " + source[right.location.offset:right.extent.end.offset] + b";")
            found.append(Mutation("declaration_merge", f"line-{source.count(b'\n', 0, a) + 1}",
                                  (Edit(a, d, replacement),)))
    return found


def _rename_mutations(fn, source: bytes):
    found = []
    nodes = list(fn.walk_preorder())
    existing = {node.spelling for node in nodes if node.kind in
                (ci.CursorKind.VAR_DECL, ci.CursorKind.PARM_DECL)}
    for decl in nodes:
        if decl.kind != ci.CursorKind.VAR_DECL or not decl.spelling or \
                decl.semantic_parent != fn:
            continue
        name = decl.spelling
        replacement = name + "Value"
        if replacement in existing:
            continue
        offsets = [decl.location.offset]
        offsets.extend(node.location.offset for node in nodes
                       if node.kind == ci.CursorKind.DECL_REF_EXPR
                       and node.referenced is not None and node.referenced.hash == decl.hash)
        encoded = name.encode()
        if not offsets or any(source[pos:pos + len(encoded)] != encoded for pos in offsets):
            continue
        found.append(Mutation("identifier_rename", f"{name}-to-{replacement}",
                              tuple(Edit(pos, pos + len(encoded), replacement.encode())
                                    for pos in sorted(set(offsets)))))
    return found


def _compilation_args(source: Path):
    commands = json.loads((common.HOMM3_DIR / "compile_commands.json").read_text())
    entry = next((row for row in commands if Path(row["file"]).resolve() == source.resolve()), None)
    if entry is None:
        raise ValueError(f"{source}: no compile_commands.json entry; run homm3 build")
    args = entry["arguments"]
    return [arg for arg in args[1:] if arg not in ("/c", str(source))]


def _target(tu, source: Path, va: int):
    marker = f"VA(0x{va:08x},".encode()
    blob = source.read_bytes()
    location = blob.find(marker)
    if location < 0 or blob.count(marker) != 1:
        raise ValueError(f"{source}: unique VA marker {marker.decode()} required")
    nexts = [point for point in (blob.find(b"\nVA(", location + 1),
                                blob.find(b"\nDATA(", location + 1)) if point >= 0]
    end = min(nexts) if nexts else len(blob)
    matches = [node for node in tu.cursor.walk_preorder()
               if node.kind in (ci.CursorKind.FUNCTION_DECL, ci.CursorKind.CXX_METHOD)
               and node.is_definition() and node.location.file
               and Path(str(node.location.file)).resolve() == source.resolve()
               and location < node.location.offset < end]
    if not matches:
        raise ValueError(f"{source}: no parsed definition after VA marker {va:#x}")
    return min(matches, key=lambda node: node.extent.start.offset), blob


def _render(source: bytes, edits: tuple[Edit, ...]):
    result = source
    for edit in sorted(edits, key=lambda item: item.start, reverse=True):
        result = result[:edit.start] + edit.replacement + result[edit.end:]
    return result


def _compatible(mutations):
    edits = sorted((edit for mutation in mutations for edit in mutation.edits),
                   key=lambda edit: edit.start)
    return all(left.end <= right.start for left, right in zip(edits, edits[1:]))


def manifest(unit: str, symbol: str, va: int, source: Path, *, families: tuple[str, ...],
             depth: int, limit: int, state_trials: int, state_family: str,
             state_insertion: str, seed: int, allow_external_errors: bool = False):
    if depth < 0 or limit < 1 or state_trials < 0:
        raise ValueError("depth, limit, and state trials must be nonnegative")
    unknown = set(families) - set(FAMILIES)
    if unknown:
        raise ValueError("unknown AST families: " + ", ".join(sorted(unknown)))
    tu = ci.Index.create().parse(str(source), args=_compilation_args(source),
                                 options=ci.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    fn, blob = _target(tu, source, va)
    errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
    target_errors = [str(d) for d in tu.diagnostics if
                     d.severity >= ci.Diagnostic.Error and d.location.file
                     and Path(str(d.location.file)).resolve() == source.resolve()
                     and fn.extent.start.offset <= d.location.offset <= fn.extent.end.offset]
    if target_errors or (errors and not allow_external_errors):
        raise ValueError("Clang parse errors: " + "\n".join((target_errors or errors)[:12]))
    first, last = _span(fn)
    # The VA attribute macro is part of Clang's function extent. Keep the
    # declaration anchor outside the editable function body.
    first = max(first, blob.index(b"\n", blob.index(f"VA(0x{va:08x},".encode())) + 1)
    source_fn = blob[first:last]
    mutations = [*_binary_mutations(fn, blob), *_return_mutations(fn, blob),
                 *_declaration_mutations(fn, blob), *_rename_mutations(fn, blob)]
    mutations = [m for m in mutations if m.family in families]
    shapes = [("baseline", source_fn)]
    for size in range(1, depth + 1):
        for choice in itertools.combinations(mutations, size):
            if not _compatible(choice):
                continue
            local = tuple(Edit(e.start - first, e.end - first, e.replacement)
                          for m in choice for e in m.edits)
            if any(e.start < 0 or e.end > len(source_fn) for e in local):
                continue
            name = "+".join(f"{m.family}:{m.name}" for m in choice)
            shapes.append((name, _render(source_fn, local)))
            if len(shapes) >= limit:
                break
        if len(shapes) >= limit:
            break
    unique = {}
    for name, value in shapes:
        unique.setdefault(value, name)
    source_options = [{"name": name, "replace": value.decode("utf-8")}
                      for value, name in unique.items()]
    source_options.sort(key=lambda option: 0 if option["name"] == "baseline" else 1)
    axes = [{"name": "ast-shape", "find": source_fn.decode("utf-8"),
             "options": source_options}]
    if state_trials:
        generated = tu_state_variants.make_variants(state_trials,
                                                    (state_family,), seed)
        if state_insertion == "top":
            anchor = blob.splitlines(keepends=True)[0]
            offset = len(anchor)
        else:
            marker = f"VA(0x{va:08x},".encode()
            marker_offset = blob.index(marker)
            offset = blob.rfind(b"\n", 0, marker_offset) + 1
            anchor = blob[offset:blob.index(b"\n", marker_offset) + 1]
        if not anchor or blob.count(anchor) != 1:
            raise ValueError("state insertion anchor is not unique")
        line = blob.count(b"\n", 0, offset) + 1
        state_options = [{"name": "baseline"}]
        state_options.extend({"name": f"{item.family}-{item.trial}",
                              "replace": (anchor.decode("utf-8") + item.body
                                          + f"#line {line + 1}\n")
                              if state_insertion == "top" else
                              (item.body + f"#line {line}\n"
                               + anchor.decode("utf-8"))}
                             for item in generated)
        axes.append({"name": "tu-state", "find": anchor.decode("utf-8"),
                     "options": state_options})
    return {"schema": 1, "unit": unit, "function": symbol, "axes": axes,
            "generator": {"name": "homm3-vc6-ast-variants", "va": va,
                          "families": families, "depth": depth,
                          "external_diagnostics": errors if allow_external_errors else []}}


def manifest_tu(unit: str, symbol: str, source: Path, *, families: tuple[str, ...],
                depth: int, limit: int, allow_external_errors: bool = False):
    """Generate one-source-edit-at-a-time options across the translation unit."""
    if depth < 0 or limit < 1 or set(families) - set(FAMILIES):
        raise ValueError("invalid AST family, depth, or limit")
    tu = ci.Index.create().parse(str(source), args=_compilation_args(source),
                                 options=ci.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
    if errors and not allow_external_errors:
        raise ValueError("Clang parse errors: " + "\n".join(errors[:12]))
    blob = source.read_bytes()
    anchor = blob.splitlines(keepends=True)[0]
    if not anchor or blob.count(anchor) != 1:
        raise ValueError("TU source anchor is not unique")
    options = [{"name": "baseline"}]
    rows = status.load_baseline()
    priorities = {row.rva + common.IMAGE_BASE: row.hist - row.max
                  for (name, _), row in rows.items() if name == unit
                  and row.rva is not None and row.cur is not None}
    markers = [int(match.group(1), 16) for match in
               re.finditer(rb"(?m)^VA\(0x([0-9a-fA-F]{8}),", blob)]
    markers.sort(key=lambda va: (-priorities.get(va, 0), va))
    selected = []
    per_function = max(1, (limit - 1) // min(4, max(1, len(markers))))
    for va in markers:
        try:
            fn, _ = _target(tu, source, va)
        except ValueError:
            continue
        first, last = _span(fn)
        first = max(first, blob.index(b"\n", blob.index(f"VA(0x{va:08x},".encode())) + 1)
        original = blob[first:last]
        if not original or blob.count(original) != 1:
            continue
        if any(d.severity >= ci.Diagnostic.Error and d.location.file
               and Path(str(d.location.file)).resolve() == source.resolve()
               and first <= d.location.offset <= last for d in tu.diagnostics):
            continue
        mutations = [*_binary_mutations(fn, blob), *_return_mutations(fn, blob),
                     *_declaration_mutations(fn, blob), *_rename_mutations(fn, blob)]
        mutations = [m for m in mutations if m.family in families]
        added_here = 0
        for size in range(1, depth + 1):
            for choice in itertools.combinations(mutations, size):
                if not _compatible(choice):
                    continue
                edits = tuple(Edit(e.start - first, e.end - first, e.replacement)
                              for m in choice for e in m.edits)
                if any(e.start < 0 or e.end > len(original) for e in edits):
                    continue
                replacement = _render(original, edits)
                if replacement == original:
                    continue
                label = "+".join(f"{m.family}:{m.name}" for m in choice)
                options.append({"name": f"{va:08x}:{label}:{len(options)}",
                                "extra_edits": [{"find": original.decode("utf-8"),
                                                 "replace": replacement.decode("utf-8")}]})
                selected.append(va)
                added_here += 1
                if len(options) >= limit or added_here >= per_function:
                    break
            if len(options) >= limit or added_here >= per_function:
                break
        if len(options) >= limit:
            break
    return {"schema": 1, "unit": unit, "function": symbol,
            "axes": [{"name": "ast-shape", "find": anchor.decode("utf-8"),
                      "options": options}],
            "generator": {"name": "homm3-vc6-ast-variants", "scope": "tu",
                          "source_functions": len(set(selected)), "families": families,
                          "depth": depth,
                          "external_diagnostics": errors if allow_external_errors else []}}


def run(args):
    source = source_for_unit(args.unit)
    if source is None:
        raise ValueError(f"unknown VC6 unit {args.unit}")
    families = tuple(item.strip() for item in args.families.split(",") if item.strip())
    if args.va:
        payload = manifest(args.unit, args.fn, int(args.va, 0), source,
                           families=families, depth=args.depth, limit=args.limit,
                           state_trials=args.state_trials, state_family=args.state_family,
                           state_insertion=args.state_insertion, seed=args.seed,
                           allow_external_errors=args.allow_external_errors)
    else:
        if args.state_trials:
            raise ValueError("TU-wide AST search uses a separate state-sweep run")
        payload = manifest_tu(args.unit, args.fn, source, families=families,
                              depth=args.depth, limit=args.limit,
                              allow_external_errors=args.allow_external_errors)
    scope = f"{int(args.va, 0):x}" if args.va else "tu"
    path = Path(args.output) if args.output else (
        common.HOMM3_DIR / "build/hypotheses" /
        f"ast-{args.unit}-{scope}.json")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n")
    count = 1
    for axis in payload["axes"]:
        count *= len(axis["options"])
    print(f"[ast-variants] {count} source/state candidates -> {path}")
    if args.run:
        from argparse import Namespace
        from homm3.vc6 import hypotheses
        return hypotheses.run(Namespace(manifest=str(path), jobs=args.jobs,
                                        limit=max(args.limit, count),
                                        keep_top=args.keep_top, output=None))
    return 0
