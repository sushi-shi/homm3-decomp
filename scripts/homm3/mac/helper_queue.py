"""Queue Mac-retained call boundaries for Windows functions.

Direct PowerPC branches are leads. A reviewed target establishes identity, but
neither a branch nor a textual call search proves the original inline qualifier.
"""
from __future__ import annotations

from bisect import bisect_right
from collections import Counter, defaultdict
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import tomllib

from homm3.mac import addresses, glue, references, reports, tables
from homm3.mac.discovery import Index
from homm3.mac.source import (_masked_source, class_header_helper, extract_body,
                              load_pairs, source_helper)


def _address(section: int, offset: int) -> str:
    return f"{section}:0x{offset:x}"


def _leaf(item) -> str:
    helper = getattr(item, "source_helper", None)
    if helper:
        return helper.split("(", 1)[0].rsplit("::", 1)[-1]
    return item.signature.split("(", 1)[0].rsplit(" ", 1)[-1].rsplit("::", 1)[-1]


def _call_text(authored: str) -> str:
    """Mask noise and remove the owning declarator, retaining ctor initializers.

    These are lexical call leads, not resolved AST calls. In particular the
    definition of Derived::save must not count as a call to Base::save.
    """
    text = _masked_source(authored)
    opening = text.find("(")
    if opening < 0:
        raise ValueError("source function has no parameter list")
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return text[index + 1:]
    raise ValueError("source function has an unterminated parameter list")


def _helper_body(caller) -> str:
    text = caller.source.read_text()
    finder = class_header_helper if caller.source.suffix == ".h" else source_helper
    try:
        return finder(text, caller.source_helper, caller.source)[2]
    except ValueError:
        if finder is class_header_helper:
            raise
        # Ordinary classes can also be defined inside their owning .cpp.
        return class_header_helper(text, caller.source_helper, caller.source)[2]


def _source_helpers(root: Path, refs: list, pairs: list, pef) -> list:
    """Include new source-only annotations even without a legacy TOML row."""
    claims, _, problems = addresses.scan(root)
    if problems:
        raise ValueError("invalid Mac source claims: " + "; ".join(problems))
    known = {(ref.mac_section, ref.mac_offset) for ref in [*refs, *pairs]}
    source_units = {ref.source: ref.unit for ref in [*refs, *pairs]}
    additions = []
    spans = tables.read_functions(root) if (root / "config/mac/functions.tsv").exists() else None
    for claim in claims:
        if claim.windows_va is not None or claim.compgen is not None:
            continue
        if (tables.CODE_SECTION, claim.offset) in known:
            continue
        if spans is not None and spans.get(claim.offset) != claim.size:
            raise ValueError(f"{claim.where}: source helper has no matching verified Mac span")
        source = root / claim.path
        unit = source_units.get(source)
        if unit is None:
            unit = addresses.unit_of(root, claim.path)
        declaration = _masked_source(source.read_text())[claim.anchor:]
        declaration = re.split(r"[;{]", declaration, maxsplit=1)[0]
        selector = claim.label + claim.parameters
        if re.search(r"\)\s*const\s*$", declaration):
            selector += " const"
        additions.append(references.Reference(
            None, unit, source, claim.label, tables.CODE_SECTION, claim.offset,
            claim.size, None, hashlib.sha256(pef.code(tables.CODE_SECTION, claim.offset,
                                                     claim.size)).hexdigest(),
            f"source MAC_ADDRESS at {claim.where}", selector))
    return additions


def _owner(unit: str, assignments: dict[str, str]) -> str:
    return assignments.get(unit, "unassigned")


def _reviewed_units(root: Path) -> set[str]:
    path = root / "config/mac/helper-review.toml"
    if not path.exists():
        return set()
    return set(tomllib.loads(path.read_text()).get("units", []))


def generate(root: Path, action_queue: dict, index: Index,
             assignments: dict[str, str] | None = None,
             *, all_functions: bool = False) -> dict:
    """Build a source-call review queue from reviewed spans and Mac branches."""
    assignments = assignments or {}
    reviewed_units = _reviewed_units(root)
    selected = {int(row["retail_va"], 0): row for row in action_queue["rows"]
                if all_functions or row.get("windows_max") is None
                or row["windows_max"] < 100 - 1e-6}
    refs = references.load(root)
    pairs = load_pairs(root)
    refs = [*refs, *_source_helpers(root, refs, pairs, index.pef)]
    by_va = {ref.retail_va: ref for ref in refs if ref.retail_va is not None}
    by_va.update({pair.retail_va: pair for pair in pairs})
    by_target = {(ref.mac_section, ref.mac_offset): ref for ref in refs}
    by_target.update({(pair.mac_section, pair.mac_offset): pair for pair in pairs})
    runtime_labels = tables.read_runtime(root)
    runtime_by_target = {(tables.CODE_SECTION, label.offset): label.name
                         for label in runtime_labels}
    indirect_targets = {(tables.CODE_SECTION, label.offset) for label in runtime_labels
                        if label.call_kind == "indirect_tvector"}
    for item in addresses.legacy_runtime(root):
        runtime_by_target.setdefault((item["mac_section"], item["mac_offset"]), item["symbol"])
        if item.get("call_kind") == "indirect_tvector":
            indirect_targets.add((item["mac_section"], item["mac_offset"]))
    for name, target in glue.imports(index.pef).items():
        runtime_by_target.setdefault((target.address.section, target.address.offset), name)

    branches = defaultdict(list)
    for at, target, kind in index.branches:
        if kind == "linked_branch":
            branches[at.section].append((at.offset, target.section, target.offset))
    starts = {}
    for section, sites in branches.items():
        sites.sort()
        starts[section] = [site[0] for site in sites]

    calls = []
    functions = []
    callers = [(f"0x{va:08x}", row, by_va.get(va)) for va, row in sorted(selected.items())]
    if all_functions:
        deferred_units = {row.get("unit") for row in action_queue["rows"]
                          if row.get("state") == "deferred"}
        for ref in refs:
            if ref.retail_va is None and ref.source_helper:
                callers.append((ref.identity, {"unit": ref.unit, "function": ref.signature,
                    "windows_max": None,
                    "state": "deferred" if ref.unit in deferred_units else "review_calls"}, ref))
    for identity, row, caller in callers:
        va_text = identity if identity.startswith("0x") else None
        owner = _owner(row.get("unit") or "", assignments)
        helper_reviewed = caller is not None and row.get("unit") in reviewed_units
        deferred = row.get("state") == "deferred"
        entry = {"owner": owner, "unit": row.get("unit"), "retail_va": va_text, "caller_id": identity,
                 "function": row["function"], "windows_max": row.get("windows_max"),
                 "deferred": deferred, "helper_reviewed": helper_reviewed,
                 "reviewed_mac_span": caller is not None,
                 "reviewed_calls": 0, "missing_named_calls": 0,
                 "direct_mac_calls": None, "source_parse_error": "",
                 "indirect_dispatch_calls": 0,
                 "unreviewed_targets": 0,
                 "state": ("helper_reviewed" if helper_reviewed else
                           "review_calls" if caller else "pair_mac_address")}
        functions.append(entry)
        if caller is None:
            continue
        try:
            authored = (_helper_body(caller) if getattr(caller, "source_helper", None)
                        else extract_body(caller))
            if authored.rstrip().endswith(";"):
                # Some VA claims preserve retail order on a forward declaration.
                # Inspect the canonical definition before reporting a missing
                # source call. Match full parameters first; unresolved overloads
                # stay explicit parse errors.
                name = caller.signature.rsplit(" ", 1)[-1]
                source_text = caller.source.read_text()
                parameters = authored[authored.index("("):].rstrip("; \n\t")
                try:
                    _, _, authored = source_helper(source_text, name + parameters, caller.source)
                except ValueError:
                    _, _, authored = source_helper(source_text, name, caller.source)
            body = _call_text(authored)
            source_error = ""
        except (OSError, ValueError) as exc:
            body = ""
            source_error = str(exc)
        sites = branches.get(caller.mac_section, [])
        lo = bisect_right(starts.get(caller.mac_section, []), caller.mac_offset - 1)
        hi = bisect_right(starts.get(caller.mac_section, []),
                          caller.mac_offset + caller.mac_size - 1)
        caller_sites = sites[lo:hi]
        target_counts = Counter((section, offset) for _, section, offset in caller_sites)
        entry["direct_mac_calls"] = len(caller_sites)
        entry["source_parse_error"] = source_error
        for at, section, offset in caller_sites:
            target = by_target.get((section, offset))
            runtime_name = runtime_by_target.get((section, offset), "")
            name = _leaf(target) if target else ""
            source_count = (len(re.findall(r"(?<![\w])" + re.escape(name) + r"\s*\(", body))
                            if name and not source_error else None)
            source_call = source_count is not None and source_count > 0
            mac_count = target_counts[(section, offset)]
            state = ("review_indirect_call" if (section, offset) in indirect_targets else
                     "runtime_call" if target is None and runtime_name else
                     "identify_target" if target is None else
                     "source_unavailable" if source_error else
                     "review_call_count" if source_call and source_count != mac_count else
                     "source_call_present" if source_call else
                     "review_missing_helper_call" if getattr(target, "source_helper", None) else
                     "review_other_named_call")
            calls.append({"owner": owner, "unit": entry["unit"], "retail_va": entry["retail_va"], "caller_id": identity,
                          "deferred": deferred,
                          "function": entry["function"], "mac_call_site": _address(caller.mac_section, at),
                          "mac_target": _address(section, offset),
                          "target_name": name or runtime_name,
                          "target_unit": target.unit if target else "",
                          "source_call_present": source_call, "source_parse_error": source_error,
                          "source_call_mentions": source_count, "mac_target_calls": mac_count,
                          "state": state})
            entry["reviewed_calls"] += target is not None
            entry["missing_named_calls"] += state == "review_missing_helper_call"
            entry["unreviewed_targets"] += state == "identify_target"
            entry["indirect_dispatch_calls"] += state == "review_indirect_call"
    coverage = {"functions_in_scope": len(functions),
                "windows_functions_in_scope": len(selected),
                "source_helper_callers": sum(row["retail_va"] is None for row in functions),
                "helper_reviewed_functions": sum(row["helper_reviewed"] for row in functions),
                "unfinished_windows_functions": sum(
                    row.get("windows_max") is None or row["windows_max"] < 100 - 1e-6
                    for row in selected.values()),
                "reviewed_mac_callers": sum(row["reviewed_mac_span"] for row in functions),
                "direct_mac_calls": len(calls),
                "indirect_dispatch_calls": sum(row["state"] == "review_indirect_call" for row in calls),
                "call_count_review_groups": len({(row["caller_id"], row["mac_target"])
                                                 for row in calls if row["state"] == "review_call_count"}),
                "source_unavailable_functions": sum(bool(row["source_parse_error"]) for row in functions),
                "missing_named_source_calls": sum(row["state"] == "review_missing_helper_call" for row in calls),
                "unreviewed_direct_targets": len({row["mac_target"] for row in calls
                                                  if row["state"] == "identify_target"})}
    return {"schema": 2,
            "scope": ("all_mac_retained_game_helper_recovery" if all_functions
                      else "mac_retained_helper_recovery"),
            "target_sha256": action_queue.get("target_sha256"),
            "coverage": coverage, "functions": functions, "calls": calls}


def write(root: Path, report: dict, *, stem: str = "helper-queue") -> None:
    out = root / "build/mac"
    out.mkdir(parents=True, exist_ok=True)
    reports.atomic_text(out / f"{stem}.json", json.dumps(report, indent=2) + "\n")
    for name, fields in (
        ("functions", ("owner", "unit", "retail_va", "caller_id", "function", "windows_max", "deferred",
                       "helper_reviewed", "reviewed_mac_span", "reviewed_calls", "missing_named_calls",
                       "unreviewed_targets", "direct_mac_calls", "indirect_dispatch_calls", "source_parse_error", "state")),
        ("calls", ("owner", "unit", "retail_va", "caller_id", "deferred", "mac_call_site", "mac_target",
                   "target_name", "target_unit", "source_call_present", "source_call_mentions",
                   "mac_target_calls", "source_parse_error", "state")),
    ):
        stream = io.StringIO()
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t", extrasaction="ignore")
        writer.writeheader()
        writer.writerows(report[name])
        reports.atomic_text(out / f"{stem}-{name}.tsv", stream.getvalue())


def leads(report: dict, unit: str | None = None,
          include_deferred: bool = False,
          include_other_named: bool = False) -> list[dict]:
    """Group actionable call leads by destination instead of repeating sites."""
    groups = {}
    states = {"review_missing_helper_call", "review_call_count", "source_unavailable",
              "identify_target", "review_indirect_call"}
    if include_other_named:
        states.add("review_other_named_call")
    for row in report["calls"]:
        if row["state"] not in states:
            continue
        if unit and row["unit"] != unit:
            continue
        if row.get("deferred") and not include_deferred:
            continue
        key = (row["state"], row["mac_target"])
        group = groups.setdefault(key, {"state": row["state"],
                                        "mac_target": row["mac_target"],
                                        "target_name": row["target_name"],
                                        "sites": 0, "callers": set(), "units": set(),
                                        "example": row})
        group["sites"] += 1
        group["callers"].add(row["caller_id"])
        group["units"].add(row["unit"])
    result = []
    for group in groups.values():
        group["caller_count"] = len(group.pop("callers"))
        group["unit_count"] = len(group.pop("units"))
        result.append(group)
    rank = {"review_missing_helper_call": 0,
            "review_call_count": 1, "review_other_named_call": 2,
            "source_unavailable": 3, "identify_target": 4, "review_indirect_call": 5}
    return sorted(result, key=lambda group: (
        rank[group["state"]],
        -group["caller_count"], -group["sites"], -group["unit_count"],
        group["mac_target"]))
