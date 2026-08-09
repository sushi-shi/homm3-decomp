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

The first canonicalization pass strips trailing COMDAT NOP fill. A linked
target sometimes has the same logical function length but necessarily keeps
one to three of those NOPs before the next 4-byte-aligned function. The paired
pass below restores exactly that target-carried fill to the base comparison
copy. It only does so when stripping the target's NOP suffix makes both logical
lengths equal; a genuinely longer or shorter function is left alone.
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
FUNCTION_TYPE = 0x0020
TEXT_PAD_TRIM_LIMIT = 15


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


def _retain_matching_target_padding(base_payload: bytes,
                                    target_payload: bytes) -> tuple[bytes, int]:
    """Retain linked-target NOP fill when the logical function sizes agree.

    VC6 emits each base function in its own padded /Gy section, whereas the
    delinked retail object packs every function into one .text section. The
    canonicalizer initially removes all trailing base fill. If retail's next
    function is 4-byte aligned, objdiff still assigns the intervening NOPs to
    the previous function. Restore only that proven suffix, without changing
    either function's logical code extent.
    """
    data = bytearray(base_payload)
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)

    base_functions: dict[int, list] = {}
    for symbol in base.symbols.values():
        if symbol.typ == FUNCTION_TYPE and symbol.section > 0:
            base_functions.setdefault(symbol.section, []).append(symbol)

    target_functions: dict[int, list] = {}
    target_by_name: dict[str, list] = {}
    for symbol in target.symbols.values():
        if symbol.typ != FUNCTION_TYPE or symbol.section <= 0:
            continue
        target_functions.setdefault(symbol.section, []).append(symbol)
        target_by_name.setdefault(symbol.name, []).append(symbol)

    retained = 0
    for section_index, functions in base_functions.items():
        # A normal /Gy contribution owns exactly one external function. Skip
        # unusual multi-function sections rather than guessing their extents.
        if len(functions) != 1:
            continue
        function = functions[0]
        section = base.sections[section_index - 1]
        if function.value != 0 or not section.characteristics & CNT_CODE:
            continue
        counterparts = target_by_name.get(function.name, ())
        if len(counterparts) != 1:
            continue
        counterpart = counterparts[0]
        target_section = target.sections[counterpart.section - 1]
        later = [
            row.value for row in target_functions[counterpart.section]
            if row.value > counterpart.value
        ]
        end = min(later) if later else target_section.raw_size
        if end <= counterpart.value:
            continue
        extent = end - counterpart.value
        target_bytes = target.section_bytes(target_section)[counterpart.value:end]
        pad = 0
        while (pad < min(TEXT_PAD_TRIM_LIMIT, len(target_bytes))
               and target_bytes[-1 - pad] == 0x90):
            pad += 1
        if not pad or section.raw_size != extent - pad:
            continue

        # Shrinking a section header leaves its original bytes in the file.
        # Require those hidden bytes to be the exact same NOP suffix before
        # making them visible again.
        fill_start = section.raw_offset + section.raw_size
        fill_end = section.raw_offset + extent
        if base_payload[fill_start:fill_end] != b"\x90" * pad:
            continue
        struct.pack_into("<I", data, section.header_offset + 16, extent)
        retained += 1

    return bytes(data), retained


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
    retained = 0
    base_root = OBJDIFF / "base"
    for base_obj in sorted(base_root.rglob("*.obj")):
        rel = base_obj.relative_to(base_root)
        target_rel = rel.with_name(rel.stem + ".c.obj")
        target_obj = OBJDIFF / "target" / target_rel
        normalized_base = OBJDIFF / "normalized/base" / rel
        normalized_target = OBJDIFF / "normalized/target" / target_rel
        if not (target_obj.is_file() and normalized_base.is_file()
                and normalized_target.is_file()):
            continue
        padded, count = _retain_matching_target_padding(
            normalized_base.read_bytes(), normalized_target.read_bytes())
        if count:
            normalized_base.write_bytes(padded)
            retained += count
        # Padding is a paired normalization decision, so the base copy is
        # stale whenever either raw input changes, even when this run found no
        # suffix to retain.
        write_stamp(normalized_base, {"raw": base_obj, "target": target_obj})

    print(f"[build normalize_objs] {wrote} normalized, {skipped} fresh, "
          f"{retained} target-padding span(s) retained "
          f"-> {OBJDIFF / 'normalized'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
