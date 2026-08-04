#!/usr/bin/env python3
"""homm3.carve.driver - S2: cached PyGhidra analysis + root-seed fixpoint.

Ghidra's rooted auto-analysis follows the WinMain call tree; on this
relocation-free image it never types `.rdata` dwords as pointers, so methods
reachable only through vtable slots (and EH funclet entries, and `.CRT$XCU`
initializers) are never carved. The fix is a seed FIXPOINT: promote roots the
structural scans prove, analyzeChanges so their call subtrees get carved, and
repeat until an iteration creates nothing (new functions can validate more
vtable runs, which promote more roots).

Boots PyGhidra in-process (analyzeHeadless cannot start PyGhidra for .py
scripts on Ghidra 12.0.4 - the attempt-1 driver pattern). Sole non-default
analyzer option: Aggressive Instruction Finder, which disassembles unreferenced
gaps at ~3-4x analysis cost, paid once - the project under build/carve/ghidra/
is cached and sha-stamped, so `--no-analyze` re-seeds/re-exports instantly.

Imports the REAL gated exe (Ghidra accepts the header-resident load config
that llvm-objdump rejects; the sanitized copy is only for S1).

seed_log.tsv is the audit trail of the PROJECT's construction, so the raw log
is append-only across driver runs (each run gets a fresh `run` id): the run
that created a root logs it as `seeded`, and a later no-op rerun logging the
same candidate as `function-entry` must never overwrite that history.
"""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path

from homm3.carve import common

PROJ_DIR = common.CARVE_DIR / "ghidra"
PROJ_NAME = "carve"
TARGET_STAMP = PROJ_DIR / "target.sha256"
SEED_ITER_JSON = PROJ_DIR / "seed_iter.json"
SEED_LOG_RAW = PROJ_DIR / "seed_log_raw.tsv"
SEED_LOG = common.CARVE_DIR / "seed_log.tsv"
FUNCTIONS_TSV = common.CARVE_DIR / "ghidra_functions.tsv"
SCRIPTS_DIR = Path(__file__).resolve().parent / "ghidra"
SEED_ROOTS = SCRIPTS_DIR / "seed_roots.py"
EXPORT_BODIES = SCRIPTS_DIR / "export_bodies.py"
MAX_ITERATIONS = 12


def _project_exists(sha: str) -> bool:
    return ((PROJ_DIR / f"{PROJ_NAME}.rep").is_dir() and TARGET_STAMP.is_file()
            and TARGET_STAMP.read_text().strip() == sha)


def main(argv=None) -> int:
    argv = list(argv or [])
    info = common.intake()
    common.need(common.CARVE_DIR / "reloc_sites.tsv", "relocs")
    if not os.environ.get("GHIDRA_INSTALL_DIR"):
        common.die("GHIDRA_INSTALL_DIR unset - enter the dev shell (`nix develop`)")
    try:
        import pyghidra
    except Exception as e:  # noqa: BLE001 - report whatever kept it out
        common.die(f"pyghidra not importable ({e}) - enter the dev shell")
    if ((PROJ_DIR / f"{PROJ_NAME}.rep").is_dir() and TARGET_STAMP.is_file()
            and TARGET_STAMP.read_text().strip() != info["sha256"]):
        common.die("cached Ghidra project belongs to a different executable; "
                   f"remove {PROJ_DIR}/{PROJ_NAME}.{{gpr,rep}} first")

    if "--no-analyze" in argv:
        analyze = False
    elif "--analyze" in argv:
        analyze = True
    else:
        analyze = not _project_exists(info["sha256"])
    if analyze:
        print("[carve ghidra] importing + auto-analyzing HEROES3.EXE with "
              "Aggressive Instruction Finder (tens of minutes, one-time) ...",
              flush=True)
    else:
        print("[carve ghidra] reusing analyzed project ...", flush=True)

    PROJ_DIR.mkdir(parents=True, exist_ok=True)
    run_id = 1
    if SEED_LOG_RAW.is_file():
        run_id = 1 + max((int(line.split("\t", 1)[0])
                          for line in SEED_LOG_RAW.read_text().splitlines()
                          if line), default=0)
    os.environ["HOMM3_CARVE_RUN"] = str(run_id)
    os.environ["HOMM3_CARVE_DIR"] = str(common.CARVE_DIR)

    pyghidra.start()
    from pyghidra.core import _setup_project, _analyze_program
    from ghidra.app.script import GhidraScriptUtil
    from ghidra.program.flatapi import FlatProgramAPI
    from ghidra.program.model.listing import Program

    gproject, program = _setup_project(
        binary_path=info["path"], project_location=str(PROJ_DIR),
        project_name=PROJ_NAME, nested_project_location=False)
    project = gproject.getProject()

    GhidraScriptUtil.acquireBundleHostReference()
    try:
        if analyze:
            opts = program.getOptions(Program.ANALYSIS_PROPERTIES)
            tx = program.startTransaction("enable-aggressive-instruction-finder")
            try:
                opts.setBoolean("Aggressive Instruction Finder", True)
            finally:
                program.endTransaction(tx, True)
            _analyze_program(FlatProgramAPI(program), program)

        total_created = 0
        for iteration in range(1, MAX_ITERATIONS + 1):
            os.environ["HOMM3_CARVE_ITER"] = str(iteration)
            print(f"[carve ghidra] seed fixpoint iteration {iteration} ...",
                  flush=True)
            pyghidra.ghidra_script(str(SEED_ROOTS), project, program=program)
            created = json.loads(SEED_ITER_JSON.read_text())["created"]
            total_created += created
            print(f"[carve ghidra]   iteration {iteration}: "
                  f"{created} functions created", flush=True)
            if created == 0:
                break
        else:
            common.die(f"seed fixpoint did not converge in {MAX_ITERATIONS} "
                       "iterations")

        pyghidra.ghidra_script(str(EXPORT_BODIES), project, program=program)
    finally:
        GhidraScriptUtil.releaseBundleHostReference()
        gproject.save(program)
        gproject.close()
        TARGET_STAMP.write_text(info["sha256"] + "\n")

    common.write_tsv(
        SEED_LOG, "homm3.carve.driver",
        ["run", "iter", "source", "site_rva", "target_rva", "result"],
        (line.split("\t") for line in
         SEED_LOG_RAW.read_text().splitlines()) if SEED_LOG_RAW.is_file()
        else [],
        ["# append-only across runs: `seeded` rows are the roots this "
         "project actually gained, in the run that gained them"])
    n = sum(1 for line in FUNCTIONS_TSV.read_text().splitlines()
            if line and not line.startswith("#")) - 1
    print(f"[carve ghidra] done - {n} functions ({total_created} seeded this "
          f"run) -> {FUNCTIONS_TSV.name}, {SEED_LOG.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
