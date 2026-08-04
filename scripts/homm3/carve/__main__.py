#!/usr/bin/env python3
"""python3 -m homm3.carve <stage> - bootstrap carving pipeline dispatch.

Deliberately NOT a `homm3` CLI subcommand: this package retires to
scripts/archive/ once its inventories are admitted (see README.md).

Stages (upstream TSVs under build/carve/ are each stage's inputs):
    intake       S0  hash-gate the exe, stamp target.json
    relocs       S1  reloc-site sweep (vendored channels)
    ghidra       S2  PyGhidra analysis + seed fixpoint + body export
    tables       S4  jump-table attribution (needs relocs + ghidra)
    extents      S3+S6  chunk census + size synthesis
    vtables      S5+S8  vtable runs, cutting, deliverable
    audit        S7  fatal gates + report-only cross-checks
    fixture      COFF-truth assertions (VC6 /O2 /Gy object is ground truth)
    all          fixture -> relocs -> ghidra -> tables -> extents -> vtables -> audit
    emit-config  render admission candidates to build/carve/config-candidate/
"""
from __future__ import annotations

import sys


def main(argv: list[str]) -> int:
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__)
        return 0
    stage, rest = argv[0], argv[1:]

    if stage == "intake":
        from homm3.carve import common
        info = common.intake()
        print(f"[carve intake] {info['path']} OK "
              f"(sha256={info['sha256'][:12]}..., {info['size']} B, "
              f"{len(info['sections'])} sections) -> {common.TARGET_JSON}")
        return 0
    if stage == "relocs":
        from homm3.carve import relocs
        return relocs.main(rest)
    if stage == "emit-relocs":
        from homm3.carve import relocs
        return relocs.emit_manifest(rest)
    if stage == "dna":
        from homm3.carve import dna
        return dna.main(rest)
    if stage == "names":
        from homm3.carve import names
        return names.main(rest)
    if stage == "relate":
        from homm3.carve import relate
        return relate.main(rest)
    if stage == "naming":
        from homm3.carve import naming
        return naming.main(rest)
    if stage == "ghidra":
        from homm3.carve import driver
        return driver.main(rest)
    if stage == "tables":
        from homm3.carve import tables
        return tables.main(rest)
    if stage == "extents":
        from homm3.carve import extents
        return extents.main(rest)
    if stage == "vtables":
        from homm3.carve import vtables
        return vtables.main(rest)
    if stage == "audit":
        from homm3.carve import audit
        return audit.main(rest)
    if stage == "fixture":
        from homm3.carve import fixture
        return fixture.main(rest)
    if stage == "emit-config":
        from homm3.carve import audit
        return audit.main(["--emit-config"] + rest)
    if stage == "all":
        from homm3.carve import (audit, driver, extents, fixture, relocs,
                                 tables, vtables)
        for name, mod, args in (("fixture", fixture, []),
                                ("relocs", relocs, []),
                                ("ghidra", driver, []),
                                ("tables", tables, []),
                                ("extents", extents, []),
                                ("vtables", vtables, []),
                                ("audit", audit, [])):
            print(f"[carve all] === {name} ===", flush=True)
            rc = mod.main(args)
            if rc == tables.RC_RESEED and name == "tables":
                # S4 derived roots for orphan dispatchers: seed + re-export,
                # then attribution must succeed with zero orphans.
                print("[carve all] === ghidra (reseed derived roots) ===",
                      flush=True)
                rc = driver.main(["--no-analyze"]) or mod.main(args)
            if rc:
                print(f"[carve all] stage {name} failed (rc={rc})",
                      file=sys.stderr)
                return rc
        return 0

    print(f"[carve] unknown stage: {stage}\n{__doc__}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
