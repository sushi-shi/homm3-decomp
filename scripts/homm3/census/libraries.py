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

Several MFC members can name one address when /OPT:ICF folded identical
library bodies (`CGdiObject::DeleteObject`, `CDC::DeleteDC` and
`CMenu::DestroyMenu`). When every such member matched that address alone
(step 1), the first name in sort order names it and the others become the
image's runtime-aliases.tsv rows: the comparison treats a call to any of
them as a call to the named function.
"""
from __future__ import annotations

import re
import struct
from collections import defaultdict
from pathlib import Path

MIN_FIXED = 12
#: The COMDAT selection of an inline or template body (IMAGE_COMDAT_SELECT_ANY).
PICK_ANY = 2
#: Consecutive census functions that open the library code.
RUN = 3
#: Initializer ordinals (`_$E365`) are volatile and never names.
VOLATILE = re.compile(r"^_?\$E[0-9]+$")
REL32 = 20
DIR32 = 6
EXTERNAL = 2
#: The library whose identical-code folds the census names: the C++
#: runtime's template bodies are the project units' to place.
FOLDED_LIBRARY = "NAFXCW"
#: A class's primary vtable symbol.
VTABLE = re.compile(r"^\?\?_7(\w+)@@6B@$")
#: A class's scalar or vector deleting destructor.
DELETING = re.compile(r"^\?\?_[GE](.+)@@[QUI]AEPAXI@Z$")


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
    selection = {sec["index"]: sec["comdat"] for sec in obj.section_table}
    for name, number, off, body, relocs in _functions_of(obj):
        out.append((name, body, relocs, selection.get(number, 0)))
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
        out.append((name, payload[off:end], own, selection.get(number, 0)))
    return out


def unreached_destructor(name, vtables):
    """Whether `name` is a deleting destructor of a class whose vtable the
    image's RTTI does not name (`vtables`: class -> vtable rva): no vtable
    reaches it, so the image holds no such body."""
    match = DELETING.match(name)
    return match is not None and match.group(1) not in vtables


def library_start(order, once, run=RUN):
    """The first census start of `run` consecutive functions that are all
    library members' own functions (`once`), or None."""
    return next((rva for k, rva in enumerate(order)
                 if len(order[k:k + run]) == run and all(r in once for r in order[k:k + run])),
                None)


def archive_functions(path: Path):
    """(member, name, body, relocs, selection) for every code function of an
    archive; `selection` is its section's COMDAT selection (0: no COMDAT,
    2: any, an inline or template body every user may emit)."""
    from homm3.delink.coffx import Obj
    from homm3.verify.library_code import archive_members
    for member, body in archive_members(path):
        if body[:4] == b"\0\0\xff\xff":
            continue
        try:
            obj = Obj(body)
        except (ValueError, IndexError):
            continue
        for name, code, relocs, selection in _member_functions(obj):
            yield member, name, code, relocs, selection


def derive(image, functions: dict[int, int], archives: dict[str, Path], log=print,
           imports: dict[str, int] | None = None,
           vtables: dict[str, int] | None = None,
           aliases: list | None = None):
    """`imports` maps each `__imp_` symbol to its IAT slot rva; `vtables`
    maps each RTTI-named class to its primary vtable's rva. `aliases`, when
    given, receives (rva, name, library, member) for the other members of
    an identical-code fold."""
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

    defined_once = set()             # (name, library, member) no project object emits
    hits = defaultdict(set)          # rva -> {(name, library, member)}
    hit_relocs = {}                  # (rva, name) -> the step-1 member's relocs
    covering = []                    # step 2: (rva, name, library, member)
    pending = []                     # step 3: (name, library, member, relocs, found)
    for library, path in archives.items():
        if not path.is_file():
            log(f"[libraries] {library}: {path} missing; skipped")
            continue
        for member, name, code, relocs, selection in archive_functions(path):
            if VOLATILE.match(name) or not code:
                continue
            if selection != PICK_ANY:
                defined_once.add((name, library, member))
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
                hit_relocs[found[0], name] = relocs
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

    # Below the band a deleting destructor is reached only through its
    # class's vtable: when the image's RTTI names no vtable of that class,
    # the body is a project class's destructor with the same masked bytes
    # (h3maped 0x43fa68, TMine's, is no std::basic_iostream's).
    for rva in [rva for rva in hits if rva < band]:
        hits[rva] = {(n, l, m) for n, l, m in hits[rva] if not unreached_destructor(n, vtables)}
        if not hits[rva]:
            del hits[rva]

    # 3. calls that land where the first passes put their callees
    named_rvas = defaultdict(set)
    for rva, names in hits.items():
        if len({n for n, _l, _m in names}) == 1:
            named_rvas[next(iter(names))[0]].add(rva)
    def agrees(rva, relocs):
        for site, (ref, kind) in relocs.items():
            value = struct.unpack_from("<i", blob(rva + site, 4))[0]
            if kind == REL32:
                if named_rvas.get(ref) != {(rva + site + 4 + value) & 0xFFFFFFFF}:
                    return False
            elif ref in imports and imports[ref] != value - image.image_base:
                return False
            elif kind == DIR32 and VTABLE.match(ref) \
                    and VTABLE.match(ref).group(1) in vtables \
                    and vtables[VTABLE.match(ref).group(1)] != value - image.image_base:
                return False
        return True

    for name, library, member, relocs, found in pending:
        agree = [rva for rva in found if agrees(rva, relocs)]
        if len(agree) == 1 and agree[0] >= band and agree[0] not in hits:
            hits[agree[0]].add((name, library, member))

    # Members whose masked bytes all name one address (the MFC destructors
    # of classes with the same body) keep the names whose fields agree.
    for rva, names in hits.items():
        if len({n for n, _l, _m in names}) > 1 and rva >= band:
            kept = {(n, l, m) for n, l, m in names
                    if (rva, n) in hit_relocs and agrees(rva, hit_relocs[rva, n])}
            if len({n for n, _l, _m in kept}) == 1:
                hits[rva] = kept

    # Identical-code folds: every member's own masked bytes name this
    # address and no other, and every member's relocated fields agree with
    # it (callees named by the runtime map or by an earlier fold).
    claimed = defaultdict(set)
    for rva, names in hits.items():
        for name, _library, _member in names:
            claimed[name].add(rva)
    candidates = {rva: names for rva, names in hits.items()
                  if aliases is not None and rva >= band
                  and len({n for n, _l, _m in names}) > 1
                  and all(library == FOLDED_LIBRARY for _n, library, _m in names)
                  and all((rva, n) in hit_relocs and claimed[n] == {rva}
                          for n, _l, _m in names)}
    folds = set()
    changed = True
    while changed:
        changed = False
        for rva, names in sorted(candidates.items()):
            if rva in folds:
                continue
            if all(agrees(rva, hit_relocs[rva, n]) for n, _l, _m in names):
                folds.add(rva)
                for n, _l, _m in names:
                    named_rvas[n].add(rva)
                changed = True

    # A member's own function (no pick-any COMDAT: an inline or template body
    # a project object may also emit) is linked with its object, after every
    # project object: one inside the project's code is a project function
    # that happens to share its masked bytes (h3maped 0x4c241b is no
    # COleControl::GetStockTextMetrics). The library code starts at the
    # first run of RUN such functions.
    order = sorted(functions)
    once = {rva for rva, names in hits.items() if names and names <= defined_once}
    start = library_start(order, once)
    if start is not None:
        for rva in sorted(r for r in once if r < start):
            log(f"[libraries]   0x{rva:x} {sorted(hits[rva])[0][0]} dropped: "
                f"its member's own function before the library code (0x{start:x})")
            del hits[rva]

    rows, ambiguous, folded = [], 0, 0
    for rva, names in sorted(hits.items()):
        if len({n for n, _l, _m in names}) != 1:
            if rva not in folds:
                ambiguous += 1
                continue
            ordered = sorted(names)
            name, library, member = ordered[0]
            rows.append((rva, name, library, member))
            seen = {name}
            for other in ordered[1:]:
                if other[0] not in seen:
                    seen.add(other[0])
                    aliases.append((rva, *other))
            folded += 1
            continue
        name, library, member = sorted(names)[0]
        rows.append((rva, name, library, member))
    log(f"[libraries] {len(rows)} library functions named ({folded} identical-code "
        f"folds); {ambiguous} addresses with competing names left unnamed")
    return rows
