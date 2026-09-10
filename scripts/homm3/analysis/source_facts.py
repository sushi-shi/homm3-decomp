"""Review positive Dreamcast source facts against the authored C++ AST.

This is a source review, not a comparison of SH4 and /Z7 structure. Clang
supplies declarations and written calls; VC6/retail still decides codegen.
Unmatched or ambiguous facts stay visible. No statements are synthesized,
counts equalized, source edited, or matching scores changed by this module.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
import subprocess
from typing import Any

from homm3 import manifest
from homm3.build import compilation_database
from homm3.core import clang, common
from homm3.core.cc_wrap import ZLIB_INC
from homm3.vc6 import _source

SCHEMA = "homm3.source-facts.v1"
CAUTION = (
    "Review leads from positive Dreamcast records, not retail-source verdicts. "
    "Zero findings means no disagreement in the checked facts, not complete "
    "source recovery. Coverage gaps and intentional platform differences "
    "must remain visible. Validate every adopted change with VC6 and retail."
)


def name_key(name: str) -> str:
    """Project case/underscore normalization; never drop type/ref syntax."""
    return name.replace("_", "").replace(" ", "").casefold()


def helper_key(name: str) -> str:
    """Recognize VC6's standard selector spelling without merging wrappers."""
    key = name_key(name).removeprefix("::")
    return {"std::cppmin": "std::min", "std::cppmax": "std::max"}.get(key, key)


@dataclass(frozen=True)
class TypeFacts:
    base: str
    qualifiers: tuple[tuple[str, ...], ...]
    indirections: tuple[str, ...]
    arrays: tuple[str, ...]


def type_facts(text: str) -> TypeFacts | None:
    """Read cv/ref layers, refusing declarators we cannot interpret reliably.

    const T* and T* const differ; const T& and T const& agree. Template
    arguments remain part of the base type and are not flattened away.
    Function/member-pointer declarators require a richer type tree.
    """
    text = re.sub(r"\b(class|struct|enum)\s+", "", text).strip()
    if not text or "(" in text or ")" in text or "::*" in text.replace(" ", ""):
        return None
    arrays = []
    while m := re.search(r"\[([^][]*)\]\s*$", text):
        arrays.insert(0, re.sub(r"\s+", "", m[1]))
        text = text[:m.start()].rstrip()
    tokens = re.findall(r"&&|::|\w+|[^\s]", text)
    layers: list[list[str]] = [[]]
    pointers = []
    depth = 0
    for token in tokens:
        if token == "<":
            depth += 1
        elif token == ">":
            depth -= 1
            if depth < 0:
                return None
        if depth == 0 and token in ("*", "&", "&&"):
            pointers.append(token)
            layers.append([])
        else:
            layers[-1].append(token)
    if depth:
        return None
    qualifiers = []
    base = []
    for index, layer in enumerate(layers):
        cv = []
        nested = 0
        for token in layer:
            if token == "<": nested += 1
            if token == ">": nested -= 1
            if nested == 0 and token in ("const", "volatile"):
                cv.append(token)
            elif index == 0:
                base.append(token)
            else:
                return None
        qualifiers.append(tuple(sorted(cv)))
    if not base:
        return None
    return TypeFacts(" ".join(base), tuple(qualifiers), tuple(pointers), tuple(arrays))


def _base_type_key(text: str) -> str:
    """Expand known standard defaults without erasing explicit arguments.

    Clang can desugar std::string to basic_string<char>, while CodeView
    spells out all three template arguments. Keep nondefault traits and
    allocators distinct, including in nested template arguments.
    """
    key = name_key(text).removeprefix("::")
    head, opening, tail = key.partition("<")
    if not opening or not tail.endswith(">"):
        return key
    arguments = []
    depth = start = 0
    body = tail[:-1]
    for index, token in enumerate(body):
        if token == "<": depth += 1
        elif token == ">": depth -= 1
        elif token == "," and depth == 0:
            arguments.append(body[start:index])
            start = index + 1
        if depth < 0:
            return key
    if depth or not body[start:]:
        return key
    arguments.append(body[start:])
    arguments = [_base_type_key(arg) for arg in arguments]
    if head == "std::basicstring":
        if len(arguments) == 1:
            arguments.append("std::chartraits<" + arguments[0] + ">")
        if len(arguments) == 2:
            arguments.append("std::allocator<" + arguments[0] + ">")
    return head + "<" + ",".join(arguments) + ">"


def type_differences(expected: str, actual: str) -> tuple[list[str], list[str]]:
    left, right = type_facts(expected), type_facts(actual)
    if left is None or right is None:
        return [], ["type declarator is outside the supported cv/ref grammar"]
    differences = []
    if left.qualifiers != right.qualifiers:
        differences.append("qualifiers")
    if left.indirections != right.indirections:
        differences.append("reference/pointer")
    # Extents and base types are reviewed too, but distinguished from cv/ref.
    if _base_type_key(left.base) != _base_type_key(right.base):
        differences.append("base-type")
    if left.arrays != right.arrays:
        differences.append("array-extent")
    return differences, []


def json_documents(text: str) -> list[dict]:
    """Clang's filtered AST dump contains adjacent JSON documents."""
    decoder = json.JSONDecoder()
    rows = []
    at = 0
    while at < len(text):
        while at < len(text) and text[at].isspace(): at += 1
        if at == len(text): break
        row, at = decoder.raw_decode(text, at)
        rows.append(row)
    return rows


def _loc(node: dict, key: str = "loc") -> dict:
    value = node.get(key, {})
    return value.get("expansionLoc", value)


def _begin(node: dict) -> dict:
    value = node.get("range", {}).get("begin", {})
    return value.get("expansionLoc", value)


def _line(source: str, offset: int) -> int:
    return source.count("\n", 0, offset) + 1


def _type(node: dict) -> str:
    value = node.get("type", {})
    return value.get("desugaredQualType", value.get("qualType", ""))


def _unwrap(node: dict) -> dict:
    while node.get("kind") in ("ImplicitCastExpr", "ParenExpr", "ExprWithCleanups") \
            and len(node.get("inner", [])) == 1:
        node = node["inner"][0]
    return node


def _receiver(text: str) -> str | None:
    facts = type_facts(text)
    return facts.base if facts else None


def _call_name(node: dict, source: str) -> str | None:
    inner = node.get("inner", [])
    if node.get("kind") in ("CXXConstructExpr", "CXXTemporaryObjectExpr"):
        owner = _receiver(_type(node))
        return owner + "::" + owner.rsplit("::", 1)[-1] if owner else None
    if not inner:
        return None
    head = _unwrap(inner[0])
    if head.get("kind") == "MemberExpr":
        receiver = head.get("inner", [])
        owner = _receiver(_type(receiver[0])) if receiver else None
        return owner + "::" + head["name"] if owner else None
    if head.get("kind") == "DeclRefExpr":
        declaration = head.get("referencedDecl", {})
        name = declaration.get("name")
        begin = _begin(head).get("offset")
        end = head.get("range", {}).get("end", {})
        end = end.get("expansionLoc", end)
        written = source[begin:end.get("offset", 0) + end.get("tokLen", 0)] if begin is not None else ""
        qualified = re.fullmatch(r"([\w:]+)(?:<[\s\S]*>)?", written)
        if qualified and "::" in qualified[1]:
            # Template arguments identify an overload, not a different named
            # helper group. Preserve the written namespace for explicit calls.
            return qualified[1]
        if declaration.get("kind") in ("CXXMethodDecl", "CXXConversionDecl"):
            owner = _receiver(_type(inner[1])) if len(inner) > 1 else None
            return owner + "::" + name if owner and name else None
        if declaration.get("kind") == "FunctionDecl":
            # An unresolved namespace stays a correlation, not an overload ID.
            return name
    return None


def _aliases(source: str, node: dict) -> list[str]:
    """Use owning comments, never invent a second name ledger."""
    at = _begin(node).get("offset", _loc(node).get("offset", 0))
    preceding = source[:at].splitlines()
    found = []
    for line in reversed(preceding[-5:]):
        if not line.strip():
            continue
        if not line.lstrip().startswith("//"):
            break
        match = re.search(r"Before normalization(?:\s*\([^)]*\))?:\s*(.*?)\.\s*$", line)
        if match:
            found.extend(part.strip() for part in match[1].split(","))
    return found


def extract_function(documents: list[dict], mangled: str, path: Path,
                     source: str) -> dict:
    matches = []
    for node in documents:
        if node.get("mangledName") != mangled or node.get("isImplicit"):
            continue
        if not any(child.get("kind") == "CompoundStmt" for child in node.get("inner", [])):
            continue
        file = _loc(node).get("file")
        if file and Path(file).resolve() == path.resolve():
            matches.append(node)
    if len(matches) != 1:
        raise ValueError(f"expected one authored definition of {mangled}, found {len(matches)}")
    root = matches[0]
    function_type = _type(root)
    signature = re.fullmatch(r"([^()]*)\([^()]*\)(.*)", function_type)
    result: dict[str, Any] = {
        "name": root.get("name"), "mangled": mangled,
        "path": str(path), "line": _line(source, _loc(root).get("offset", 0)),
        "function_type": function_type, "parameters": [], "locals": [],
        "calls": [], "automatic_objects": [], "gaps": [],
    }
    if signature:
        if root["kind"] not in ("CXXConstructorDecl", "CXXDestructorDecl"):
            result["return"] = signature[1].strip()
        result["method_cv"] = sorted(re.findall(r"\b(?:const|volatile)\b", signature[2]))
    else:
        result["gaps"].append("function return/member qualifiers use an unsupported declarator")
    for child in root.get("inner", []):
        if child.get("kind") == "ParmVarDecl":
            result["parameters"].append({"name": child.get("name", ""),
                "type": _type(child), "line": _line(source, _loc(child).get("offset", 0)),
                "aliases": _aliases(source, child)})

    def visit(node: dict, statement: int | None = None) -> None:
        kind = node.get("kind")
        if node.get("isInvalid") or kind == "RecoveryExpr":
            raise ValueError("selected function contains invalid/recovered AST nodes")
        offset = _begin(node).get("offset")
        if kind in ("LambdaExpr", "CXXRecordDecl", "FunctionDecl", "CXXMethodDecl"):
            result["gaps"].append("nested callable/type body excluded from caller facts")
            return
        if kind == "CXXDefaultArgExpr":
            return  # supplied by a declaration, not written at this call site
        if kind == "VarDecl":
            result["locals"].append({"name": node.get("name", ""),
                "type": _type(node), "line": _line(source, _loc(node).get("offset", 0)),
                "aliases": _aliases(source, node)})
            typ = type_facts(_type(node))
            if typ and not typ.indirections and node.get("storageClass") not in ("static", "extern") \
                    and not node.get("tls"):
                result["automatic_objects"].append({"type": typ.base,
                    "line": _line(source, _loc(node).get("offset", 0))})
        if kind == "CXXBindTemporaryExpr" and node.get("dtor"):
            typ = type_facts(_type(node))
            if typ:
                result["automatic_objects"].append({"type": typ.base,
                    "line": _line(source, offset or 0)})
        if kind in ("CallExpr", "CXXMemberCallExpr", "CXXOperatorCallExpr", "CXXConstructExpr", "CXXTemporaryObjectExpr"):
            name = _call_name(node, source)
            if name and offset is not None:
                result["calls"].append({"name": name, "offset": offset,
                    "line": _line(source, offset), "statement": statement})
            else:
                result["gaps"].append(f"unresolved authored call at line {_line(source, offset or 0)}")
        for child in node.get("inner", []):
            child_statement = _begin(child).get("offset") if kind == "CompoundStmt" else statement
            visit(child, child_statement)

    for child in root.get("inner", []):
        if child.get("kind") in ("CompoundStmt", "CXXCtorInitializer"):
            visit(child)
    result["calls"].sort(key=lambda row: row["offset"])
    result["gaps"] = sorted(set(result["gaps"]))
    return result


def parse_candidate(path: Path, mangled: str, root: Path = common.HOMM3_DIR) -> dict:
    compiler, includes = clang.clang_bin(), clang.mirror()
    if not compiler or not includes:
        raise ValueError("Clang or the VC6 header mirror is unavailable")
    commands = compilation_database.commands(manifest.load(root / "config/units.toml"),
        root, compiler, [includes, root / "include", root / ZLIB_INC])
    commands = [row for row in commands if Path(row["file"]).resolve() == path.resolve()]
    if len(commands) != 1:
        raise ValueError(f"expected one compiler profile for {path}, found {len(commands)}")
    names = _source.source_names(mangled)
    if not names:
        raise ValueError("compiler-generated or unsupported source identity")
    args = [arg for arg in commands[0]["arguments"] if arg != "/c"]
    args[-1:-1] = ["-fsyntax-only", "-ferror-limit=0", "-Xclang", "-ast-dump=json", "-Xclang",
                   "-ast-dump-filter=" + names[0]]
    completed = subprocess.run(args, cwd=root, text=True, capture_output=True, timeout=120)
    source = path.read_text()
    candidate = extract_function(json_documents(completed.stdout), mangled, path, source)
    if completed.returncode:
        candidate["gaps"].append("Clang reported TU errors; selected definition was recovered, "
                                 "but the audit is incomplete:\n" + completed.stderr[-3000:])
    candidate["source_sha256"] = hashlib.sha256(source.encode()).hexdigest()
    return candidate


def compare_facts(expected: dict, candidate: dict) -> dict:
    """Facts use source names/types/line anchors, never machine-code counts."""
    findings = []
    gaps = list(expected.get("gaps", [])) + list(candidate.get("gaps", []))
    checked = Counter()

    def finding(kind: str, subject: str, dc: Any, cpp: Any, **where) -> None:
        identity = json.dumps([kind, subject, dc, cpp], sort_keys=True)
        findings.append({"id": hashlib.sha256(identity.encode()).hexdigest()[:12],
            "kind": kind, "subject": subject, "dreamcast": dc, "candidate": cpp, **where})

    def compare_type(dc: dict, cpp: dict, role: str) -> None:
        differences, unsupported = type_differences(dc["type"], cpp["type"])
        subject = role + " " + (dc.get("name") or "<unnamed>")
        if unsupported:
            gaps.append(subject + ": " + "; ".join(unsupported))
            return
        checked[role + " types"] += 1
        if differences:
            finding("type", subject, dc["type"], cpp["type"],
                    aspects=differences, candidate_line=cpp.get("line"))

    if "return" in expected:
        if "return" in candidate:
            compare_type({"name": "", "type": expected["return"]},
                         {"type": candidate["return"]}, "return")
        else:
            gaps.append("candidate return type unavailable")
    if "method_cv" in expected:
        if "method_cv" in candidate:
            checked["member qualifiers"] += 1
            if expected["method_cv"] != candidate["method_cv"]:
                finding("member-qualifiers", "this", expected["method_cv"], candidate["method_cv"])
        else:
            gaps.append("candidate member qualifiers unavailable")
    dc_params = [row for row in expected.get("parameters", [])
                 if row["name"] not in ("this", "__$ReturnUdt")]
    cpp_params = candidate.get("parameters", [])
    if len(dc_params) == len(cpp_params):
        for dc, cpp in zip(dc_params, cpp_params):
            compare_type(dc, cpp, "parameter")
        left = [name_key(row["name"]) for row in dc_params]
        right = [name_key(row["name"]) for row in cpp_params]
        if len(set(left)) == len(left) and set(left) == set(right) and left != right:
            finding("parameter-order", "named parameters", [row["name"] for row in dc_params],
                    [row["name"] for row in cpp_params])
    else:
        gaps.append(f"parameter inventory differs ({len(dc_params)} recorded, {len(cpp_params)} authored); "
                    "optimized-out/hidden parameters or a platform ABI difference need review")
    # Match locals only through their names or an explicit owning alias comment.
    local_index = defaultdict(list)
    for row in candidate.get("locals", []):
        # An explicit owning alias takes precedence over coincidental spelling:
        # DC's player/iThisPlayer can otherwise join the wrong C++ local.
        names = row.get("aliases") or [row["name"]]
        for key in {name_key(name) for name in names}:
            local_index[key].append(row)
    dc_counts = Counter(name_key(row["name"]) for row in expected.get("locals", []))
    for row in expected.get("locals", []):
        matches = local_index[name_key(row["name"])]
        if len(matches) != 1 or dc_counts[name_key(row["name"])] != 1:
            gaps.append(f"local {row['name']}: " + ("no named/annotated C++ counterpart" if not matches
                        else "ambiguous shadowed name; scope correlation required"))
        else:
            compare_type(row, matches[0], "local")
    # Source call order is checked only for unique named anchors on distinct
    # source lines/statements. Do not compare argument evaluation order,
    # repeated loop calls, or SH4 scheduling/address order.
    dc_calls = defaultdict(list)
    for row in expected.get("calls", []):
        dc_calls[helper_key(row["name"])].append(row)
    cpp_calls = defaultdict(list)
    for row in candidate.get("calls", []):
        cpp_calls[helper_key(row["name"])].append(row)
    anchors = []
    for key, rows in dc_calls.items():
        # The two public selectors have unambiguous source names. Other STL
        # groups still need correlation of port-specific implementation types.
        if (key.startswith("std::") and key not in ("std::min", "std::max")) \
                or rows[0]["name"].startswith(("_", "?", "<")):
            continue
        checked["named helper groups"] += 1
        actual = cpp_calls.get(key, [])
        # A source object lifetime owns its destructor even though Clang does
        # not emit an authored CallExpr at scope exit. Do not invent an order
        # anchor from the declaration's location or count heap pointers here.
        if not actual and "::~" in key:
            owner = key.rsplit("::~", 1)[0]
            if any(name_key(obj["type"]) == owner for obj in candidate.get("automatic_objects", [])):
                checked["automatic destructor boundaries"] += 1
                continue
        if not actual:
            finding("helper", rows[0]["name"],
                    [{"file": row.get("file"), "line": row["line"]} for row in rows],
                    "no corresponding authored call; inspect expansion or platform boundary")
        elif len(rows) == 1 and len(actual) == 1:
            anchors.append((rows[0], actual[0]))
    for index, (left_dc, left_cpp) in enumerate(anchors):
        for right_dc, right_cpp in anchors[index + 1:]:
            if left_dc.get("file") != right_dc.get("file") or left_dc["line"] == right_dc["line"]:
                continue
            if left_cpp.get("statement") is None or right_cpp.get("statement") is None \
                    or left_cpp["statement"] == right_cpp["statement"]:
                continue
            checked["source-order pairs"] += 1
            if (left_dc["line"] < right_dc["line"]) != (left_cpp["offset"] < right_cpp["offset"]):
                finding("source-order", left_dc["name"] + " / " + right_dc["name"],
                        [left_dc["line"], right_dc["line"]], [left_cpp["line"], right_cpp["line"]])
    return {"findings": findings, "coverage_gaps": sorted(set(gaps)), "checked": dict(checked)}


def expected_facts(dossier, procedure, types) -> dict:
    shape = dossier.shape
    owner_file = shape.source_file.replace("\\", "/").rsplit("/", 1)[-1].casefold()
    calls = []
    for statement in shape.statements:
        if statement.source_file.replace("\\", "/").rsplit("/", 1)[-1].casefold() != owner_file:
            continue
        for call in statement.calls:
            if call.name:
                calls.append({"name": call.name, "file": statement.source_file,
                              "line": statement.source_line})
    result = {"parameters": [], "locals": [row.to_dict() for row in shape.locals],
              "calls": calls, "gaps": []}
    if shape.line_map and shape.line_map.bodyless:
        result["gaps"].append("Dreamcast has a minimal/bodyless procedure; implementation source facts unavailable")
    function = types.get(procedure.type_index)
    if function["kind"] != "function":
        result["gaps"].append("formal Dreamcast signature unavailable; parameter inventory is only a lower bound")
        return result
    arguments = types.get(function["arguments"]).get("types")
    if arguments is None:
        result["gaps"].append("formal Dreamcast argument list unavailable")
        return result
    parameters = [v for v in procedure.variables if v.kind == "param" and v.name not in ("this", "__$ReturnUdt")]
    complete_names = len(parameters) == sum(t != 0 for t in arguments)
    if not complete_names:
        result["gaps"].append("partial Dreamcast parameter names; named parameter order unchecked")
    for index, typ in enumerate(arguments):
        if typ == 0:
            result["gaps"].append("variadic signature boundary requires review")
            continue
        result["parameters"].append({"name": parameters[index].name if complete_names else "",
                                     "type": types.declaration(typ)})
    owner = types.get(function.get("owner", 0)).get("name", "").rsplit("::", 1)[-1]
    method = procedure.name.rsplit("::", 1)[-1]
    if function["returns"] and not (owner and method in (owner, "~" + owner)):
        result["return"] = types.declaration(function["returns"])
    this_type = function.get("this")
    if this_type:
        facts = type_facts(types.declaration(this_type))
        if facts:
            result["method_cv"] = list(facts.qualifiers[0])
        else:
            result["gaps"].append("Dreamcast member qualifiers unavailable")
    return result


def audit(corpus, row: dict, *, dump=None, data=None, type_table=None) -> dict:
    from homm3.analysis import dc_lines, dreamcast
    from homm3.core.nb11_types import Types
    if dump is None:
        dump = dc_lines.load_symbols()
    if type_table is None:
        type_table = Types.from_symbols(dump)
    dossier = dreamcast.build_dossier(corpus, row, dump=dump, data=data, type_table=type_table)
    output = {"function": dossier.shape.name, "module": dossier.shape.module,
        "dc_offset": dossier.shape.address, "findings": [], "coverage_gaps": [], "checked": {}}
    claims = {(item["va"], item["path"]) for item in dossier.retail_source_claims}
    if len(claims) != 1:
        output["coverage_gaps"] = [f"expected one source claim, found {len(claims)}"]
        return output
    va, relative = next(iter(claims))
    output.update(retail_va=va, source=relative)
    mangled = corpus._retail_name(va - common.IMAGE_BASE)
    if not mangled:
        output["coverage_gaps"] = ["retail/source symbol binding unavailable; run homm3 build"]
        return output
    try:
        candidate = parse_candidate(common.HOMM3_DIR / relative, mangled)
    except (ValueError, OSError, subprocess.SubprocessError, json.JSONDecodeError) as exc:
        output["coverage_gaps"] = [str(exc)]
        return output
    output.update(compare_facts(expected_facts(dossier, dump.procedures[dossier.shape.address], type_table), candidate))
    output["source_sha256"] = candidate["source_sha256"]
    return output


def run(corpus, rows: list[dict], *, as_json: bool = False) -> int:
    from homm3.analysis import dc_lines
    from homm3.core import inputs
    from homm3.core.nb11_types import Types
    dump, data = dc_lines.load_symbols(), inputs.read_dreamcast_exe()
    types = Types.from_symbols(dump)
    results = [audit(corpus, row, dump=dump, data=data, type_table=types) for row in rows]
    total_findings = sum(len(row["findings"]) for row in results)
    total_gaps = sum(len(row["coverage_gaps"]) for row in results)
    payload = {"schema": SCHEMA, "caution": CAUTION, "functions": results,
               "summary": {"functions": len(results), "findings": total_findings, "coverage_gaps": total_gaps}}
    if as_json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print("SOURCE FACT REVIEW — " + CAUTION)
        for row in results:
            print(f"\n{row['function']} [{row['module']} dc:{row['dc_offset']:#x}]")
            if row.get("source"): print("  C++: " + row["source"])
            for item in row["findings"]:
                aspects = " (" + ", ".join(item["aspects"]) + ")" if item.get("aspects") else ""
                print(f"  REVIEW {item['id']} {item['kind']}{aspects}: {item['subject']}")
                print(f"    DC:  {item['dreamcast']}")
                print(f"    C++: {item['candidate']}")
            for gap in row["coverage_gaps"]: print("  UNCHECKED: " + gap)
            print("  checked: " + (", ".join(f"{key}={value}" for key, value in row["checked"].items()) or "none"))
        print(f"\n{len(results)} function(s), {total_findings} review finding(s), {total_gaps} coverage gap(s)")
    return 2 if total_gaps else 1 if total_findings else 0
