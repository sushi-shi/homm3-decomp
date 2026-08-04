# -*- coding: utf-8 -*-
"""Export every executable Ghidra function body -> ghidra_functions.tsv.

body_size is the body address-set CARDINALITY and body_ranges keeps every
discontiguous range (half-open) - downstream S6 synthesizes the interval-span
size itself; exporting a flattened span here would silently bake in Ghidra's
notion of the body (the attempt-1 lesson: consumers read its byte_size as a
span and were subtly wrong on 39 discontiguous rows).
"""
import os

from homm3.carve import common

prog = currentProgram  # noqa: F821 - injected by the pyghidra script harness
fm = prog.getFunctionManager()
memory = prog.getMemory()
image_base = prog.getImageBase().getOffset()

rows = []
for fn in fm.getFunctions(True):
    entry = fn.getEntryPoint()
    block = memory.getBlock(entry)
    if block is None or not block.isExecute():
        continue
    ranges = []
    iterator = fn.getBody().getAddressRanges(True)
    while iterator.hasNext():
        rng = iterator.next()
        lo = rng.getMinAddress().getOffset() - image_base
        hi = rng.getMaxAddress().getOffset() - image_base + 1
        ranges.append("0x%x-0x%x" % (lo, hi))
    rows.append(("0x%x" % (entry.getOffset() - image_base),
                 fn.getBody().getNumAddresses(), len(ranges),
                 ";".join(ranges), fn.getName().replace("\t", "_")))

rows.sort(key=lambda row: int(row[0], 16))
path = common.write_tsv(
    common.CARVE_DIR / "ghidra_functions.tsv", "homm3.carve.ghidra.export_bodies",
    ["entry_rva", "body_size", "chunks", "body_ranges", "name"], rows)
print("[export_bodies] wrote %d executable functions -> %s" % (len(rows), path))
