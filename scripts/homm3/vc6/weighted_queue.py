"""This lane's complete remaining-work queue, derived from the current build.

Run `HOMM3_DIR=<worktree> python -m homm3.vc6.weighted_queue` after a full
build. Start with zero/unscored functions, largest first, then increasing
current score with remaining bytes breaking ties. Historical peaks remain
visible but do not hide functions whose current code is still non-exact.
"""
from __future__ import annotations

import csv
import json

from homm3.core import common
from homm3.match import status, universe


def ranked_rows(report, baseline, categories, sizes, labels):
    measured = {}
    for unit in report.get("units", []):
        owner = (unit.get("name") or unit.get("id") or "").split("/")[-1]
        for fn in unit.get("functions", []):
            row = baseline.get((owner, fn["name"]))
            if row is None or row.rva is None:
                raise ValueError(f"report row lacks checkpoint RVA: {owner}:{fn['name']}")
            current = fn.get("fuzzy_match_percent")
            entry = (float(current) if current is not None else None,
                     row.max, row.hist, owner, fn["name"])
            previous = measured.get(row.rva)
            if previous is None or (entry[0] or 0) > (previous[0] or 0):
                measured[row.rva] = entry

    rows = []
    for rva, size in sizes.items():
        if categories[rva] not in ("target", "zlib"):
            continue
        label = labels.get(rva, {})
        current, maximum, historical, owner, name = measured.get(
            rva, (None, 0.0, 0.0, label.get("unit", ""), label.get("name", "")))
        if current is not None and current >= 100.0:
            continue
        rows.append(dict(
            va=f"0x{rva + common.IMAGE_BASE:08x}", size=size,
            current=current, maximum=maximum, historical=historical,
            remaining_bytes=size * (1 - (current or 0) / 100),
            unit=owner, function=name,
            state="unscored" if current is None else "non-exact"))
    rows.sort(key=lambda row: (row["current"] or 0,
                              -row["remaining_bytes"], row["va"]))
    return rows


def main():
    root = common.HOMM3_DIR
    report = json.loads((root / "build/objdiff/report.json").read_text())
    baseline = status.load_baseline()
    categories, sizes = universe.classify()
    with (root / "build/gen/symbol_names.csv").open() as stream:
        labels = {int(row["rva"], 16): row for row in csv.DictReader(
            line for line in stream if not line.startswith("#"))
            if row["kind"] == "func"}
    rows = ranked_rows(report, baseline, categories, sizes, labels)
    output = root / "evidence/weighted-queue.tsv"
    with output.open("w") as stream:
        stream.write("# GENERATED: python -m homm3.vc6.weighted_queue\n")
        stream.write("# Sorted by increasing current score, then decreasing unmatched bytes.\n")
        stream.write("# Unscored rows rank at zero; MAX/history never hide current residuals.\n")
        writer = csv.DictWriter(stream, fieldnames=[
            "va", "size", "current", "maximum", "historical",
            "remaining_bytes", "unit", "function", "state"], delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(f"{len(rows)} remaining functions; "
          f"{sum(r['remaining_bytes'] for r in rows):,.1f} unmatched weighted bytes")
    for row in rows[:20]:
        score = "unscored" if row["current"] is None else f"{row['current']:.4f}%"
        print(f"{row['va']} {row['size']:6d} B {score:>10} "
              f"{row['unit']}:{row['function']}")
    print(f"Wrote {output}")


if __name__ == "__main__":
    main()
