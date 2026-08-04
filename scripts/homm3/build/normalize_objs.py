#!/usr/bin/env python3
"""homm3.build.normalize_objs - normalize comparison copies of the objs.

Thin driver over homm3.build.canonicalize_data_symbols (the gruntz/homm2
pattern): every object under build/objdiff/base/ and build/objdiff/target/
is canonicalized into build/objdiff/normalized/{base,target}/ with a
`.symbols.tsv` sidecar next to each copy. objdiff will point ONLY at the
normalized copies once the comparison graph lands (P2.3); the raw objects
are never touched. Absent roots are tolerated - the machinery predates its
first full user by design.
"""
from __future__ import annotations

import sys
from pathlib import Path

from homm3.build import canonicalize_data_symbols as canon
from homm3.carve import common

OBJDIFF = common.HOMM3_DIR / "build/objdiff"


def main(argv=None) -> int:
    argv = list(argv or [])
    wrote = skipped = 0
    for side in ("base", "target"):
        root = OBJDIFF / side
        out_root = OBJDIFF / "normalized" / side
        if not root.is_dir():
            continue
        for obj in sorted(root.rglob("*.obj")):
            rel = obj.relative_to(root)
            out = out_root / rel
            sidecar = out.with_suffix(".symbols.tsv")
            out.parent.mkdir(parents=True, exist_ok=True)
            if out.exists() and out.stat().st_mtime >= obj.stat().st_mtime:
                skipped += 1
                continue
            result = canon.canonicalize_coff(obj.read_bytes())
            out.write_bytes(result.data)
            sidecar.write_bytes(canon.sidecar_bytes(result.rows))
            wrote += 1
    print(f"[build normalize_objs] {wrote} normalized, {skipped} fresh "
          f"-> {OBJDIFF / 'normalized'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
