#!/usr/bin/env python3
"""homm3.carve.extents - S3 chunk census + S6 extent synthesis.

size = (max end over {body ranges U attributed tables}) - entry_rva: the
interval SPAN, including internal gaps, excluding trailing padding. Falling
back to Ghidra's body size anywhere would reinstate exactly the clipping
defect this package exists to fix.

chunks>1 is analysis error, not a real hole - VC6 emits contiguous functions
and the gaps hold the function's own EH blocks. Two structural sub-cases are
resolved automatically, everything else fatal:

  nested-funclet  a `Catch@`/`Unwind@` entry contained in a chunked parent's
                  span is the parent's own catch handler, emitted inside the
                  parent's COMDAT contribution (VC6 x86 EH); the funclet row
                  is merged into the parent and dropped.
  stray-chunk     a body range separated from the entry range by a gap that
                  contains a NON-funclet entry is a Ghidra shared-tail
                  artifact (the attempt-1 "39 discontiguous rows" defect);
                  the stray range is dropped from the span, flagged, and
                  reported for review.
  table-sandwich  a function whose attributed jump table lies PAST other
                  entries absorbs every function contained in
                  [entry, table_end): COMDAT contributions are contiguous, so
                  bytes between a function and its own table are that
                  function's bytes - VC6 places catch funclets between the
                  main body and the trailing tables, and Ghidra carves the
                  larger funclets as ordinary FUN_ entries.

Remaining overlaps are fatal: they mean a defect no structural rule explains,
so they need investigation here - there is no override side-channel. Once the
inventories are admitted, the config/ copies are manually owned and any later
boundary correction is edited directly into them (audit.py stamps that
contract into the emitted candidates).

Emits functions_extended.tsv (rich truth). The bare functions.tsv deliverable
is rendered by audit.py only when S7 is green.
"""
from __future__ import annotations

import sys

from homm3.carve import common

EXTENDED_TSV = common.CARVE_DIR / "functions_extended.tsv"
CENSUS_TSV = common.CARVE_DIR / "chunk_census.tsv"
FUNCLET_PREFIXES = ("Catch@", "Unwind@")


def load_functions():
    rows = common.read_tsv(common.need(
        common.CARVE_DIR / "ghidra_functions.tsv", "ghidra"))
    funcs = {}
    for r in rows:
        entry = int(r["entry_rva"], 16)
        ranges = sorted(tuple(int(x, 16) for x in c.split("-"))
                        for c in r["body_ranges"].split(";"))
        funcs[entry] = {"entry": entry, "ranges": ranges,
                        "body_size": int(r["body_size"]),
                        "name": r["name"], "flags": [], "tables": []}
    return funcs


def attach_tables(funcs):
    for r in common.read_tsv(common.need(
            common.CARVE_DIR / "jump_tables.tsv", "tables")):
        owner = int(r["owner_rva"], 16)
        if owner not in funcs:
            common.die(f"jump_tables owner 0x{owner:x} not in ghidra_functions"
                       " - stale S4 output, rerun `tables`")
        funcs[owner]["tables"].append(
            (int(r["table_rva"], 16), int(r["table_rva"], 16) + int(r["size"])))


def drop_stray_chunks(funcs, dropped):
    """Rule: a non-entry range whose gap to the previous kept range contains a
    non-funclet entry is a shared-tail artifact, not function bytes."""
    entries = sorted(funcs)

    def nonfunclet_entry_in(lo, hi):
        import bisect
        i = bisect.bisect_left(entries, lo)
        while i < len(entries) and entries[i] < hi:
            if not funcs[entries[i]]["name"].startswith(FUNCLET_PREFIXES):
                return entries[i]
            i += 1
        return None

    for fn in funcs.values():
        kept = [fn["ranges"][0]]
        for rng in fn["ranges"][1:]:
            blocker = nonfunclet_entry_in(kept[-1][1], rng[0])
            if blocker is None:
                kept.append(rng)
                continue
            fn["flags"].append(f"stray-chunk-dropped:0x{rng[0]:x}-0x{rng[1]:x}")
            dropped.append(
                f"0x{fn['entry']:x}: dropped ghidra range 0x{rng[0]:x}-"
                f"0x{rng[1]:x} (lies past non-funclet entry 0x{blocker:x}; "
                "shared-tail artifact)")
        fn["ranges"] = kept


def merge_nested_funclets(funcs):
    """Catch handlers inside a chunked parent's span belong to the parent's
    COMDAT contribution; their separate rows would break the partition."""
    merged = []
    entries = sorted(funcs)
    for entry in entries:
        fn = funcs.get(entry)
        if fn is None or len(fn["ranges"]) < 2:
            continue
        span_end = max(hi for _lo, hi in fn["ranges"])
        for other in [e for e in entries if entry < e < span_end]:
            of = funcs.get(other)
            if of is None or not of["name"].startswith(FUNCLET_PREFIXES):
                continue
            if max(hi for _lo, hi in of["ranges"]) <= span_end:
                fn["flags"].append(f"merged-funclet:0x{other:x}")
                fn["body_size"] += of["body_size"]
                merged.append(other)
                del funcs[other]
    return merged


def merge_table_sandwiched(funcs):
    """Functions contained between an owner's body and its own jump table are
    the owner's EH funclets (contribution contiguity); absorb them."""
    merged = []
    for entry in sorted(funcs):
        fn = funcs.get(entry)
        if fn is None or not fn["tables"]:
            continue
        table_end = max(hi for _lo, hi in fn["tables"])
        for other in [e for e in sorted(funcs) if entry < e < table_end]:
            of = funcs[other]
            other_end = max(hi for _lo, hi in of["ranges"])
            if other_end > table_end:
                common.die(f"function 0x{other:x} straddles 0x{entry:x}'s "
                           f"table end 0x{table_end:x} - needs manual "
                           "investigation")
            fn["flags"].append(f"merged-sandwiched:0x{other:x}")
            fn["body_size"] += of["body_size"]
            fn["ranges"].extend(of["ranges"])
            merged.append(other)
            del funcs[other]
    return merged


def synthesize(funcs):
    """Extent per function; then the fatal zero-overlap sweep."""
    rows = []
    for entry in sorted(funcs):
        fn = funcs[entry]
        ends = [hi for _lo, hi in fn["ranges"]] + [hi for _lo, hi in fn["tables"]]
        extent = max(ends)
        table_bytes = sum(hi - lo for lo, hi in fn["tables"])
        size = extent - entry
        gap = size - fn["body_size"] - table_bytes
        fn["extent"] = extent
        rows.append((entry, size, fn))
        if gap < 0 and not fn["flags"]:
            # tables inside the body would double-count; report, not fatal
            fn["flags"].append("table-within-body")

    overlaps = []
    prev_entry, prev_end = None, 0
    for entry, size, fn in rows:
        if entry < prev_end:
            overlaps.append((prev_entry, prev_end, entry))
        if entry + size > prev_end:
            prev_entry, prev_end = entry, entry + size
    return rows, overlaps


def main(argv=None) -> int:
    funcs = load_functions()
    attach_tables(funcs)

    # S3 census over the raw Ghidra bodies (pre-resolution truth)
    census = []
    for entry in sorted(funcs):
        fn = funcs[entry]
        if len(fn["ranges"]) < 2:
            continue
        span = max(hi for _lo, hi in fn["ranges"]) - entry
        gaps = [b_lo - a_hi for (_a, a_hi), (b_lo, _b) in
                zip(fn["ranges"], fn["ranges"][1:])]
        census.append((f"0x{entry:x}", fn["body_size"], len(fn["ranges"]),
                       span, sum(gaps), max(gaps)))
    common.write_tsv(CENSUS_TSV, "homm3.carve.extents",
                     ["entry_rva", "body_size", "chunks", "span_extent",
                      "total_gap", "max_gap"], census)

    dropped = []
    drop_stray_chunks(funcs, dropped)
    merged = merge_nested_funclets(funcs)
    sandwiched = merge_table_sandwiched(funcs)
    rows, overlaps = synthesize(funcs)

    for line in dropped:
        print(f"[carve extents] {line}")

    if overlaps:
        for prev_entry, prev_end, entry in overlaps[:20]:
            print(f"[carve extents] OVERLAP: 0x{prev_entry:x} extends to "
                  f"0x{prev_end:x}, past entry 0x{entry:x}", file=sys.stderr)
        common.die(f"{len(overlaps)} extent overlaps remain - a defect no "
                   "structural rule explains; investigate before delivering")

    out = []
    for entry, size, fn in rows:
        table_bytes = sum(hi - lo for lo, hi in fn["tables"])
        out.append((f"0x{entry:x}", size, fn["body_size"], len(fn["ranges"]),
                    table_bytes, size - fn["body_size"] - table_bytes,
                    ",".join(fn["flags"]) or "-"))
    common.write_tsv(EXTENDED_TSV, "homm3.carve.extents",
                     ["rva", "size", "body_size", "chunks", "table_bytes",
                      "gap_bytes", "flags"], out)

    total = sum(r[1] for r in rows)
    print(f"[carve extents] {len(rows)} functions, {total} bytes spanned "
          f"({len(census)} multi-chunk, {len(merged)} funclets merged, "
          f"{len(sandwiched)} table-sandwiched merged, "
          f"{len(dropped)} stray chunks dropped) -> {EXTENDED_TSV.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
