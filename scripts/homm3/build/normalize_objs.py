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

import struct
import sys
from pathlib import Path

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalized_freshness import freshness_problems, write_stamp
from homm3.core import common

OBJDIFF = common.HOMM3_DIR / "build/objdiff"

CNT_CODE = 0x00000020


def _drop_data_sections(payload: bytes) -> bytes:
    """Truncate every non-code section in a comparison copy to zero.

    The matching scope is FUNCTIONS ONLY for now (user decision
    2026-08-06): data comparison returns later as its own phase. The
    raw base/delinked objects keep their data sections untouched -
    only the disposable objdiff copies are scoped, so flipping this
    call back re-admits data wholesale. Section headers stay in place
    (no renumbering); raw size and relocation count drop to zero."""
    data = bytearray(payload)
    nsec, = struct.unpack_from("<H", data, 2)
    dropped = set()
    for index in range(nsec):
        offset = 20 + index * 40
        characteristics, = struct.unpack_from("<I", data, offset + 36)
        if characteristics & CNT_CODE:
            continue
        dropped.add(index + 1)
        struct.pack_into("<I", data, offset + 16, 0)   # SizeOfRawData
        struct.pack_into("<H", data, offset + 32, 0)   # NumberOfRelocations
    # Symbols defined in a dropped section become undefined externs in
    # the copy - .text relocations keep resolving them by name, and the
    # differ no longer sees extents pointing past the emptied section.
    symoff, nsyms = struct.unpack_from("<II", data, 8)
    o, i = symoff, 0
    while i < nsyms:
        section, = struct.unpack_from("<h", data, o + 12)
        if section in dropped:
            struct.pack_into("<I", data, o + 8, 0)     # Value
            struct.pack_into("<h", data, o + 12, 0)    # SectionNumber
        aux = data[o + 17]
        o += 18 * (1 + aux)
        i += 1 + aux
    return bytes(data)


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
            out.write_bytes(_drop_data_sections(result.data))
            sidecar.write_bytes(canon.sidecar_bytes(result.rows))
            write_stamp(out, {"raw": obj})
            wrote += 1
    print(f"[build normalize_objs] {wrote} normalized, {skipped} fresh "
          f"-> {OBJDIFF / 'normalized'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
