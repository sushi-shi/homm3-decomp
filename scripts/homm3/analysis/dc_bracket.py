#!/usr/bin/env python3
"""homm3.analysis.dc_bracket - locate DC-only functions by link-order
bracketing.

The problem this solves: 322 of the homm2-traveled functions are
attested only in the Dreamcast build (a CodeView proc offset) with no
proven retail address, so they cannot be claimed or matched. Locating
them one at a time by hand is the bottleneck.

The argument. Within one compiland the linker emits functions in
SOURCE order, and the Dreamcast CodeView proc offsets are in that same
source order (both builds compile the same .cpp). So inside a TU the
DC sequence and the retail sequence are the SAME sequence. Where a TU
already has proven retail addresses (`VA()` claims, the `anchor-*` /
`linkorder` rows of the game tree), those addresses cut the TU into
GAPS. If a gap contains exactly as many DC-only functions as it
contains unclaimed retail functions, the order-preserving map between
them is FORCED - there is only one way to line up two equal-length
ordered runs. That is a proof, not a guess, and it is the same
reasoning the link-order carve already uses at TU granularity.

Grades emitted:
  forced       counts agree - the mapping is the unique order-preserving
               one. NOTE the DC size is an SH4 size and never equals the
               x86 size: `proposed_size` is the RETAIL size from
               config/retail-functions.tsv and is the one a VA() claim
               must carry (a claim built from the DC size truncates the
               body mid-instruction).
  ambiguous    counts disagree (inlined-away, OPT:REF-dropped, or
               compiler-generated functions live in the gap) - reported
               with both runs so a human can adjudicate
  unbracketed  the gap has no closing anchor (TU tail) - no proof

ANALYSIS OUTPUT, not retail evidence: nothing here is admitted until a
independent retail proof promotes a row into a `VA()` claim. The forced rows
are the queue for that review.

  homm3 analysis dc-bracket            write evidence/dc-bracket-map.tsv
  homm3 analysis dc-bracket --unit X   report one TU to stdout
  homm3 analysis dc-bracket --traveled only rows in the homm2 overlap set
"""
from __future__ import annotations

import argparse
import csv
import sys
from collections import defaultdict

from homm3.core import common

GAME_TREE = common.EVIDENCE_DIR / "retail-game-tree.csv"
OVERLAP = common.EVIDENCE_DIR / "homm2-overlap/functions.csv"
FUNCTIONS = common.HOMM3_DIR / "config/retail-functions.tsv"
OUT = common.EVIDENCE_DIR / "dc-bracket-map.tsv"


def read_functions():
    """[(rva, size)] of every carved retail function, ascending."""
    rows = []
    for line in FUNCTIONS.open():
        if line.startswith("#") or line.startswith("rva"):
            continue
        parts = line.split()
        if len(parts) < 2:
            continue
        rows.append((int(parts[0], 16), int(parts[1])))
    return sorted(rows)


def read_game_tree():
    """unit -> [row...] in DC order. A row is a dict with dc offset/size,
    the retail rva/size when proven, and the display name."""
    units = defaultdict(list)
    with GAME_TREE.open(encoding="utf-8", errors="replace") as fh:
        for row in csv.reader(fh):
            if not row or row[0].startswith("#") or len(row) < 8:
                continue
            unit, _src, name = row[0], row[1], row[2]
            dc_off, dc_size, h3_rva, h3_size = row[4], row[5], row[6], row[7]
            if not dc_off.startswith("0x"):
                continue  # a comma inside a signature shifted the row
            try:
                entry = {
                    "unit": unit.replace(".obj", ""),
                    "name": name,
                    "dc": int(dc_off, 16),
                    "dc_size": int(dc_size) if dc_size.isdigit() else 0,
                    "rva": int(h3_rva, 16) if h3_rva.startswith("0x") else None,
                    "rva_size": int(h3_size) if h3_size.isdigit() else 0,
                }
            except ValueError:
                continue
            units[entry["unit"]].append(entry)
    for unit in units:
        units[unit].sort(key=lambda e: e["dc"])
    return units


def traveled_dc_offsets():
    """DC offsets of the homm2-traveled functions still unlocated."""
    wanted = set()
    if not OVERLAP.is_file():
        return wanted
    with OVERLAP.open() as fh:
        for row in csv.DictReader(l for l in fh if not l.startswith("#")):
            try:
                if float(row.get("h2_fuzzy") or 0.0) < 100.0:
                    continue
            except ValueError:
                continue
            if row.get("h3_retail_rva"):
                continue
            off = row.get("h3_dc_offset") or ""
            if off.startswith("0x"):
                wanted.add(int(off, 16))
    return wanted


def bracket(unit_rows, functions):
    """Yield result rows for one TU's DC sequence."""
    anchors = [i for i, e in enumerate(unit_rows) if e["rva"] is not None]
    if len(anchors) < 2:
        return
    claimed = {e["rva"] for e in unit_rows if e["rva"] is not None}
    for left, right in zip(anchors, anchors[1:]):
        gap_dc = unit_rows[left + 1:right]
        if not gap_dc:
            continue
        lo, hi = unit_rows[left]["rva"], unit_rows[right]["rva"]
        if lo is None or hi is None or hi <= lo:
            continue  # a claim out of link order: not our business here
        gap_retail = [(rva, size) for rva, size in functions
                      if lo < rva < hi and rva not in claimed]
        if len(gap_dc) != len(gap_retail):
            for entry in gap_dc:
                yield {**entry, "proposed": None, "proposed_size": 0,
                       "grade": "ambiguous",
                       "detail": f"gap {len(gap_dc)} dc vs "
                                 f"{len(gap_retail)} retail "
                                 f"[0x{lo:x}..0x{hi:x}]"}
            continue
        for entry, (rva, size) in zip(gap_dc, gap_retail):
            yield {**entry, "proposed": rva, "proposed_size": size,
                   "grade": "forced",
                   "detail": f"gap of {len(gap_dc)} "
                             f"[0x{lo:x}..0x{hi:x}]"}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 analysis dc-bracket")
    parser.add_argument("--unit", help="report one TU to stdout")
    parser.add_argument("--traveled", action="store_true",
                        help="only rows in the homm2-traveled set")
    args = parser.parse_args(argv)

    functions = read_functions()
    units = read_game_tree()
    wanted = traveled_dc_offsets() if args.traveled else None

    rows = []
    for unit in sorted(units):
        if args.unit and unit != args.unit:
            continue
        for row in bracket(units[unit], functions):
            if wanted is not None and row["dc"] not in wanted:
                continue
            rows.append(row)

    counts = defaultdict(int)
    for row in rows:
        counts[row["grade"]] += 1

    if args.unit:
        for row in rows:
            proposed = (f"0x{row['proposed']:x}" if row["proposed"]
                        else "-")
            print(f"  {row['grade']:12s} dc 0x{row['dc']:<7x} "
                  f"{proposed:>10s}  {row['name'][:52]:52s} {row['detail']}")
    else:
        OUT.parent.mkdir(parents=True, exist_ok=True)
        with OUT.open("w") as fh:
            fh.write("# generator: homm3.analysis.dc_bracket\n")
            fh.write(f"# exe: HEROES3.EXE sha256={common.TARGET_SHA256}\n")
            fh.write("# ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - a `forced`\n"
                     "# row is a link-order proof proposal; it becomes\n"
                     "# evidence only when independent retail proof promotes it\n"
                     "# into a VA() claim.\n")
            fh.write("unit\tdc_offset\tdc_size\tproposed_rva\t"
                     "proposed_size\tgrade\tname\tdetail\n")
            for row in rows:
                proposed = (f"0x{row['proposed']:x}" if row["proposed"]
                            else "")
                fh.write(f"{row['unit']}\t0x{row['dc']:x}\t{row['dc_size']}\t"
                         f"{proposed}\t{row['proposed_size']}\t{row['grade']}\t"
                         f"{row['name']}\t{row['detail']}\n")
        print(f"[dc_bracket] {len(rows)} rows -> {OUT}")
    for grade in sorted(counts):
        print(f"  {grade:12s} {counts[grade]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
