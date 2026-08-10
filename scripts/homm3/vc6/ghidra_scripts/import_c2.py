# -*- coding: utf-8 -*-
"""Headless import of the pinned C2.DLL into the vc6 atlas Ghidra project.

Phase 2 substrate (docs/vc6/c2-atlas.md): one Ghidra project under gitignored
build/re/vc6/, holding the auto-analyzed C2.DLL at its preferred base
0x10700000. The subject is HASH-GATED via _toolchain.resolve("C2.DLL") before
any import - a wrong pressing is a hard abort, never a silently wrong atlas.

Idempotent: an existing analyzed project is reused (and re-verified against
the pinned sha256 that Ghidra recorded at import time); --reimport deletes the
project and starts over. Runs in-process via pyghidra - the pattern proven by
scripts/archive/carve/driver.py and scripts/archive/ghidra_recover_structs/
(analyzeHeadless cannot start PyGhidra for .py postScripts in this setup).

Standalone:  python3 scripts/homm3/vc6/ghidra_scripts/import_c2.py [--reimport]
Pipeline:    imported by homm3.vc6.atlas (loaded by file path; no __init__ in
             ghidra_scripts/ by design - these are in-Ghidra headless tools).
"""
from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

if __package__ in (None, ""):  # standalone: put scripts/ on the path
    sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from homm3.vc6 import _common, _toolchain

PROJ_DIR = _common.REPO / "build/re/vc6"
PROJ_NAME = "c2-atlas"
RAW_DIR = PROJ_DIR / "raw"
IMAGE_BASE = 0x10700000


def preflight() -> None:
    if not os.environ.get("GHIDRA_INSTALL_DIR"):
        _common.die("GHIDRA_INSTALL_DIR unset - enter `nix develop .#build`")
    try:
        import pyghidra  # noqa: F401
    except Exception as exc:  # pragma: no cover - environment failure
        _common.die(f"pyghidra not importable ({exc}) - enter `nix develop .#build`")


def open_program(reimport: bool = False, heap: str = "2G"):
    """(gproject, program) for the gated C2.DLL, auto-analyzed and cached.

    The caller owns the pair and must hand it back to close_program().
    """
    preflight()
    c2 = _toolchain.resolve("C2.DLL")  # the hash gate: wrong pressing = abort

    if reimport and PROJ_DIR.exists():
        for stale in sorted(PROJ_DIR.glob(PROJ_NAME + ".*")):
            if stale.is_dir():
                shutil.rmtree(stale)
            else:
                stale.unlink()
        print(f"[atlas] --reimport: removed the {PROJ_NAME} project", flush=True)

    import pyghidra
    if not pyghidra.started():
        launcher = pyghidra.HeadlessPyGhidraLauncher()
        launcher.add_vmargs(f"-Xmx{heap}")
        launcher.start()
    from pyghidra.core import _setup_project, _analyze_program
    from ghidra.program.flatapi import FlatProgramAPI
    from ghidra.program.util import GhidraProgramUtilities

    PROJ_DIR.mkdir(parents=True, exist_ok=True)
    gproject, program = _setup_project(
        binary_path=str(c2), project_location=str(PROJ_DIR),
        project_name=PROJ_NAME, nested_project_location=False)

    # Stale-project gate: the program cached in an existing project must be the
    # pinned bytes (Ghidra records the import-time sha256 on the program).
    want_sha = _toolchain.PINNED["C2.DLL"][0]
    got_sha = str(program.getExecutableSHA256() or "").lower()
    if got_sha and got_sha != want_sha:
        gproject.close()
        _common.die(f"{PROJ_NAME}: cached program sha256 {got_sha} is not the "
                    f"pinned C2.DLL ({want_sha}) - rerun with --reimport")
    base = program.getImageBase().getOffset()
    if base != IMAGE_BASE:
        gproject.close()
        _common.die(f"{PROJ_NAME}: image base 0x{base:x} != 0x{IMAGE_BASE:x}")

    if GhidraProgramUtilities.shouldAskToAnalyze(program):
        print("[atlas] auto-analyzing C2.DLL (a few minutes) ...", flush=True)
        _analyze_program(FlatProgramAPI(program), program)
        # markProgramAnalyzed needs an open transaction to stick across a save;
        # without it the next run re-analyzes from scratch (recover_structs.py).
        tx = program.startTransaction("mark-analyzed")
        try:
            GhidraProgramUtilities.markProgramAnalyzed(program)
        finally:
            program.endTransaction(tx, True)
        _save(gproject, program)
    else:
        print(f"[atlas] reusing analyzed project {PROJ_DIR / (PROJ_NAME + '.gpr')}",
              flush=True)
    return gproject, program


def _save(gproject, program) -> None:
    """Persist the analyzed DB via GhidraProject.save - the only correct
    path here. GhidraProject HOLDS ONE OPEN TRANSACTION per program by
    design (its openPrograms map is literally program->transaction, see
    GhidraProject.java); save() ends that transaction, saves, and restarts
    a fresh one. Saving the DomainFile directly instead fails with 'Unable
    to lock due to active transaction' - the active transaction is
    GhidraProject's own, not a leak (measured 2026-08-09; an earlier note
    in recover_structs.py blaming GhidraProject.save() for no-op'ing led
    this module astray - do not resurrect the DomainFile.save detour).
    A failed save only costs the next run a re-analysis."""
    try:
        gproject.save(program)
    except Exception as exc:  # pragma: no cover
        print(f"[atlas] WARNING: could not persist the analyzed DB ({exc}); "
              "the next run will re-analyze", flush=True)


def close_program(gproject, program) -> None:
    _save(gproject, program)
    gproject.close()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--reimport", action="store_true",
                    help="delete and re-create the Ghidra project")
    args = ap.parse_args(argv)
    gproject, program = open_program(reimport=args.reimport)
    try:
        fm = program.getFunctionManager()
        print(f"[atlas] {PROJ_NAME}: {fm.getFunctionCount()} functions, "
              f"base 0x{program.getImageBase().getOffset():x}")
    finally:
        close_program(gproject, program)
    return 0


if __name__ == "__main__":
    sys.exit(main())
