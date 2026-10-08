"""homm3.census.libraries - name an image's statically linked library code.

Each function of a pinned archive member (VC6 LIBCMT/LIBCPMT, the SP3 MFC
NAFXCW overlay) is a /Gy COMDAT, or a label of an assembler member. An
assembler label's body runs to the next label, and an external one's also
to the next external label (`_memcpy` past its interior `TrailUpVec`,
`LeadUp1` ...). A member function names a census function when:

1. its bytes, with every relocation field masked, equal exactly one census
   function of the same size (at least MIN_FIXED fixed bytes); or
2. no census function has its size, and its whole body equals the bytes at
   exactly one census start whose extent it covers (the census cut it: an
   `__except` filter it seeds from the scope table, or `_strcpy`'s jump
   into `_strcat`); or
3. after those, its masked bytes match at several starts, or have fewer
   fixed bytes than MIN_FIXED, and exactly one of the matches calls and
   imports where the member does: every REL32 field lands on a census
   function already named by the field's symbol (`_atoi` calls `_atol`,
   `operator delete` calls `_free`; `__errno` calls `__getptd`) and every
   `__imp_` field holds that import's IAT slot (`std::_Lockit::~_Lockit`
   leaves its critical section), and every DIR32 field naming a class's
   vtable (`??_7CButton@@6B@`) holds the address the image's RTTI gives
   that vtable (the MFC control destructors differ only in the vptr they
   store).

The rows become the image's runtime-map.tsv (rva, name, library, member):
named, not matched, and excluded from the scores like the game's runtime
code.
"""
from __future__ import annotations

import re
import struct
from collections import defaultdict
from pathlib import Path

MIN_FIXED = 12
#: Initializer ordinals (`_$E365`) are volatile and never names.
VOLATILE = re.compile(r"^_?\$E[0-9]+$")
REL32 = 20
DIR32 = 6
EXTERNAL = 2
#: A class's primary vtable symbol.
VTABLE = re.compile(r"^\?\?_7(\w+)@@6B@$")


def _member_functions(obj):
    """(name, body, relocs) of every function an archive member defines.

    Every symbol of a code section runs to the next one, as compiled objects
    split; an external symbol of a section that also holds static labels
    (an assembler member's) is offered again running to the next external
    symbol, past its interior labels."""
    from homm3.census.placements import _functions_of
    externals = {}
    for sec in obj.section_table:
        if sec["characteristics"] & 0x20:
            number = sec["index"]
            externals[number] = sorted(
                off for off, name, scl in obj.section_members(number)
                if scl == EXTERNAL and not name.startswith(("$", ".")))
    out = []
    for name, number, off, body, relocs in _functions_of(obj):
        out.append((name, body, relocs))
        starts = externals.get(number)
        if not body or not starts or off not in starts:
            continue
        payload = obj.section_payload(number)
        later = [o for o in starts if o > off]
        end = later[0] if later else len(payload)
        if end <= off + len(body):
            continue                      # no interior label: the same body
        own = {site - off: ref for site, ref in obj.typed_relocations(number).items()
               if off <= site < end}
        floor = off + max((site + 4 for site in own), default=0)
        while end > max(floor, off + 1) and payload[end - 1] in (0x90, 0xCC):
            end -= 1
        out.append((name, payload[off:end], own))
    return out


def archive_functions(path: Path):
    """(member, name, body, relocs) for every code function of an archive."""
    from homm3.delink.coffx import Obj
    from homm3.verify.library_code import archive_members
    for member, body in archive_members(path):
        if body[:4] == b"\0\0\xff\xff":
            continue
        try:
            obj = Obj(body)
        except (ValueError, IndexError):
            continue
        for name, code, relocs in _member_functions(obj):
            yield member, name, code, relocs


def derive(image, functions: dict[int, int], archives: dict[str, Path], log=print,
           imports: dict[str, int] | None = None,
           vtables: dict[str, int] | None = None):
    """`imports` maps each `__imp_` symbol to its IAT slot rva; `vtables`
    maps each RTTI-named class to its primary vtable's rva."""
    imports = imports or {}
    vtables = vtables or {}
    by_size = defaultdict(list)
    by_first = defaultdict(list)

    def blob(rva, size):
        sec = image.section_of(rva)
        o = sec.raw_offset + rva - sec.rva
        return image.data[o:o + size]

    for rva, size in functions.items():
        by_size[size].append(rva)
        by_first[blob(rva, 1)].append(rva)

    def same(rva, code, mask):
        theirs = blob(rva, len(code))
        return len(theirs) == len(code) and all(
            m or a == b for a, b, m in zip(theirs, code, mask))

    hits = defaultdict(set)          # rva -> {(name, library, member)}
    covering = []                    # step 2: (rva, name, library, member)
    pending = []                     # step 3: (name, library, member, relocs, found)
    for library, path in archives.items():
        if not path.is_file():
            log(f"[libraries] {library}: {path} missing; skipped")
            continue
        for member, name, code, relocs in archive_functions(path):
            if VOLATILE.match(name) or not code:
                continue
            mask = bytearray(len(code))
            for site in relocs:
                mask[site:site + 4] = b"\1\1\1\1"
            fixed = len(code) - sum(mask)
            found = [rva for rva in by_size.get(len(code), ()) if same(rva, code, mask)]
            if not found and fixed >= MIN_FIXED:
                # 2. a census start whose extent the whole body covers
                cover = [rva for rva in by_first.get(code[:1], ())
                         if functions[rva] < len(code) and same(rva, code, mask)]
                if len(cover) == 1:
                    covering.append((cover[0], name, library, member))
            elif len(found) == 1 and fixed >= MIN_FIXED:
                hits[found[0]].add((name, library, member))
            elif found and any(kind == REL32 or ref in imports
                               for ref, kind in relocs.values()):
                pending.append((name, library, member, relocs, found))

    # Steps 2 and 3 name only the library band, which starts at the first C
    # runtime function: below it a library body is a project object's copy
    # of an inline or template COMDAT (`basic_string::_Copy`), which the
    # project object owns as the game's units do.
    band = min((rva for rva, names in hits.items()
                if any(lib == "LIBCMT" for _n, lib, _m in names)),
               default=0)
    log(f"[libraries] library band from 0x{band:x}")
    for rva, name, library, member in covering:
        if rva >= band:
            hits[rva].add((name, library, member))

    # 3. calls that land where the first passes put their callees
    named_rvas = defaultdict(set)
    for rva, names in hits.items():
        if len({n for n, _l, _m in names}) == 1:
            named_rvas[next(iter(names))[0]].add(rva)
    for name, library, member, relocs, found in pending:
        agree = []
        for rva in found:
            for site, (ref, kind) in relocs.items():
                value = struct.unpack_from("<i", blob(rva + site, 4))[0]
                if kind == REL32:
                    if named_rvas.get(ref) != {(rva + site + 4 + value) & 0xFFFFFFFF}:
                        break
                elif ref in imports and imports[ref] != value - image.image_base:
                    break
                elif kind == DIR32 and VTABLE.match(ref) and VTABLE.match(ref).group(1) in vtables \
                        and vtables[VTABLE.match(ref).group(1)] != value - image.image_base:
                    break
            else:
                agree.append(rva)
        if len(agree) == 1 and agree[0] >= band and agree[0] not in hits:
            hits[agree[0]].add((name, library, member))

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
