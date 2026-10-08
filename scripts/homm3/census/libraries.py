"""homm3.census.libraries - name an image's statically linked library code.

Each function of a pinned archive member (VC6 LIBCMT/LIBCPMT, the SP3 MFC
NAFXCW overlay) is a /Gy COMDAT. A member function whose bytes, with every
relocation field masked, equal exactly one census function of the same size
(at least MIN_FIXED fixed bytes) names that function. The rows become the
image's runtime-map.tsv (rva, name, library, member): named, not matched,
and excluded from the scores like the game's runtime code.
"""
from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path

MIN_FIXED = 12
#: Initializer ordinals (`_$E365`) are volatile and never names.
VOLATILE = re.compile(r"^_?\$E[0-9]+$")


def archive_functions(path: Path):
    """(member, name, body, masked offsets) for every code COMDAT of an archive."""
    from homm3.delink.coffx import Obj
    from homm3.verify.library_code import archive_members
    for member, body in archive_members(path):
        if body[:4] == b"\0\0\xff\xff":
            continue
        try:
            obj = Obj(body)
        except (ValueError, IndexError):
            continue
        from homm3.census.placements import _functions_of
        for name, _sec, _off, code, relocs in _functions_of(obj):
            yield member, name, code, relocs


def derive(image, functions: dict[int, int], archives: dict[str, Path], log=print):
    by_size = defaultdict(list)
    for rva, size in functions.items():
        by_size[size].append(rva)

    def blob(rva, size):
        sec = image.section_of(rva)
        o = sec.raw_offset + rva - sec.rva
        return image.data[o:o + size]

    hits = defaultdict(set)          # rva -> {(name, library, member)}
    for library, path in archives.items():
        if not path.is_file():
            log(f"[libraries] {library}: {path} missing; skipped")
            continue
        for member, name, code, relocs in archive_functions(path):
            mask = bytearray(len(code))
            for site in relocs:
                mask[site:site + 4] = b"\1\1\1\1"
            if len(code) - sum(mask) < MIN_FIXED:
                continue
            found = [rva for rva in by_size.get(len(code), ())
                     if all(m or a == b for a, b, m in zip(blob(rva, len(code)), code, mask))]
            if len(found) == 1 and not VOLATILE.match(name):
                hits[found[0]].add((name, library, member))
    rows, ambiguous = [], 0
    for rva, names in sorted(hits.items()):
        if len({n for n, _l, _m in names}) != 1:
            ambiguous += 1
            continue
        name, library, member = sorted(names)[0]
        rows.append((rva, name, library, member))
    log(f"[libraries] {len(rows)} library functions named; {ambiguous} addresses "
        "with competing names left unnamed")
    return rows
