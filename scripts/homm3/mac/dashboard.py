"""One view of the Mac target's separate accounting numbers.

The plan keeps these apart on purpose: source correspondence (which source
functions have a Mac address), library coverage, code without identity,
unresolved bytes, full-TU compilation, and byte-match scores over verified
spans only. None of them is folded into another's denominator.

Numbers come from the committed tables and the reports the other commands
generate under build/; a missing report is named with the command that
produces it rather than shown as zero.
"""
from __future__ import annotations

import csv
from collections import Counter
import io
import json
from pathlib import Path

from homm3.mac import tables


def _tsv(path: Path) -> list[dict] | None:
    if not path.is_file():
        return None
    text = "\n".join(line for line in path.read_text().splitlines() if not line.startswith("#"))
    return list(csv.DictReader(io.StringIO(text), delimiter="\t"))


def collect(root: Path) -> dict:
    generated = root / "build/gen/mac"
    result: dict = {}

    parity = _tsv(generated / "parity.tsv")
    result["source_correspondence"] = (
        {"functions": len(parity), **Counter(row["state"] for row in parity)}
        if parity is not None else "run `homm3 mac parity`")

    spans = tables.read_functions(root)
    runtime = tables.read_runtime(root)
    glue = tables.read_glue(root)
    zlib = tables.read_zlib(root)
    result["library_coverage"] = {
        "runtime_labels": len(runtime),
        "runtime_by_owner": dict(Counter(label.owner or "unknown" for label in runtime)),
        "runtime_aliases": len(tables.read_aliases(root)),
        "import_glue": len(glue),
        "vendored_zlib": len(zlib),
        "library_bytes": sum(spans.get(offset, 0) for offset in
                             {label.offset for label in runtime} | {stub.offset for stub in glue}
                             | {row.offset for row in zlib}),
    }

    inventory = generated / "inventory.json"
    if inventory.is_file():
        summary = json.loads(inventory.read_text())
        total = summary["code_section_bytes"]
        result["code_section"] = {
            "bytes": total,
            "percent": {name: round(100.0 * value / total, 2) for name, value in summary["bytes"].items()},
            "unowned_function_rows": summary["unowned_function_rows"],
            "unresolved_gaps": summary["unresolved_gaps"],
        }
    else:
        result["code_section"] = "run `homm3 mac inventory`"
    dispositions = tables.read_dispositions(root)
    result["mac_only_or_absent"] = dict(Counter(value for value, _evidence in dispositions.values()))

    objects = _tsv(root / "build/mac/objects.tsv")
    result["full_tu_objects"] = (dict(Counter(row["state"] for row in objects))
                                 if objects is not None else "run `ninja mac-objects; homm3 mac objects`")
    bodies = _tsv(generated / "claim-bodies.tsv")
    result["claim_bodies"] = (dict(Counter(row["state"] for row in bodies))
                              if bodies is not None else "run `homm3 mac emitted`")

    baseline = root / "config/mac/match_baseline.tsv"
    scores = []
    if baseline.is_file():
        for line in baseline.read_text().splitlines():
            if line and not line.startswith("#"):
                scores.append(float(line.split("\t")[4]))
    result["byte_match"] = {
        "scope": "admitted Mac pairs over verified spans only",
        "pairs": len(scores),
        "exact": sum(score >= 100.0 - 1e-9 for score in scores),
        "mean_cur": round(sum(scores) / len(scores), 3) if scores else None,
    }
    return result


def write(root: Path, result: dict) -> Path:
    path = root / "build/gen/mac/dashboard.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, indent=2) + "\n")
    return path
