"""Generate the admitted-function polish queue from banked MAX.

Run `HOMM3_DIR=<worktree> python -m homm3.vc6.weighted_queue` after a full
build. Lowest banked scores rank first, with retail size breaking ties.
HIST remains visible as lost-headroom evidence but does not hide work whose
current implementation has a lower MAX.
"""
from __future__ import annotations

import csv
import json

from homm3.core import common
from homm3.match import status, universe
from homm3.vc6.queue import EXACT, _compiled_functions


def ranked_rows(report, baseline, categories, sizes, compiled):
    peaks = {}
    for row in baseline.values():
        if row.rva is not None:
            maximum, historical = peaks.get(row.rva, (0.0, 0.0))
            peaks[row.rva] = max(maximum, row.max), max(historical, row.hist)

    measured = {}
    for unit in report.get("units", []):
        owner = (unit.get("name") or unit.get("id") or "").split("/")[-1]
        for fn in unit.get("functions", []):
            key = owner, fn["name"]
            if key not in compiled:
                continue
            row = baseline.get(key)
            if row is None or row.rva is None:
                raise ValueError(f"report row lacks checkpoint RVA: {owner}:{fn['name']}")
            current = fn.get("fuzzy_match_percent")
            entry = (float(current) if current is not None else None,
                     owner, fn["name"])
            previous = measured.get(row.rva)
            if previous is None or (entry[0] or 0) > (previous[0] or 0):
                measured[row.rva] = entry

    rows = []
    for rva, (current, owner, name) in measured.items():
        if categories[rva] not in ("target", "zlib"):
            continue
        maximum, historical = peaks[rva]
        if maximum >= EXACT:
            continue
        size = sizes[rva]
        rows.append(dict(
            va=f"0x{rva + common.IMAGE_BASE:08x}", size=size,
            current=current, maximum=maximum, historical=historical,
            remaining_bytes=size * (1 - maximum / 100),
            unit=owner, function=name, state="admitted"))
    rows.sort(key=lambda row: (row["maximum"],
                              -row["size"], row["va"]))
    return rows


def main():
    root = common.HOMM3_DIR
    report = json.loads((root / "build/objdiff/report.json").read_text())
    baseline = status.load_baseline()
    categories, sizes = universe.classify()
    rows = ranked_rows(report, baseline, categories, sizes,
                       _compiled_functions(report))
    output = root / "evidence/weighted-queue.tsv"
    with output.open("w") as stream:
        stream.write("# GENERATED: python -m homm3.vc6.weighted_queue\n")
        stream.write("# Admitted compiled bodies, sorted by increasing current-implementation MAX, then decreasing retail size.\n")
        stream.write("# HIST is retained to expose peaks lost by source edits; it does not exclude a row.\n")
        writer = csv.DictWriter(stream, fieldnames=[
            "va", "size", "current", "maximum", "historical",
            "remaining_bytes", "unit", "function", "state"], delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(f"{len(rows)} remaining functions; "
          f"{sum(r['remaining_bytes'] for r in rows):,.1f} unmatched weighted bytes")
    for row in rows[:20]:
        print(f"{row['va']} {row['size']:6d} B MAX {row['maximum']:8.4f}% "
              f"HIST {row['historical']:8.4f}% "
              f"{row['unit']}:{row['function']}")
    print(f"Wrote {output}")


if __name__ == "__main__":
    main()
