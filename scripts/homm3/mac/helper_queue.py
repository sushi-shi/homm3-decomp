"""Queue Mac-retained call boundaries for unfinished Windows functions.

Direct PowerPC branches are leads. A reviewed target establishes identity, but
neither a branch nor a textual call search proves the original inline qualifier.
"""
from __future__ import annotations

from bisect import bisect_right
from collections import defaultdict
import csv
import io
import json
from pathlib import Path
import re
import tomllib

from homm3.mac import references, reports
from homm3.mac.discovery import Index
from homm3.mac.source import _masked_source, extract_body, load_pairs


def _address(section: int, offset: int) -> str:
    return f"{section}:0x{offset:x}"


def _leaf(item) -> str:
    helper = getattr(item, "source_helper", None)
    if helper:
        return helper.split("::")[-1].split("(")[0]
    return item.signature.rsplit(" ", 1)[-1].split("::")[-1].split("(", 1)[0]


def _owner(unit: str, assignments: dict[str, str]) -> str:
    return assignments.get(unit, "unassigned")


def generate(root: Path, action_queue: dict, index: Index,
             assignments: dict[str, str] | None = None) -> dict:
    """Build a source-call review queue from reviewed spans and Mac branches."""
    assignments = assignments or {}
    unfinished = {int(row["retail_va"], 0): row for row in action_queue["rows"]
                  if row.get("windows_max") is None or row["windows_max"] < 100 - 1e-6}
    refs = references.load(root)
    pairs = load_pairs(root)
    by_va = {ref.retail_va: ref for ref in refs if ref.retail_va is not None}
    by_va.update({pair.retail_va: pair for pair in pairs})
    by_target = {(ref.mac_section, ref.mac_offset): ref for ref in refs}
    by_target.update({(pair.mac_section, pair.mac_offset): pair for pair in pairs})
    runtime = tomllib.loads((root / "config/mac/runtime.toml").read_text()).get("functions", [])
    runtime_by_target = {(item["mac_section"], item["mac_offset"]): item["symbol"]
                         for item in runtime}

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
    for va, row in sorted(unfinished.items()):
        caller = by_va.get(va)
        owner = _owner(row.get("unit") or "", assignments)
        deferred = row.get("state") == "deferred"
        entry = {"owner": owner, "unit": row.get("unit"), "retail_va": f"0x{va:08x}",
                 "function": row["function"], "windows_max": row.get("windows_max"),
                 "deferred": deferred,
                 "reviewed_mac_span": caller is not None,
                 "reviewed_calls": 0, "missing_named_calls": 0,
                 "unreviewed_targets": 0,
                 "state": "review_calls" if caller else "pair_mac_address"}
        functions.append(entry)
        if caller is None:
            continue
        try:
            body = _masked_source(extract_body(caller))
            source_error = ""
        except (OSError, ValueError) as exc:
            body = ""
            source_error = str(exc)
        sites = branches.get(caller.mac_section, [])
        lo = bisect_right(starts.get(caller.mac_section, []), caller.mac_offset - 1)
        for at, section, offset in sites[lo:]:
            if at >= caller.mac_offset + caller.mac_size:
                break
            target = by_target.get((section, offset))
            runtime_name = runtime_by_target.get((section, offset), "")
            name = _leaf(target) if target else ""
            source_call = bool(name and body and re.search(r"\b" + re.escape(name) + r"\s*\(", body))
            state = ("runtime_call" if target is None and runtime_name else
                     "identify_target" if target is None else
                     "source_call_present" if source_call else
                     "review_missing_helper_call" if getattr(target, "source_helper", None) else
                     "review_other_named_call")
            calls.append({"owner": owner, "unit": entry["unit"], "retail_va": entry["retail_va"],
                          "deferred": deferred,
                          "function": entry["function"], "mac_call_site": _address(caller.mac_section, at),
                          "mac_target": _address(section, offset),
                          "target_name": name or runtime_name,
                          "target_unit": target.unit if target else "",
                          "source_call_present": source_call, "source_parse_error": source_error,
                          "state": state})
            entry["reviewed_calls"] += target is not None
            entry["missing_named_calls"] += state == "review_missing_helper_call"
            entry["unreviewed_targets"] += state == "identify_target"
    coverage = {"unfinished_windows_functions": len(functions),
                "reviewed_mac_callers": sum(row["reviewed_mac_span"] for row in functions),
                "direct_mac_calls": len(calls),
                "missing_named_source_calls": sum(row["state"] == "review_missing_helper_call" for row in calls),
                "unreviewed_direct_targets": len({row["mac_target"] for row in calls
                                                  if row["state"] == "identify_target"})}
    return {"schema": 1, "scope": "mac_retained_helper_recovery",
            "target_sha256": action_queue.get("target_sha256"),
            "coverage": coverage, "functions": functions, "calls": calls}


def write(root: Path, report: dict) -> None:
    out = root / "build/mac"
    out.mkdir(parents=True, exist_ok=True)
    reports.atomic_text(out / "helper-queue.json", json.dumps(report, indent=2) + "\n")
    for name, fields in (
        ("functions", ("owner", "unit", "retail_va", "function", "windows_max", "deferred",
                       "reviewed_mac_span", "reviewed_calls", "missing_named_calls",
                       "unreviewed_targets", "state")),
        ("calls", ("owner", "unit", "retail_va", "deferred", "mac_call_site", "mac_target",
                   "target_name", "target_unit", "source_call_present", "state")),
    ):
        stream = io.StringIO()
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t", extrasaction="ignore")
        writer.writeheader()
        writer.writerows(report[name])
        reports.atomic_text(out / f"helper-queue-{name}.tsv", stream.getvalue())


def leads(report: dict, unit: str | None = None,
          include_deferred: bool = False) -> list[dict]:
    """Group actionable call leads by destination instead of repeating sites."""
    groups = {}
    for row in report["calls"]:
        if row["state"] not in ("review_missing_helper_call", "identify_target"):
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
        group["callers"].add(row["retail_va"])
        group["units"].add(row["unit"])
    result = []
    for group in groups.values():
        group["caller_count"] = len(group.pop("callers"))
        group["unit_count"] = len(group.pop("units"))
        result.append(group)
    return sorted(result, key=lambda group: (
        group["state"] != "review_missing_helper_call",
        -group["caller_count"], -group["sites"], -group["unit_count"],
        group["mac_target"]))
