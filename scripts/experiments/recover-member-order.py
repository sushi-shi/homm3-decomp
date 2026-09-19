"""Recover header declaration order from the Dreamcast CodeView field lists.

This is an ordering proposal, not a transcription of the missing header.
Overloads share their field-list name's position; their authored order stays
stable. Repeated class records must agree. Retail data-member and virtual-method
order are hard constraints. Uncorrelated declarations, nested declarations,
friends and preprocessor directives are barriers, not guessed source positions.
Comments travel with their declarations. Access is preserved, never inferred
from a neighbour. Run the access audit and full VC6 build after applying.

    python scripts/experiments/recover-member-order.py --output build/member-order.json
    python scripts/experiments/recover-member-order.py --apply-plan build/member-order.json
"""
import argparse
from collections import defaultdict
import hashlib
import heapq
import importlib.util
import json
from pathlib import Path
import re
import sys

from homm3 import manifest
from homm3.analysis import access_facts as access
from homm3.analysis.dc_lines import load_symbols
from homm3.analysis.source_facts import name_key
from homm3.build import compilation_database
from homm3.core import clang, common
from homm3.core.project import Project
from homm3.core.nb11_types import Types
from homm3.retail_labels.source import mask_lexical_noise


def digest(source):
    return hashlib.sha256(source.encode()).hexdigest()


def recorded_order(types):
    """Name-group ordinals, only where every complete class record agrees."""
    variants = defaultdict(set)
    for index in types.records:
        item = types.get(index)
        if item.get("kind") not in ("class", "struct") or item.get("forward"):
            continue
        fields = types.fields(item.get("fields", 0))
        if any(f["kind"] == "unresolved" for f in fields):
            continue
        names = tuple(dict.fromkeys(name_key(f["name"]) for f in fields
                      if f.get("name") and f["kind"] in
                      ("method", "member", "static_member")))
        if names:
            variants[name_key(item["name"])].add(names)
    ranks, conflicts = {}, []
    for owner, sequences in variants.items():
        if len(sequences) == 1:
            ranks[owner] = {name: i for i, name in enumerate(next(iter(sequences)))}
        else:
            conflicts.append(owner)
    return ranks, sorted(conflicts)


def constrained_order(members):
    """Prioritize recorded order subject to retail layout/slot dependencies."""
    edges = [set() for _ in members]
    previous = {}
    for i, member in enumerate(members):
        groups = []
        if member["kind"] == "member":
            groups.append("data")
        if member.get("virtual"):
            groups.append("virtual")
        groups.append(("overload", member["rank"]))
        for group in groups:
            if group in previous:
                edges[previous[group]].add(i)
            previous[group] = i
    indegree = [0] * len(members)
    for successors in edges:
        for j in successors:
            indegree[j] += 1
    ready = [(m["rank"], i) for i, m in enumerate(members) if not indegree[i]]
    heapq.heapify(ready)
    result = []
    while ready:
        _, i = heapq.heappop(ready)
        result.append(i)
        for j in edges[i]:
            indegree[j] -= 1
            if not indegree[j]:
                heapq.heappush(ready, (members[j]["rank"], j))
    assert len(result) == len(members)
    return result


LABEL = re.compile(r"^[ \t]*(public|private|protected):[ \t]*(?=\n|$)", re.M)
DIRECTIVE = re.compile(r"^[ \t]*#", re.M)


def without_labels(text):
    """Remove class-scope labels while retaining comments on the same line."""
    for match in reversed(list(LABEL.finditer(mask_lexical_noise(text)))):
        end = match.start() + re.match(r"[ \t]*(?:public|private|protected):[ \t]*",
                                      text[match.start():]).end()
        if end < len(text) and text[end] == "\n":
            end += 1
        text = text[:match.start()] + text[end:]
    return text


def render_segment(members, order, indent):
    result, current = [], None
    for i in order:
        member = members[i]
        if member["access"] != current:
            result.append(f"{indent}{member['access']}:\n")
            current = member["access"]
        result.append(member["text"])
    # A following nested type/friend may inherit the final member's access.
    if current != members[-1]["access"]:
        result.append(f"{indent}{members[-1]['access']}:\n")
    return "".join(result)


def method_end(masked, start):
    """Find the complete declaration, including a parser-skipped inline body.

    PARSE_SKIP_FUNCTION_BODIES can stop a cursor before its opening brace.
    Scan balanced source tokens, never use that truncated end as a move range.
    Parentheses hide constructor initializers/default arguments; the first
    outer semicolon is a prototype and the first outer brace starts the body.
    """
    parens = brackets = 0
    for at in range(start, len(masked)):
        ch = masked[at]
        if ch == "(":
            parens += 1
        elif ch == ")":
            parens -= 1
        elif ch == "[":
            brackets += 1
        elif ch == "]":
            brackets -= 1
        elif not parens and not brackets:
            if ch == ";":
                return at + 1
            if ch == "{":
                depth = 1
                for end in range(at + 1, len(masked)):
                    depth += (masked[end] == "{") - (masked[end] == "}")
                    if depth == 0:
                        return end + 1
                raise RuntimeError("unterminated inline body")
    raise RuntimeError("unterminated method declaration")


def class_edits(source, record, ranks, dc, ci):
    lines = source.splitlines(keepends=True)
    masked_source = mask_lexical_noise(source)
    children = [c for c in record.get_children()
                if c.kind != ci.CursorKind.CXX_ACCESS_SPEC_DECL
                and c.location.file and c.location.file.name == record.location.file.name]
    owner = access.qualified_owner(record, ci)
    owner = f"{owner}::{record.spelling}" if owner else record.spelling
    class_key = name_key(owner)
    rank_map = ranks.get(class_key)
    if rank_map is None:
        return [], []
    tokens = list(record.get_tokens())
    opening = next((t for t in tokens if t.spelling == "{"), None)
    if opening is None:
        return [], []
    previous_end = opening.location.line
    if source.splitlines()[previous_end - 1][opening.location.column:].strip():
        return [], [{"class": owner, "reason": "single-line class opening"}]
    indent = re.match(r"\s*", lines[record.extent.start.line - 1]).group()
    method_kinds = (ci.CursorKind.CXX_METHOD, ci.CursorKind.CONSTRUCTOR,
                    ci.CursorKind.DESTRUCTOR)
    accesses = {ci.AccessSpecifier.PUBLIC: "public", ci.AccessSpecifier.PRIVATE: "private",
                ci.AccessSpecifier.PROTECTED: "protected"}
    edits, barriers, segment = [], [], []

    def flush():
        if segment:
            order = constrained_order(segment)
            if order != list(range(len(segment))):
                edits.append({"start": segment[0]["start"], "end": segment[-1]["end"],
                              "replacement": render_segment(segment, order, indent),
                              "class": owner,
                              "before": [m["name"] for m in segment],
                              "after": [segment[i]["name"] for i in order]})
            segment.clear()

    for child_index, child in enumerate(children):
        start, end = child.extent.start.line - 1, child.extent.end.line
        is_method = child.kind in method_kinds
        # Offset is a byte offset, whereas source operations consume text.
        offset = len(source.encode()[:child.extent.start.offset].decode())
        if is_method:
            end = source.count("\n", 0, method_end(masked_source, offset) - 1) + 1
        shares_next = (child_index + 1 < len(children)
                       and children[child_index + 1].extent.start.line <= end)
        if start < previous_end or end >= record.extent.end.line or shares_next:
            flush()
            barriers.append({"class": owner, "member": child.spelling,
                             "reason": "shared declaration/class-end line"})
            previous_end = max(previous_end, end)
            continue
        kind = ("method" if is_method else "member" if child.kind == ci.CursorKind.FIELD_DECL
                else "static_member" if child.kind == ci.CursorKind.VAR_DECL else None)
        keys = access.owning_member_keys(source, offset, owner, child.spelling)
        fact = {"class_key": class_key, "member_key": name_key(child.spelling),
                "member_keys": keys, "kind": kind, "is_method": is_method,
                "parameters": [a.type.get_canonical().spelling for a in child.get_arguments()]
                              if is_method else None,
                "const": child.is_const_method() if child.kind == ci.CursorKind.CXX_METHOD else False}
        matched, gap = access.correlate(fact, dc) if kind else (None, "nested/friend/type declaration")
        rank = rank_map.get(name_key(matched["display"].rpartition("::")[2])) if matched else None
        gap_text = "".join(lines[previous_end:start])
        declaration = "".join(lines[start:end])
        # Nontrivia in a gap includes inactive declarations, macros, directives.
        gap_mask = LABEL.sub("", mask_lexical_noise(gap_text))
        leading = previous_end
        if gap_mask.strip():
            flush()
            barriers.append({"class": owner, "member": child.spelling,
                             "reason": "preprocessor or unparsed gap"})
            # Only attach the contiguous trailing trivia, leaving the barrier.
            while leading < start and LABEL.sub("", mask_lexical_noise("".join(lines[leading:start]))).strip():
                leading += 1
            gap_text = "".join(lines[leading:start])
        if rank is None or child.access_specifier not in accesses or DIRECTIVE.search(declaration):
            flush()
            barriers.append({"class": owner, "member": child.spelling,
                             "reason": gap or "directive in declaration"})
            previous_end = end
            continue
        # Strip only bare labels at class scope. Labels in bodies stay intact.
        gap_text = without_labels(gap_text)
        segment.append({"start": leading, "end": end, "name": child.spelling,
                        "rank": rank, "kind": kind,
                        "virtual": is_method and child.is_virtual_method(),
                        "access": accesses[child.access_specifier],
                        "text": gap_text + declaration})
        previous_end = end
    flush()
    return edits, barriers


def make_plan(modules):
    ci = access.load_cindex()
    root, mirror = common.HOMM3_DIR, clang.mirror()
    if mirror is None:
        raise RuntimeError("VC6 mirror unavailable; build first")
    types = Types.from_symbols(load_symbols())
    ranks, conflicts = recorded_order(types)
    dc = access.dc_visibility(types)
    commands = compilation_database.commands(manifest.load(root / "config/units.toml"), root,
        clang.clang_bin(), [mirror, *Project(root).includes])
    commands = [r for r in commands if Path(r["file"]).exists()
                and (not modules or Path(r["file"]).stem in modules)]
    index, seen, sources = ci.Index.create(), set(), {}
    changes, barriers, layouts = defaultdict(list), [], {}
    record_kinds = (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL, ci.CursorKind.UNION_DECL)
    for n, row in enumerate(commands, 1):
        src = Path(row["file"])
        print(f"[{n}/{len(commands)}] {src.name}", file=sys.stderr, flush=True)
        args = [a for a in row["arguments"][1:] if a != "/c" and Path(a).resolve() != src.resolve()]
        tu = index.parse(str(src), args=["--driver-mode=cl", "-fsyntax-only", "-ferror-limit=0"] + args,
                         options=ci.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
        errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
        if errors:
            raise RuntimeError("\n".join(errors))
        pending = list(tu.cursor.get_children())
        while pending:
            rec = pending.pop()
            if rec.location.file is None:
                continue
            path = Path(rec.location.file.name)
            if not access.is_project_file(path, root, mirror):
                continue
            if rec.kind == ci.CursorKind.NAMESPACE or rec.kind in record_kinds:
                pending.extend(rec.get_children())
            if rec.kind not in record_kinds or not rec.is_definition() or not path.is_relative_to(root / "include"):
                continue
            key = (str(path), rec.extent.start.offset)
            if key in seen:
                continue
            seen.add(key)
            source = sources.setdefault(path, path.read_text())
            owner = access.qualified_owner(rec, ci)
            owner = f"{owner}::{rec.spelling}" if owner else rec.spelling
            layout_key = f"{path.relative_to(root)}:{owner}"
            method_kinds = (ci.CursorKind.CXX_METHOD, ci.CursorKind.CONSTRUCTOR,
                            ci.CursorKind.DESTRUCTOR)
            declarations, fields, virtuals = [], [], []
            for child in rec.get_children():
                if child.kind == ci.CursorKind.CXX_ACCESS_SPEC_DECL:
                    continue
                identity = (child.kind.name, child.spelling, child.type.spelling,
                            child.access_specifier.name)
                declarations.append(identity)
                if child.kind == ci.CursorKind.FIELD_DECL:
                    fields.append((*identity, child.get_field_offsetof()))
                if child.kind in method_kinds and child.is_virtual_method():
                    virtuals.append(identity)
            layouts[layout_key] = {"size": rec.type.get_size(), "fields": fields,
                                   "virtuals": virtuals, "declarations": sorted(declarations)}
            edits, skipped = class_edits(source, rec, ranks, dc, ci)
            changes[str(path.relative_to(root))].extend(edits)
            barriers.extend(skipped)
    if any(path.read_text() != source for path, source in sources.items()):
        raise RuntimeError("source changed during scan")
    return {"root": str(root), "classes_seen": len(seen), "record_conflicts": conflicts,
            "layouts": layouts,
            "barriers": barriers,
            "files": {path: {"sha256": digest(sources[root / path]), "edits": edits}
                      for path, edits in changes.items() if edits}}


def apply_plan(plan):
    root = common.HOMM3_DIR
    if plan["root"] != str(root):
        raise RuntimeError("plan belongs to another worktree")
    spec = importlib.util.spec_from_file_location("access_transform",
            Path(__file__).with_name("apply-access-adherence.py"))
    transform = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(transform)
    outputs = {}
    for relative, file in plan["files"].items():
        path = root / relative
        source = path.read_text()
        if digest(source) != file["sha256"]:
            raise RuntimeError(f"stale plan: {relative}")
        lines = source.splitlines(keepends=True)
        boundary = len(lines)
        for edit in sorted(file["edits"], key=lambda e: e["start"], reverse=True):
            if edit["end"] > boundary:
                raise RuntimeError(f"overlapping edits: {relative}")
            lines[edit["start"]:edit["end"]] = [edit["replacement"]]
            boundary = edit["start"]
        outputs[path] = transform.remove_redundant_access_labels("".join(lines))
    for path, source in outputs.items():
        path.write_text(source)


def layout_differences(before, after):
    def normalized(value):
        # Anonymous type spellings include their source coordinates. Moving
        # the containing declaration changes those coordinates, not the type.
        return re.sub(r"( at [^()\n]+?):\d+:\d+", r"\1", value)
    def grouped(layouts):
        result = defaultdict(list)
        for key, layout in layouts.items():
            result[normalized(key)].append(normalized(json.dumps(layout, sort_keys=True)))
        return {key: sorted(values) for key, values in result.items()}
    before, after = grouped(before), grouped(after)
    return [key for key in sorted(before.keys() | after.keys()) if before.get(key) != after.get(key)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", action="append", default=[])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--apply-plan", type=Path)
    parser.add_argument("--compare-layouts", type=Path,
                        help="fail if this scan changes the saved layout/interface snapshot")
    args = parser.parse_args()
    if args.apply_plan:
        plan = json.loads(args.apply_plan.read_text())
        apply_plan(plan)
    else:
        plan = make_plan(args.module)
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(plan, indent=2) + "\n")
        if args.compare_layouts:
            previous = json.loads(args.compare_layouts.read_text())
            differences = layout_differences(previous["layouts"], plan["layouts"])
            if differences:
                raise RuntimeError(f"layout/interface changed: {differences}")
            print(f"Layout/interface snapshots agree for {len(plan['layouts'])} classes")
    print(json.dumps({"files": len(plan["files"]),
                      "segments": sum(len(f["edits"]) for f in plan["files"].values()),
                      "classes": len({e["class"] for f in plan["files"].values() for e in f["edits"]}),
                      "barriers": len(plan["barriers"]),
                      "conflicting_dc_classes": len(plan["record_conflicts"])}, indent=2))


if __name__ == "__main__":
    main()
