"""Turn the action queue into disjoint unit packets for a matching wave."""
from collections import defaultdict
import json
from pathlib import Path

from homm3.mac import reports


def plan(queue: dict, workers: int = 6, units: list[str] | None = None) -> dict:
    if workers < 1:
        raise ValueError("worker count must be positive")
    grouped = defaultdict(list)
    for row in queue["rows"]:
        if row["unit"] and row["state"] != "deferred":
            grouped[row["unit"]].append(row)

    def priority(row):
        return (not row["dispatchable"], (row.get("windows_max") or 0) < 95,
                row["size"], -(row.get("windows_max") or 0), row["retail_va"])
    for rows in grouped.values():
        rows.sort(key=priority)
    if units is not None:
        if len(units) != len(set(units)) or len(units) > workers:
            raise ValueError("select distinct units, at most one per worker")
        if any(unit not in grouped for unit in units):
            raise ValueError("selected unit has no active unfinished task")
        selected = units
    else:
        selected = sorted(grouped, key=lambda unit: (priority(grouped[unit][0]), unit))[:workers]
    packets = []
    for index, unit in enumerate(selected, 1):
        rows = grouped[unit]
        packets.append({
            "worker": index, "unit": unit, "source": rows[0]["source"],
            "phase": "match" if rows[0]["dispatchable"] else "establish_mac_evidence",
            "initial_targets": rows[:3], "remaining_unit_targets": len(rows) - min(3, len(rows)),
            "pair_inventory": f"config/mac/functions/{unit}.toml",
            "unit_profile": f"config/mac/units/{unit}.toml",
            "commands": [f"homm3 mac queue --unit {unit}", f"homm3 build --fast {unit}"],
            "rule": "Establish pairing, shared declarations and fresh byte/call reports before speculative source edits. Stop and report unsupported cases; never infer missing evidence from zero calls.",
        })
    return {"schema": 1, "analysis_sha256": queue["analysis_sha256"],
            "scope": "initial_wave_with_explicit_preparation_tasks",
            "packets": packets,
            "unassigned_tasks": [row for row in queue["rows"] if row["unit"] not in selected],
            "note": "Packets assign source ownership; only dispatchable rows already have current comparison evidence. The action queue retains every deferred, unowned and later-wave function."}


def write(root: Path, report: dict) -> Path:
    path = root / "build/mac/campaign.json"
    reports.atomic_text(path, json.dumps(report, indent=2) + "\n")
    return path
