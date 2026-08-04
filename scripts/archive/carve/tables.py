#!/usr/bin/env python3
"""homm3.carve.tables - S4: attribute every jump table to its owning function.

Three channels, because no single one is complete:

  switch     each `jmp [4*reg + TABLE]` (S1's switch channel) is owned by the
             function whose Ghidra body contains the dispatch site; the extent
             comes from re-walking the table with the same rules the vendored
             walker used, additionally bounded by the next function entry.
  back-scan  between a function's claimed end and the next entry, a run of
             aligned dwords pointing BACK into that function is a table whose
             dispatch the linear disassembly desynced past (buka's 70-function
             find - the only channel that catches what Ghidra and the switch
             channel both miss). Requires >=2 entries.
  case-map   a code-channel disp32 from inside F targeting the residual gap
             after F's dword tables is the byte case-map load of a sparse
             switch (VC6 emits `mov r8,[r32+MAP]` before the dispatch; the
             fixture pins the layout). Extent runs to the next entry minus
             trailing padding.

The walkers are pure functions over (blob, offsets) so fixture.py can run
them unchanged on a reloc-applied COFF section - that round-trip against the
compiler's own section sizes is what certifies these rules.

A switch-channel table whose dispatch site lies in no function means S2's
carve is incomplete, not that the table is ownerless. When that happens the
owner entry is derived from an independent site-anchored channel - a
non-call/jmp code imm32 (a pushed/stored function-pointer literal) targeting
uncarved .text at a 16-aligned address before the dispatch - and written to
derived_roots.tsv for S2 to seed (exit code 3 = rerun `ghidra --no-analyze`
then `tables`; `all` does this automatically). An orphan with no such literal
is fatal - it needs investigation, not a silent guess. On this image the four
orphans are byte-indexed message dispatchers whose only callers push their
addresses as callbacks.
"""
from __future__ import annotations

import bisect
import struct
import sys

from homm3.carve import common

TABLES_TSV = common.CARVE_DIR / "jump_tables.tsv"
DERIVED_ROOTS_TSV = common.CARVE_DIR / "derived_roots.tsv"
PAD_BYTES = (0x90, 0xCC)
RC_RESEED = 3


# --- pure core (fixture-shared) -----------------------------------------

def walk_dword_table(blob, base, in_code, known_bases, hi):
    """Entries from `base` until a value stops being a code address, the next
    known table starts, or `hi`. Returns (end_exclusive, entry_count)."""
    cursor = base
    while cursor + 4 <= hi:
        value = struct.unpack_from("<I", blob, cursor)[0]
        if not in_code(value):
            break
        if cursor != base and cursor in known_bases:
            break
        cursor += 4
    return cursor, (cursor - base) // 4


def trailing_padding(blob, lo, hi):
    """Alignment fill (NOP/INT3) immediately before `hi`, not below `lo`."""
    end = hi
    while end > lo and blob[end - 1] in PAD_BYTES:
        end -= 1
    return hi - end


def byte_table_extent(blob, base, hi):
    """A byte case-map runs to the next boundary minus alignment padding."""
    return hi - trailing_padding(blob, base, hi)


def back_scan_run(blob, base, is_owner_target, hi):
    """Aligned dwords from `base` whose values point back into the owner."""
    cursor = base
    while cursor + 4 <= hi:
        value = struct.unpack_from("<I", blob, cursor)[0]
        if not is_owner_target(value):
            break
        cursor += 4
    return cursor, (cursor - base) // 4


# --- retail wiring --------------------------------------------------------

class Bodies:
    """Owner lookup over Ghidra body ranges + entry order."""

    def __init__(self, rows):
        self.entries = sorted(int(r["entry_rva"], 16) for r in rows)
        self.body_end = {}
        ranges = []
        for r in rows:
            entry = int(r["entry_rva"], 16)
            for chunk in r["body_ranges"].split(";"):
                lo, hi = (int(x, 16) for x in chunk.split("-"))
                ranges.append((lo, hi, entry))
            self.body_end[entry] = max(
                int(c.split("-")[1], 16) for c in r["body_ranges"].split(";"))
        ranges.sort()
        self._lo = [r[0] for r in ranges]
        self._ranges = ranges

    def owner_of(self, rva):
        i = bisect.bisect_right(self._lo, rva) - 1
        if i >= 0 and self._ranges[i][0] <= rva < self._ranges[i][1]:
            return self._ranges[i][2]
        return None

    def next_entry(self, rva, default):
        i = bisect.bisect_right(self.entries, rva)
        return self.entries[i] if i < len(self.entries) else default


def switch_channel(sites, base_va):
    """S1's switch tables + the code-channel jmp sites that reference them."""
    table_entries = {}
    for row in sites:
        if row["channel"] == "switch":
            tbl = int(row["detail"].split("@", 1)[1], 16)
            table_entries.setdefault(tbl, []).append(int(row["site_rva"], 16))
    dispatch_of = {}
    for row in sites:
        if (row["channel"] == "code" and row["detail"] == "jmp"
                and int(row["value"], 16) - base_va in table_entries):
            dispatch_of.setdefault(int(row["value"], 16) - base_va,
                                   []).append(int(row["site_rva"], 16))
    return table_entries, dispatch_of


def find_orphans(sites, bodies, base_va):
    """Switch tables whose every dispatch site lies outside all bodies."""
    table_entries, dispatch_of = switch_channel(sites, base_va)
    orphans = []
    for tbl in sorted(table_entries):
        disp = dispatch_of.get(tbl, [])
        if not disp or all(bodies.owner_of(d) is None for d in disp):
            orphans.append((tbl, disp))
    return orphans


def resolve_orphans(orphans, sites, bodies, base_va, text_lo, text_hi):
    """Derive each orphan's owner entry from a pushed/stored code literal.

    The entry must be a non-call/jmp code-channel imm32 target: 16-aligned,
    in no existing body, at or before the dispatch, with no function entry
    between it and the dispatch. Underivable orphans are fatal."""
    literals = {}
    for row in sites:
        if (row["channel"] == "code" and row["detail"] not in ("call", "jmp")
                and row.get("ctx") == "imm"):
            target = int(row["value"], 16) - base_va
            if text_lo <= target < text_hi and target % 16 == 0:
                literals.setdefault(target, []).append(
                    int(row["site_rva"], 16))
    candidates = sorted(literals)

    rows = []
    for tbl, disp in orphans:
        if not disp:
            common.die(f"switch table 0x{tbl:x} has no dispatch site at all - "
                       "S1 switch channel inconsistent")
        dispatch = min(disp)
        i = bisect.bisect_right(candidates, dispatch)
        entry = None
        while i > 0:
            i -= 1
            cand = candidates[i]
            if bodies.owner_of(cand) is not None:
                continue
            if bodies.next_entry(cand, text_hi) <= dispatch:
                break  # a carved function intervenes: no plausible candidate
            entry = cand
            break
        if entry is None:
            common.die(f"orphan switch table 0x{tbl:x} (dispatch "
                       f"0x{dispatch:x}): no code-literal root candidate - "
                       "needs manual investigation")
        rows.append((f"0x{entry:x}", f"0x{tbl:x}", f"0x{dispatch:x}",
                     f"push-literal@{','.join(f'0x{s:x}' for s in literals[entry])}"))
    return rows


def attribute(image, sites, bodies):
    """All three channels -> list of table dicts. Pure of I/O."""
    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    lo, hi = text.rva, text.rva + text.size
    base_va = image.image_base

    def off(rva):
        return rva - lo

    in_text_va = lambda v: base_va + lo <= v < base_va + hi  # noqa: E731

    table_entries, dispatch_of = switch_channel(sites, base_va)
    known_bases = {off(t) for t in table_entries}
    tables = []
    orphans = []
    for tbl in sorted(table_entries):
        disp_sites = dispatch_of.get(tbl, [])
        owners = {bodies.owner_of(d) for d in disp_sites} - {None}
        if not owners:
            orphans.append(tbl)
            continue
        owner = min(owners)
        bound = min(bodies.next_entry(tbl, hi), hi)
        end, count = walk_dword_table(blob, off(tbl), in_text_va,
                                      known_bases, off(bound))
        tables.append({"rva": tbl, "end": lo + end, "count": count,
                       "kind": "dword", "owner": owner,
                       "dispatch": min(disp_sites),
                       "evidence": f"switch-channel;s1_entries="
                                   f"{len(table_entries[tbl])}"})
    if orphans:
        common.die(f"{len(orphans)} switch tables with no owning function "
                   f"(first: 0x{orphans[0]:x}) - S2 carve is incomplete")

    claimed_end = dict(bodies.body_end)
    for t in tables:
        claimed_end[t["owner"]] = max(claimed_end[t["owner"]], t["end"])

    # channel 2: claim-gap back-scan (gap bounded from the claimed END - a
    # sandwiched-table function's gap starts past its own funclets)
    for entry in bodies.entries:
        end = claimed_end.get(entry)
        if end is None:
            continue
        nxt = bodies.next_entry(end - 1, hi)
        start = (end + 3) & ~3
        if start >= nxt or start in known_bases:
            continue
        entry_va, end_va = base_va + entry, base_va + end
        run_end, count = back_scan_run(
            blob, off(start), lambda v: entry_va <= v < end_va, off(nxt))
        if count >= 2:
            tables.append({"rva": start, "end": lo + run_end, "count": count,
                           "kind": "dword", "owner": entry, "dispatch": 0,
                           "evidence": "back-scan"})
            claimed_end[entry] = lo + run_end
            known_bases.add(off(start))

    # channel 3: byte case-maps in the residual gap
    code_refs = {}
    for row in sites:
        if row["channel"] == "code" and row["detail"] not in ("call", "jmp"):
            target = int(row["value"], 16) - base_va
            if lo <= target < hi:
                code_refs.setdefault(target, []).append(
                    int(row["site_rva"], 16))
    for entry in bodies.entries:
        end = claimed_end.get(entry)
        if end is None or end <= bodies.body_end.get(entry, 0):
            continue  # only functions that own tables have a residual gap
        nxt = bodies.next_entry(end - 1, hi)
        for target in sorted(t for t in code_refs if end <= t < nxt):
            ref_sites = [s for s in code_refs[target]
                         if bodies.owner_of(s) == entry]
            if not ref_sites:
                continue
            map_end = lo + byte_table_extent(blob, off(target), off(nxt))
            if map_end <= target:
                continue
            tables.append({"rva": target, "end": map_end,
                           "count": map_end - target, "kind": "byte",
                           "owner": entry, "dispatch": min(ref_sites),
                           "evidence": "case-map-ref"})
            claimed_end[entry] = max(claimed_end[entry], map_end)
            break

    tables.sort(key=lambda t: t["rva"])
    return tables


def main(argv=None) -> int:
    image, _info = common.load_image()
    sites = common.read_tsv(common.need(
        common.CARVE_DIR / "reloc_sites.tsv", "relocs"))
    bodies = Bodies(common.read_tsv(common.need(
        common.CARVE_DIR / "ghidra_functions.tsv", "ghidra")))
    text = next(s for s in image.sections if s.name == ".text")

    orphans = find_orphans(sites, bodies, image.image_base)
    if orphans:
        rows = resolve_orphans(orphans, sites, bodies, image.image_base,
                               text.rva, text.rva + text.size)
        common.write_tsv(DERIVED_ROOTS_TSV, "homm3.carve.tables",
                         ["entry_rva", "table_rva", "dispatch_rva", "evidence"],
                         rows)
        print(f"[carve tables] {len(rows)} orphan dispatch owners derived -> "
              f"{DERIVED_ROOTS_TSV.name}")
        print("[carve tables] rerun `ghidra --no-analyze` to seed them, then "
              "`tables` (rc=3; `all` does this automatically)")
        return RC_RESEED

    tables = attribute(image, sites, bodies)

    rows = [(f"0x{t['rva']:x}", t["end"] - t["rva"], t["count"], t["kind"],
             f"0x{t['owner']:x}",
             f"0x{t['dispatch']:x}" if t["dispatch"] else "-",
             t["evidence"]) for t in tables]
    common.write_tsv(TABLES_TSV, "homm3.carve.tables",
                     ["table_rva", "size", "entry_count", "kind", "owner_rva",
                      "dispatch_rva", "evidence"], rows)

    by_kind = {}
    for t in tables:
        key = (t["kind"], t["evidence"].split(";")[0])
        by_kind[key] = by_kind.get(key, 0) + 1
    total_bytes = sum(t["end"] - t["rva"] for t in tables)
    print(f"[carve tables] {len(tables)} tables, {total_bytes} bytes -> "
          f"{TABLES_TSV.name}")
    for (kind, ev), n in sorted(by_kind.items()):
        print(f"  {kind}/{ev}: {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
