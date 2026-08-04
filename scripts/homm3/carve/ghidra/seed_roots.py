# -*- coding: utf-8 -*-
"""One seed-fixpoint iteration: promote structurally proven roots.

Runs inside Ghidra via pyghidra.ghidra_script (CPython; the harness supplies
currentProgram and a transaction). Promotion is CreateFunctionCmd, gated on
getFunctionAt AND getFunctionContaining both null. Four sources, in order:

  init-array    the unique longest aligned exec-target dword run in `.data`
                (.CRT$XCU merged constructor pointers); hard-fail under 1000
                slots - that means the wrong image or a broken scan.
  vtable-run    stride-4 aligned exec-target runs in `.rdata`; a run promotes
                its uncovered slots only when >=2 targets already sit at
                function entries and 0 land inside a function (interior
                targets mean switch data, never a vtable).
  rdata-scalar  every aligned literal-masked `.rdata` dword -> exec target,
                from S1's reloc_sites.tsv (data channel). Catches EH funclet
                entries and stray callbacks; isolated pointers stay IN - a
                FuncInfo unwind action pointer is isolated by construction.
  data-run      non-init-array `.data` pointer runs: LEDGER ONLY, never
                promoted (writable section, higher false rate; the attempt-1
                gate fired zero times).
  derived-root  entries S4 derived for orphan switch dispatchers (pushed
                function-pointer literals; build/carve/derived_roots.tsv).
                Absent until tables.py first finds an orphan.

Every candidate is appended to seed_log_raw.tsv with its result; the created
count lands in seed_iter.json for the driver's fixpoint test. When anything
was created, analyzeChanges runs so the new bodies' call subtrees get carved
before the next iteration re-scans.
"""
import json
import os

from ghidra.app.cmd.function import CreateFunctionCmd
from ghidra.program.flatapi import FlatProgramAPI

from homm3.carve import common

RUN = os.environ.get("HOMM3_CARVE_RUN", "1")
ITERATION = os.environ.get("HOMM3_CARVE_ITER", "1")
CARVE_DIR = common.CARVE_DIR
LOG_PATH = CARVE_DIR / "ghidra" / "seed_log_raw.tsv"
ITER_PATH = CARVE_DIR / "ghidra" / "seed_iter.json"

prog = currentProgram  # noqa: F821 - injected by the pyghidra script harness
memory = prog.getMemory()
fm = prog.getFunctionManager()
image_base = prog.getImageBase().getOffset()


def address(value):
    return prog.getAddressFactory().getDefaultAddressSpace().getAddress(value)


def read_u32(addr):
    value = memory.getInt(addr)
    return value if value >= 0 else value + 0x100000000


def is_executable_va(value):
    block = memory.getBlock(address(value))
    return block is not None and block.isExecute()


def pointer_runs(block, aligned_only=True):
    """Maximal runs of consecutive exec-target dwords, stride 4, 4-aligned."""
    runs = []
    cursor = block.getStart().getOffset()
    end = block.getEnd().getOffset() + 1
    if aligned_only and cursor & 3:
        cursor += 4 - (cursor & 3)
    while cursor + 4 <= end:
        try:
            target = read_u32(address(cursor))
        except Exception:  # uninitialized tail
            break
        if not is_executable_va(target):
            cursor += 4
            continue
        start = cursor
        targets = []
        while cursor + 4 <= end:
            try:
                target = read_u32(address(cursor))
            except Exception:
                break
            if not is_executable_va(target):
                break
            targets.append(target)
            cursor += 4
        runs.append((start, targets))
        cursor += 4
    return runs


created = {"init-array": 0, "vtable-run": 0, "rdata-scalar": 0}
log_rows = []


def promote(source, site_va, target_va):
    """The gate: only virgin addresses become functions."""
    target_addr = address(target_va)
    if fm.getFunctionAt(target_addr) is not None:
        result = "function-entry"
    elif fm.getFunctionContaining(target_addr) is not None:
        result = "function-interior"
    elif CreateFunctionCmd(target_addr).applyTo(prog):
        result = "seeded"
        created[source] += 1
    else:
        result = "unresolved"
    log_rows.append((ITERATION, source, site_va - image_base,
                     target_va - image_base, result))
    return result


data = memory.getBlock(".data")
rdata = memory.getBlock(".rdata")
if data is None or rdata is None:
    raise RuntimeError("target lacks .data/.rdata blocks")

# --- init-array ---------------------------------------------------------
data_runs = pointer_runs(data)
init_start, init_targets = max(data_runs, key=lambda run: len(run[1]))
if len(init_targets) < 1000:
    raise RuntimeError("longest .data code-pointer run is unexpectedly short: "
                       "%d slots" % len(init_targets))
for index, target in enumerate(init_targets):
    promote("init-array", init_start + index * 4, target)

# --- .rdata vtable runs -------------------------------------------------
for start, targets in pointer_runs(rdata):
    if len(targets) < 2:
        continue
    at_entry = interior = 0
    for target in targets:
        target_addr = address(target)
        if fm.getFunctionAt(target_addr) is not None:
            at_entry += 1
        elif fm.getFunctionContaining(target_addr) is not None:
            interior += 1
    if at_entry >= 2 and interior == 0:
        for index, target in enumerate(targets):
            promote("vtable-run", start + index * 4, target)
    else:
        for index, target in enumerate(targets):
            log_rows.append((ITERATION, "vtable-run", start + index * 4 -
                             image_base, target - image_base, "gated"))

# --- .rdata scalars (literal-masked via S1) -----------------------------
sites = common.read_tsv(CARVE_DIR / "reloc_sites.tsv")
scalar_targets = {}
for row in sites:
    if (row["channel"] == "data" and row["detail"] == ".rdata"
            and row["target_class"] in ("code", "code-isolated")):
        target = int(row["value"], 16)
        scalar_targets.setdefault(target, int(row["site_rva"], 16))
for target, site_rva in sorted(scalar_targets.items()):
    promote("rdata-scalar", image_base + site_rva, target)

# --- S4-derived orphan dispatch owners ----------------------------------
derived_path = CARVE_DIR / "derived_roots.tsv"
if derived_path.is_file():
    created["derived-root"] = 0
    for row in common.read_tsv(derived_path):
        promote("derived-root", image_base + int(row["dispatch_rva"], 16),
                image_base + int(row["entry_rva"], 16))

# --- .data non-init-array runs: ledger only -----------------------------
for start, targets in data_runs:
    if start == init_start or len(targets) < 2:
        continue
    for index, target in enumerate(targets):
        log_rows.append((ITERATION, "data-run", start + index * 4 - image_base,
                         target - image_base, "ledger-only"))

with open(str(LOG_PATH), "a") as fh:
    for iteration, source, site, target, result in log_rows:
        fh.write("%s\t%s\t%s\t0x%x\t0x%x\t%s\n" %
                 (RUN, iteration, source, site, target, result))

total = sum(created.values())
ITER_PATH.write_text(json.dumps({"created": total, "by_source": created}))
print("[seed_roots] iter %s: created %d (init-array %d, vtable-run %d, "
      "rdata-scalar %d)" % (ITERATION, total, created["init-array"],
                            created["vtable-run"], created["rdata-scalar"]))

if total:
    FlatProgramAPI(prog).analyzeChanges(prog)
