#!/usr/bin/env python3
"""homm3.build.normalize_objs - normalize comparison copies of the objs.

Thin driver over homm3.build.canonicalize_data_symbols (the gruntz/homm2
pattern): every object under build/objdiff/base/ and build/objdiff/target/
is canonicalized into build/objdiff/normalized/{base,target}/ with a
`.symbols.tsv` sidecar next to each copy. objdiff will point ONLY at the
normalized copies once the comparison graph lands (P2.3); the raw objects
are never touched. Absent roots are tolerated - the machinery predates its
first full user by design.

Each normalized copy gets a provenance stamp recording the raw object it
came from (homm3.build.normalized_freshness), so consumers of the
disposable copies (`homm3 sema diff`) can refuse a stale one instead of
silently comparing through it. The skip decision uses the SAME verifier
the consumers use (content identity, not mtimes), so a copy this driver
skips is by construction one `homm3 sema diff` accepts - a stale stamp
can never wedge between "build says fresh" and "sema says stale".

Known trade-off: the stamp records data inputs only; a change to the
canonicalizer's own code is not detected. Bump STAMP_SCHEMA (which
invalidates every stamp) when the transform changes behavior.
"""
from __future__ import annotations

import sys
from pathlib import Path

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalized_freshness import freshness_problems, write_stamp
from homm3.core import common

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
            if out.exists() and sidecar.is_file() \
                    and not freshness_problems(out):
                skipped += 1
                continue
            result = canon.canonicalize_coff(obj.read_bytes())
            out.write_bytes(result.data)
            sidecar.write_bytes(canon.sidecar_bytes(result.rows))
            write_stamp(out, {"raw": obj})
            wrote += 1
    print(f"[build normalize_objs] {wrote} normalized, {skipped} fresh "
          f"-> {OBJDIFF / 'normalized'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
