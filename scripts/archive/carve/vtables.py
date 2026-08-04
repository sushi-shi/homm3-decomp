#!/usr/bin/env python3
"""homm3.carve.vtables - S5 vtable run building/cutting/classification + S8.

Substrate: S1's data-channel `.rdata` sites with exec targets - aligned,
literal-masked dwords. Maximal stride-4 runs of length >=2 are the vtable
signal (buka measured isolated data->code pointers at 0.07 precision vs 0.89
in-run). Deliberately NOT filtered by known function starts: only 63% of real
vtable slots land on Ghidra-known boundaries, and a callback need never be
called directly, so a start-whitelist silently rejects real slots.

Runs are CUT at code-channel imm32 sites that name a run address - a ctor/
dtor storing its vptr marks a vtable START - excluding call/jmp mnemonics
(devirtualized calls through interior slot addresses fabricate phantom
starts). Per piece: >=2 targets at final function entries and 0 interior ->
`vtable`; any interior target -> `switch-data` (case targets are
mid-function by construction); else `unclassified`.

Hard gate: a vtable-classified piece with UNCOVERED targets (in no function
at all) means the S2 seed fixpoint did not converge - fatal.

Accepted limitation: true 1-slot vtables are indistinguishable from
coincidence without RTTI (this game has none) and are not reported.
"""
from __future__ import annotations

import bisect
import sys

from homm3.carve import common

RUNS_TSV = common.CARVE_DIR / "vtable_runs.tsv"
DETAIL_TSV = common.CARVE_DIR / "vtables_detail.tsv"
SLOTS_TSV = common.CARVE_DIR / "vtable_slots.tsv"
VTABLES_TSV = common.CARVE_DIR / "vtables.tsv"


class Extents:
    """entry/interior/uncovered state lookup over the S6 inventory."""

    def __init__(self, rows):
        self.spans = sorted((int(r["rva"], 16),
                             int(r["rva"], 16) + int(r["size"])) for r in rows)
        self.entries = {lo for lo, _hi in self.spans}
        self._lo = [lo for lo, _hi in self.spans]

    def state(self, rva):
        if rva in self.entries:
            return "entry"
        i = bisect.bisect_right(self._lo, rva) - 1
        if i >= 0 and self._lo[i] <= rva < self.spans[i][1]:
            return "interior"
        return "uncovered"


def build_runs(sites):
    """Maximal stride-4 runs (>=2 slots) of .rdata code pointers."""
    slots = {}
    for row in sites:
        if (row["channel"] == "data" and row["detail"] == ".rdata"
                and row["target_class"] in ("code", "code-isolated")):
            slots[int(row["site_rva"], 16)] = int(row["value"], 16)
    runs = []
    current = []
    for rva in sorted(slots):
        if current and rva == current[-1][0] + 4:
            current.append((rva, slots[rva]))
        else:
            if len(current) >= 2:
                runs.append(current)
            current = [(rva, slots[rva])]
    if len(current) >= 2:
        runs.append(current)
    return runs


def cut_sites(sites, base_va):
    """code imm32 -> .rdata VA, excluding call/jmp: vptr-store cut points.

    ctx==imm only: a bracketed (memory) operand is a LOAD from a slot, and
    slot loads name interior addresses, not vtable starts."""
    cuts = {}
    for row in sites:
        if (row["channel"] == "code" and row["detail"] not in ("call", "jmp")
                and row.get("ctx") == "imm"):
            target = int(row["value"], 16) - base_va
            if target % 4 == 0:
                cuts.setdefault(target, []).append(
                    (int(row["site_rva"], 16), row["detail"]))
    return cuts


def split_run(run, cuts):
    """Pieces of one run, cut wherever code names a slot address."""
    pieces = []
    piece = [run[0]]
    evidence = {run[0][0]: cuts.get(run[0][0], [])}
    for rva, target in run[1:]:
        if rva in cuts:
            pieces.append(piece)
            piece = []
            evidence[rva] = cuts[rva]
        piece.append((rva, target))
    pieces.append(piece)
    return pieces, evidence


def main(argv=None) -> int:
    image, _info = common.load_image()
    sites = common.read_tsv(common.need(
        common.CARVE_DIR / "reloc_sites.tsv", "relocs"))
    extents = Extents(common.read_tsv(common.need(
        common.CARVE_DIR / "functions_extended.tsv", "extents")))
    base_va = image.image_base

    runs = build_runs(sites)
    cuts = cut_sites(sites, base_va)

    run_rows, detail_rows, slot_rows, deliverable = [], [], [], []
    uncovered_vtables = []
    counts = {"vtable": 0, "switch-data": 0, "unclassified": 0}
    total_vslots = 0
    for run in runs:
        pieces, evidence = split_run(run, cuts)
        run_rows.append((f"0x{run[0][0]:x}", len(run), len(pieces)))
        for piece in pieces:
            states = [(rva, target, extents.state(target - base_va))
                      for rva, target in piece]
            n_entry = sum(1 for _r, _t, s in states if s == "entry")
            n_interior = sum(1 for _r, _t, s in states if s == "interior")
            n_uncovered = len(states) - n_entry - n_interior
            if n_entry >= 2 and n_interior == 0:
                klass = "vtable"
            elif n_interior:
                klass = "switch-data"
            else:
                klass = "unclassified"
            counts[klass] += 1
            piece_rva = piece[0][0]
            ev = evidence.get(piece_rva, [])
            detail_rows.append((
                f"0x{piece_rva:x}", len(piece), klass, n_entry, n_interior,
                n_uncovered,
                ";".join(f"{m}@0x{s:x}" for s, m in ev[:4]) or "-"))
            for index, (rva, target, state) in enumerate(states):
                slot_rows.append((f"0x{piece_rva:x}", index,
                                  f"0x{target - base_va:x}", state))
            if klass == "vtable":
                deliverable.append((f"0x{piece_rva:x}", len(piece)))
                total_vslots += len(piece)
                if n_uncovered:
                    uncovered_vtables.append(piece_rva)

    common.write_tsv(RUNS_TSV, "homm3.carve.vtables",
                     ["run_rva", "slot_count", "piece_count"], run_rows)
    common.write_tsv(DETAIL_TSV, "homm3.carve.vtables",
                     ["piece_rva", "slot_count", "classification",
                      "entry_targets", "interior_targets", "uncovered_targets",
                      "cut_evidence"], detail_rows)
    common.write_tsv(SLOTS_TSV, "homm3.carve.vtables",
                     ["piece_rva", "slot", "target_rva", "state"], slot_rows)

    if uncovered_vtables:
        common.die(f"{len(uncovered_vtables)} vtable pieces have uncovered "
                   f"targets (first: 0x{uncovered_vtables[0]:x}) - S2 fixpoint"
                   " did not converge")

    common.write_tsv(VTABLES_TSV, "homm3.carve.vtables",
                     ["rva", "function_count"], deliverable,
                     [f"# {len(deliverable)} vtables, {total_vslots} slots"])
    print(f"[carve vtables] {len(runs)} runs -> {len(detail_rows)} pieces: "
          f"{counts['vtable']} vtable ({total_vslots} slots), "
          f"{counts['switch-data']} switch-data, "
          f"{counts['unclassified']} unclassified -> {VTABLES_TSV.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
