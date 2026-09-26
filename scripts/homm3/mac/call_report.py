"""Mac call-site reports for the scored full-TU pairs."""
from __future__ import annotations

import hashlib
from pathlib import Path

from homm3.core import inputs
from homm3.mac import calls, reports


def analysis_hash(root: Path) -> str:
    paths = sorted((root / "config/mac").rglob("*.toml")) + sorted(path for path in (root / "config/mac").glob("*.tsv") if path.name != "match_baseline.tsv")
    paths += sorted(path for path in (root / "scripts/homm3/mac").glob("*.py")
                    if not path.name.startswith("test_"))
    digest = hashlib.sha256()
    for path in paths:
        digest.update(str(path.relative_to(root)).encode() + b"\0" + path.read_bytes() + b"\0")
    return digest.hexdigest()


def write(root: Path, rows: list[dict], *, units: list[str] | None = None) -> dict:
    report = {"schema": 2, "scope": "recorded_admitted_function_observations", "refreshed_units": units,
              "target_sha256": inputs.MAC.sha256,
              "analysis_sha256": analysis_hash(root),
              "pairs": list(rows),
              "freshness": "Queue verifies current inputs for every observation; other units retain their original provenance."}
    out = root / "build/mac"
    def finish(merged):
        merged["reported_pairs"] = len(merged["pairs"])
        merged["totals"] = calls.totals([row["calls"] for row in merged["pairs"]])
        reports.atomic_text(out / "calls.tsv", _tsv(merged["pairs"]))
    return reports.publish(out / "calls.json", report, units=units, finish=finish)


def _tsv(rows):
    header = ["windows_va", "unit", "function", "retail_calls", "candidate_calls",
              "delta", "retail_direct", "candidate_direct", "retail_indirect",
              "candidate_indirect", "state"]
    lines = ["# Generated call-site census; scored pairs only.",
             "\t".join(header)]
    for row in rows:
        comparison = row["calls"]
        retail, candidate = comparison["retail"], comparison["candidate"]
        values = [row["retail_va"], row["unit"], row["signature"], retail["total"],
                  candidate["total"] if candidate else "", comparison["delta"] if candidate else "",
                  retail["direct"], candidate["direct"] if candidate else "",
                  retail["indirect"], candidate["indirect"] if candidate else "", comparison["state"]]
        lines.append("\t".join(str(value).replace("\t", " ").replace("\n", " ") for value in values))
    return "\n".join(lines) + "\n"
