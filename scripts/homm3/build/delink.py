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

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from dataclasses import dataclass

from homm3 import model
from homm3.build import configure, normalize_objs, synth_pdb
from homm3.delink import data_manifest
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


def run(only_units: list[str] | None = None) -> DelinkResult:
    rc = labels_source.extract(only_units)   # src macros -> claim fragments
    if rc:
        raise RuntimeError("source label extraction failed; delinking stopped")
    # One resolution serves the inventory and the data manifest: nothing in
    # between (aliases, synthetic PDB) feeds back into the claim join.
    inventory, resolved = model.generate_model()
    from homm3.delink import reloc_pairing
    aliases = reloc_pairing.write_aliases(reloc_pairing._STATE) \
        if reloc_pairing._STATE is not None else \
        common.HOMM3_DIR / "config/retail/reloc-aliases.tsv"
    pdb = synth_pdb.generate(inventory)
    data_manifest.generate(resolved)
    data = data_manifest.OUTPUT

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
         "--reloc-alias-manifest", str(aliases),
         "--data-manifest", str(data),
         "--data-section-manifest", str(data_manifest.SECTION_OUTPUT)],
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unit', action='append',
                        help='refresh selected source labels (repeatable); '
                             'shared-header claims remain resolved tree-wide')
    args = parser.parse_args(argv)
    from homm3.core import worktree_lock
    label = " ".join(["homm3 delink", *(f"--unit {unit}" for unit in args.unit or [])])
    with worktree_lock.hold(label):
        run(args.unit)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
