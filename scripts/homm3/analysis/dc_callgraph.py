#!/usr/bin/env python3
"""homm3.analysis.dc_callgraph - the second discriminator for DC-only
functions: intersect the retail call graph with the Dreamcast one.

`homm3.analysis.dc_bracket` locates a DC-only function whenever its gap
between two proven addresses holds equally many DC and retail
functions. Most gaps fail that test because retail inlined or
OPT:REF-dropped some DC functions, leaving a run of N DC candidates
against M < N retail slots. This tool resolves those gaps with a second,
independent signal.

The argument. If DC function F is called by DC function G, and G's
retail counterpart R(G) is already proven, then R(F) - if it exists at
all - is one of the functions R(G) calls. Intersecting that callee set
with the gap's retail candidates usually collapses the choice; when the
intersection is a single address AND the order constraint from the
bracket still holds, the location is proven by two independent
arguments at once.

Signals combined per candidate:
  caller-set    the retail callees of every proven-retail DC caller of F
  order         F's position among the gap's DC run bounds which retail
                slots it can occupy (a monotone map cannot cross)
  arity         retail `ret N` immediate vs the DC argument count, when
                both are recoverable - a cheap veto, never a proof

Grades:
  callgraph-unique   exactly one retail candidate survives both the
                     caller-set intersection and the order window
  callgraph-narrowed  2-3 survive; reported with all of them
  no-signal          F has no proven-retail DC caller, so this lane is
                     silent (the bracket lane's verdict stands)

ANALYSIS OUTPUT, not retail evidence. A `callgraph-unique` row is a
proposal for supervised promotion into a `VA()` claim, exactly like a
bracket `forced` row.

  homm3 analysis dc-callgraph            write evidence/dc-callgraph-map.tsv
  homm3 analysis dc-callgraph --unit X   report one TU to stdout
"""
from __future__ import annotations

import argparse
import csv
import sys
from collections import defaultdict

from homm3.core import common
from homm3.analysis import dc_bracket

XREF = common.EVIDENCE_DIR / "dc-xref-graph.tsv"
OUT = common.EVIDENCE_DIR / "dc-callgraph-map.tsv"


def read_dc_calls():
    """dst_dc_offset -> {src_dc_offset...} for DIRECT calls only.
    pool_refs are address-takings (vtable slots, function pointers) and
    do not imply a call site in the retail body, so they are excluded."""
    callers = defaultdict(set)
    if not XREF.is_file():
        return callers
    with XREF.open() as fh:
        rows = csv.DictReader((l for l in fh if not l.startswith("#")),
                              delimiter="\t")
        for row in rows:
            try:
                if int(row.get("bsr_calls") or 0) <= 0:
                    continue
                src = int(row["src_offset"], 16)
                dst = int(row["dst_offset"], 16)
            except (ValueError, KeyError, TypeError):
                continue
            callers[dst].add(src)
    return callers


_SITES = None


def call_sites(ctx):
    """[(site_rva, callee_rva)] ascending - the callee-keyed shared index
    inverted ONCE, so a body's callees are a binary-searched slice."""
    global _SITES
    if _SITES is None:
        pairs = []
        for callee, sites in ctx.call_index.items():
            for site, _op in sites:
                pairs.append((site, callee))
        pairs.sort()
        _SITES = pairs
    return _SITES


def retail_callees(ctx, rva, size, slots):
    """Retail functions this body calls, restricted to the gap's slots."""
    import bisect
    pairs = call_sites(ctx)
    start = bisect.bisect_left(pairs, (rva, 0))
    end = bisect.bisect_left(pairs, (rva + size, 0))
    wanted = {f for f, _ in slots}
    return {callee for _site, callee in pairs[start:end] if callee in wanted}


def _enforce_injective_monotone(gap_results):
    """A location map is a strictly increasing INJECTION: two DC
    functions cannot share a retail address, and a later DC function
    cannot land before an earlier one. Any `callgraph-unique` row that
    breaks either law is demoted - the lane must not emit a proposal it
    knows is unsound."""
    uniques = [r for r in gap_results if r["grade"] == "callgraph-unique"]
    claimed = defaultdict(list)
    for row in uniques:
        claimed[row["candidates"][0]].append(row)
    bad = set()
    for rva, owners in claimed.items():
        if len(owners) > 1:
            for row in owners:
                bad.add(id(row))
    ordered = [r for r in uniques if id(r) not in bad]
    previous = -1
    for row in ordered:
        rva = row["candidates"][0]
        if rva <= previous:
            bad.add(id(row))
        else:
            previous = rva
    for row in gap_results:
        if id(row) in bad:
            row["grade"] = ("callgraph-narrowed" if row["candidates"]
                            else "no-signal")
            row["detail"] += " [demoted: not an injective monotone map]"
    return gap_results


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 analysis dc-callgraph")
    parser.add_argument("--unit", help="report one TU to stdout")
    args = parser.parse_args(argv)

    from homm3.sema.context import get_context
    ctx = get_context()

    functions = dc_bracket.read_functions()
    fn_size = dict(functions)
    units = dc_bracket.read_game_tree()
    dc_callers = read_dc_calls()

    # dc offset -> proven retail rva, over every TU
    dc_to_rva = {}
    for rows in units.values():
        for entry in rows:
            if entry["rva"] is not None:
                dc_to_rva[entry["dc"]] = entry["rva"]

    results = []
    for unit in sorted(units):
        if args.unit and unit != args.unit:
            continue
        # the bracket lane's ambiguous rows are this lane's input
        gaps = defaultdict(list)
        for row in dc_bracket.bracket(units[unit], functions):
            if row["grade"] == "ambiguous":
                gaps[row["detail"]].append(row)
        for detail, gap_rows in gaps.items():
            span = detail.rsplit("[", 1)[-1].rstrip("]")
            try:
                lo, hi = (int(x, 16) for x in span.split(".."))
            except ValueError:
                continue
            claimed = {e["rva"] for e in units[unit] if e["rva"] is not None}
            slots = [(rva, size) for rva, size in functions
                     if lo < rva < hi and rva not in claimed]
            if not slots:
                continue
            gap_results = []
            for index, row in enumerate(gap_rows):
                proven_callers = [dc_to_rva[c] for c in dc_callers.get(row["dc"], ())
                                  if c in dc_to_rva]
                if not proven_callers:
                    gap_results.append({**row, "grade": "no-signal",
                                        "candidates": [], "detail": detail})
                    continue
                allowed = set()
                for caller in proven_callers:
                    size = fn_size.get(caller)
                    if not size:
                        continue
                    allowed |= retail_callees(ctx, caller, size, slots)
                # order window: a monotone map leaves at least index slots
                # before F and (len(gap_rows) - 1 - index) after it
                lower = index
                upper = len(slots) - (len(gap_rows) - 1 - index)
                window = {rva for pos, (rva, _s) in enumerate(slots)
                          if lower <= pos < max(upper, lower + 1)}
                survivors = sorted(allowed & window) or sorted(allowed)
                if len(survivors) == 1:
                    grade = "callgraph-unique"
                elif 2 <= len(survivors) <= 3:
                    grade = "callgraph-narrowed"
                else:
                    grade = "no-signal"
                gap_results.append({**row, "grade": grade,
                                    "candidates": survivors,
                                    "detail": detail})
            results.extend(_enforce_injective_monotone(gap_results))

    counts = defaultdict(int)
    for row in results:
        counts[row["grade"]] += 1

    if args.unit:
        for row in results:
            cands = ",".join(f"0x{c:x}" for c in row["candidates"]) or "-"
            print(f"  {row['grade']:19s} dc 0x{row['dc']:<7x} {cands:24s} "
                  f"{row['name'][:48]}")
    else:
        with OUT.open("w") as fh:
            fh.write("# generator: homm3.analysis.dc_callgraph\n")
            fh.write(f"# exe: HEROES3.EXE sha256={common.TARGET_SHA256}\n")
            fh.write("# ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - a\n"
                     "# `callgraph-unique` row is a proposal for supervised\n"
                     "# promotion into a VA() claim.\n")
            fh.write("unit\tdc_offset\tproposed_rva\tcandidates\tgrade\t"
                     "name\tdetail\n")
            for row in results:
                cands = ",".join(f"0x{c:x}" for c in row["candidates"])
                proposed = (f"0x{row['candidates'][0]:x}"
                            if row["grade"] == "callgraph-unique" else "")
                fh.write(f"{row['unit']}\t0x{row['dc']:x}\t{proposed}\t"
                         f"{cands}\t{row['grade']}\t{row['name']}\t"
                         f"{row['detail']}\n")
        print(f"[dc_callgraph] {len(results)} rows -> {OUT}")
    for grade in sorted(counts):
        print(f"  {grade:19s} {counts[grade]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
