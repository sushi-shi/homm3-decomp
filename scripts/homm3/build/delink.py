#!/usr/bin/env python3
"""homm3.build.delink - the delink half of the loop (P2.3).

Run by full `homm3 build`, or directly with `homm3 delink`:

    labels (extraction -> claim fragments) -> model (the one join
        -> build/gen/symbol_names.csv) -> synth_pdb -> data_manifest
        -> vostok-delinker (--reloc-manifest config/retail/relocs.tsv)
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
from pathlib import Path
from dataclasses import dataclass

from homm3 import model
from homm3.build import configure, data_manifest, normalize_objs, synth_pdb
from homm3.core import common
from homm3.retail_labels import source as labels_source

DELINK_DIR = common.HOMM3_DIR / "build/delink"
TARGET_DIR = common.HOMM3_DIR / "build/objdiff/target"


def _prune_normalized(units):
    normalized = common.HOMM3_DIR / 'build/objdiff/normalized'
    for side in ('base', 'target'):
        directory = normalized / side
        expected = set()
        for unit in units:
            name = Path(unit['unit'] + ('.obj' if side == 'base' else '.c.obj'))
            raw = common.HOMM3_DIR / 'build/objdiff' / side / name
            if raw.is_file():
                expected.update((name, name.with_suffix('.symbols.tsv'),
                                 name.with_name(name.name + '.stamp.json')))
        # This directory owns only disposable comparison files. Sweep all
        # unreferenced files, including nested entries and interrupted writes.
        for cached in directory.rglob('*'):
            if cached.is_file() and cached.relative_to(directory) not in expected:
                cached.unlink()


@dataclass(frozen=True)
class DelinkResult:
    inventory: Path
    pdb: Path
    data_manifest: Path
    targets: tuple[Path, ...]
    missing: tuple[str, ...]


def run() -> DelinkResult:
    rc = labels_source.extract()   # src macros -> claim fragments
    if rc:
        raise RuntimeError("source label extraction failed; delinking stopped")
    inventory = model.generate()
    pdb = synth_pdb.generate(inventory)
    data = data_manifest.generate()

    if DELINK_DIR.exists():
        shutil.rmtree(DELINK_DIR)
    subprocess.run(
        ["vostok-delinker",
         "--pdb-path", str(pdb),
         "--exe-path", str(common.resolve_exe()),
         "--output-path", str(DELINK_DIR),
         "--engine-path", "c:\\proj\\",
         "--reloc-manifest", str(common.HOMM3_DIR /
                                 "config/retail/relocs.tsv"),
         "--reloc-alias-manifest", str(common.HOMM3_DIR /
                                       "config/retail/reloc-aliases.tsv"),
         "--data-manifest", str(data)],
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
    expected = {f"{u['unit']}.c.obj" for u in units} - {f'{u}.c.obj' for u in missing}
    for stale in TARGET_DIR.glob('*.c.obj'):
        if stale.name not in expected:
            stale.unlink()
    print(f"[build delink] {copied} unit objects -> {TARGET_DIR}"
          + (f"; missing from delink: {', '.join(missing)}"
             if missing else ""))

    # Preserve only cache entries backed by current raw objects. Normalization
    # verifies complete input/tool/output hashes before reusing any entry.
    normalize_objs.normalize_all()
    _prune_normalized(units)
    configure.configure()
    return DelinkResult(inventory, pdb, data,
                        tuple(TARGET_DIR / name for name in sorted(expected)), tuple(missing))


def main(argv=None) -> int:
    run()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
