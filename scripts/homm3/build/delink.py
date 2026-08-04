#!/usr/bin/env python3
"""homm3.build.delink - the delink half of the loop (P2.3).

Explicit invocation only, never part of `homm3 build` (the homm2 rule):

    labels -> synth_pdb -> data_manifest
        -> vostok-delinker (--reloc-manifest config/retail-relocs.tsv)
        -> build/delink/<unit>.c.obj (the whole image, ~100 objects)
        -> copy the units.toml-scoped objects to build/objdiff/target/
        -> canonicalize both sides into build/objdiff/normalized/
        -> re-emit build/objdiff/objdiff.json (normalized paths)

After this, objdiff (GUI or objdiff-cli) compares REAL delinked retail
code against our compiled base objs, name-paired per symbol.
"""
from __future__ import annotations

import shutil
import subprocess
import sys

from homm3.build import (configure, data_manifest, labels, normalize_objs,
                         synth_pdb)
from homm3.carve import common

DELINK_DIR = common.HOMM3_DIR / "build/delink"
TARGET_DIR = common.HOMM3_DIR / "build/objdiff/target"
PDB = common.HOMM3_DIR / "build/pdb/HEROES3.pdb"


def main(argv=None) -> int:
    for stage in (labels, synth_pdb, data_manifest):
        rc = stage.main([])
        if rc:
            return rc

    if DELINK_DIR.exists():
        shutil.rmtree(DELINK_DIR)
    subprocess.run(
        ["vostok-delinker",
         "--pdb-path", str(PDB),
         "--exe-path", str(common.resolve_exe()),
         "--output-path", str(DELINK_DIR),
         "--engine-path", "c:\\proj\\",
         "--reloc-manifest", str(common.HOMM3_DIR /
                                 "config/retail-relocs.tsv"),
         "--data-manifest", str(common.HOMM3_DIR /
                                "build/gen/delink_data_manifest.tsv")],
        check=True)

    _build, _profiles, units = configure.load_manifest()
    TARGET_DIR.mkdir(parents=True, exist_ok=True)
    copied, missing = 0, []
    for unit in units:
        name = unit["unit"]
        source = DELINK_DIR / f"{name}.c.obj"
        if source.is_file():
            shutil.copy2(source, TARGET_DIR / f"{name}.c.obj")
            copied += 1
        else:
            missing.append(name)
    print(f"[build delink] {copied} unit objects -> {TARGET_DIR}"
          + (f"; missing from delink: {', '.join(missing)}"
             if missing else ""))

    normalized = common.HOMM3_DIR / "build/objdiff/normalized"
    if normalized.exists():
        shutil.rmtree(normalized)  # stale copies must not outlive a delink
    normalize_objs.main([])
    configure.main()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
