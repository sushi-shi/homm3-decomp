"""Verify statically linked library code against its pinned object bytes.

`config/retail/runtime-contributions.tsv` places individual COFF sections of
the pinned VC6 SP3 `LIBCMT.LIB`/`LIBCPMT.LIB` members (plus import-library
thunks) at retail RVAs. The reviewed zlib map places compiled pristine
vendor sections the same way. Each placement is checked here:

* every non-relocated byte of the section equals retail;
* every relocation resolves to the right symbol: a placed library section,
  an import slot, an absolute or COMMON symbol, a game definition whose own
  bytes are the same emitted COMDAT (ICF), or a library datum whose implied
  base holds the member's initialized bytes and is the same for every
  reference in the image.

Linker fill is credited only as the 0xCC run directly before a verified
contribution, shorter than its section alignment and ending on its aligned
start.
A failure leaves the bytes `library-unverified`; nothing is masked away.

    homm3 verify library-code            summary and findings
    homm3 verify library-code --tsv      per-contribution verdicts
    homm3 verify library-code --data     library data placements implied
"""

from __future__ import annotations

import struct
from collections import defaultdict
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path

from homm3.core.paths import BUILD, RETAIL
from homm3.core.tsv import read as read_tsv

INVENTORY = RETAIL / 'runtime-contributions.tsv'
ZLIB_MAP = RETAIL / 'zlib-map.tsv'
RUNTIME_LIBRARIES = ('LIBCMT.LIB', 'LIBCPMT.LIB')

KINDS = ('code', 'alias', 'data', 'bss', 'common', 'thunk')
DIR32, DIR32NB, REL32 = 6, 7, 20
MEM_EXECUTE = 0x20000000
LNK_COMDAT = 0x1000
WEAK_EXTERNAL = 105
FILL = 0xCC


@dataclass(frozen=True)
class Contribution:
    rva: int
    size: int
    library: str
    member: str
    section: int          # COFF section number; 0 for a thunk or COMMON
    symbol: str
    evidence: str = '-'
    kind: str = 'code'    # code | data | bss | common | thunk


@dataclass
class Verdict:
    row: Contribution
    verdict: str = 'exact'            # exact | mismatch | unresolved
    reasons: list = field(default_factory=list)
    data: list = field(default_factory=list)   # (key, address) data references
    align: int = 16
    symbols: list = field(default_factory=list)  # (rva, name) defined here


def read_inventory(path: Path | None = None) -> list[Contribution]:
    _banner, _header, rows = read_tsv(path or INVENTORY)
    out = [Contribution(int(r['rva'], 16), int(r['size'], 0), r['library'],
                        r['member'], int(r['section']), r['symbol'],
                        r.get('evidence', '-'), r['kind']) for r in rows]
    for row in out:
        if row.kind not in KINDS:
            raise ValueError(f'{row.rva:#x}: unknown contribution kind {row.kind!r}')
    return out


# ---------------------------------------------------------------- archives --

def archive_members(path: Path):
    """(member basename, body) for a Microsoft `!<arch>` library.

    VC6 long names are NUL-terminated offsets into the `//` member.
    """
    data = Path(path).read_bytes()
    if data[:8] != b'!<arch>\n':
        raise ValueError(f'{path}: not an archive')
    pos, longnames = 8, b''
    while pos + 60 <= len(data):
        name = data[pos:pos + 16].decode('latin1').rstrip()
        size = int(data[pos + 48:pos + 58].strip() or 0)
        body = data[pos + 60:pos + 60 + size]
        pos += 60 + size + (size & 1)
        if name == '//':
            longnames = body
            continue
        if name == '/':
            continue
        if name.startswith('/') and name[1:].isdigit():
            start = int(name[1:])
            end = longnames.find(b'\0', start)
            name = longnames[start:end if end >= 0 else None].decode('latin1')
        name = name.rstrip('/').replace('\\', '/').rsplit('/', 1)[-1]
        yield name, body


@lru_cache(maxsize=None)
def _archive(path: str, mtime: float):
    from homm3.compare.canonicalize import CoffObject
    objects, imports = {}, {}
    for name, body in archive_members(Path(path)):
        if body[:4] == b'\0\0\xff\xff':
            parsed = _short_import(body)
            if parsed:
                imports[parsed[0]] = parsed
            continue
        try:
            obj = CoffObject(body)
        except (ValueError, struct.error):
            continue
        if name in objects:
            # Import libraries repeat the DLL name; runtime members are unique.
            objects.setdefault(None, []).append(obj)
            continue
        objects[name] = obj
    return objects, imports


def _short_import(body: bytes):
    from homm3.delink.implib import _short_import as parse
    return parse(body)


def archive(path: Path):
    path = Path(path)
    return _archive(str(path), path.stat().st_mtime)


# ----------------------------------------------------------------- objects --

@dataclass
class Section:
    library: str
    member: str
    obj: object
    number: int
    data: bytes
    size: int
    align: int
    code: bool
    bss: bool
    relocations: tuple        # (offset, type, symbol index)
    comdat: str | None        # public COMDAT symbol name


def _comdat_names(obj):
    names, seen = {}, set()
    for index in sorted(obj.symbols):
        sym = obj.symbols[index]
        if sym.section <= 0:
            continue
        sec = obj.sections[sym.section - 1]
        if sym.storage_class == 3 and sym.aux_count and sym.name == sec.name \
                and sym.section not in seen:
            seen.add(sym.section)
            continue
        if sym.section in seen and sym.section not in names:
            names[sym.section] = sym.name if sym.storage_class == 2 else None
    return names


def _bss_size(obj, number):
    for sym in obj.symbols.values():
        if sym.section == number and sym.storage_class == 3 and sym.aux_count \
                and sym.name == obj.sections[number - 1].name:
            return struct.unpack_from('<I', obj.data, sym.offset + 18)[0]
    return 0


def section(library, member, obj, number) -> Section:
    sec = obj.sections[number - 1]
    flags = sec.characteristics
    code = bool(flags & MEM_EXECUTE)
    bss = bool(flags & 0x80) and not code
    data = b'' if bss else obj.section_bytes(sec)
    size = sec.raw_size or (_bss_size(obj, number) if bss else 0)
    align_bits = (flags >> 20) & 0xF
    relocations = tuple(sorted((r.site, r.typ, r.symbol_index)
                               for r in obj.relocations if r.section == number))
    comdat = _comdat_names(obj).get(number) if flags & LNK_COMDAT else None
    return Section(library, member, obj, number, data, size,
                   1 << (align_bits - 1) if align_bits else 16, code, bss,
                   relocations, comdat)


def _mask(sec: Section) -> bytearray:
    mask = bytearray(len(sec.data))
    for offset, _typ, _sym in sec.relocations:
        for i in range(offset, min(offset + 4, len(mask))):
            mask[i] = 1
    return mask


def differences(sec: Section, retail: bytes | None) -> list[int]:
    """Offsets of unrelocated bytes that differ (a short read differs wholly)."""
    if retail is None or len(retail) != len(sec.data):
        return list(range(len(sec.data)))
    if retail == sec.data:
        return []
    mask = _mask(sec)
    return [i for i, (a, b) in enumerate(zip(sec.data, retail)) if a != b and not mask[i]]


# ---------------------------------------------------------------- resolver --

class Image:
    """Retail bytes, sections and import slots."""

    def __init__(self, pe):
        from homm3.delink.image import Image as Delink
        self.pe = pe
        self.base = pe.image_base
        self.sections = {s['name']: s for s in pe.sections}
        self.slots = defaultdict(set)
        for slot, name, dll, _ordinal in Delink(pe).import_slots():
            if name:
                self.slots[(dll.lower(), name)].add(slot)
                self.slots[('', name)].add(slot)

    def read(self, rva, size):
        return self.pe.read(rva, size)

    def section_of(self, rva, size=1):
        for name, s in self.sections.items():
            if s['va'] <= rva and rva + size <= s['va'] + s['vsize']:
                return name
        return None

    def import_slot(self, imp_symbol: str, dll: str = ''):
        from homm3.delink.implib import _normalize
        slots = self.slots.get((dll.lower(), _normalize(imp_symbol)), set())
        return next(iter(slots)) if len(slots) == 1 else None


class Library:
    """Symbol tables of the runtime archives and compiled vendor objects."""

    def __init__(self, archives: dict, vendor: dict, imports: dict):
        self.objects = {}                     # (library, member) -> CoffObject
        self.public = defaultdict(list)       # name -> [(library, member, number, value)]
        self.absolute, self.common = {}, defaultdict(set)
        self.imports = imports                # thunk symbol -> (symbol, dll, hint, type)
        self._sections = {}
        for library, members in list(archives.items()) + list(vendor.items()):
            for member, obj in members.items():
                self.objects[(library, member)] = obj
                for sym in obj.symbols.values():
                    if sym.storage_class != 2:
                        continue
                    if sym.section > 0:
                        self.public[sym.name].append((library, member, sym.section, sym.value))
                    elif sym.section == -1:
                        self.absolute[sym.name] = sym.value
                    elif sym.value:
                        self.common[sym.name].add(sym.value)

    def section(self, library, member, number) -> Section:
        key = (library, member, number)
        if key not in self._sections:
            self._sections[key] = section(library, member, self.objects[(library, member)], number)
        return self._sections[key]


def load_libraries(toolchain: Path | None = None, vendor_dir: Path | None = None,
                   zlib_units=()):
    from homm3.compare.canonicalize import CoffObject
    from homm3.core.paths import msvc_dir
    root = Path(toolchain or msvc_dir()) / 'lib'
    archives = {name: archive(root / name)[0] for name in RUNTIME_LIBRARIES}
    for name, members in archives.items():
        if None in members:
            raise ValueError(f'{name}: duplicate member basenames')
    vendor = {'zlib': {f'{u}.obj': CoffObject((Path(vendor_dir or BUILD / 'objdiff/base')
                                                / f'{u}.obj').read_bytes())
                       for u in sorted(set(zlib_units))}}
    imports = {}
    for path in sorted(root.glob('*.LIB')) + sorted(root.glob('*.lib')):
        if path.name.upper() in RUNTIME_LIBRARIES:
            continue
        try:
            for symbol, parsed in archive(path)[1].items():
                imports.setdefault(symbol, (path.name.upper(),) + parsed)
        except (ValueError, OSError):
            continue
    return Library(archives, vendor, imports)


# ------------------------------------------------------------- verification --

@dataclass
class Placement:
    row: Contribution
    sec: Section | None


def _placements(library: Library, rows, zlib_rows):
    out = []
    for row in rows:
        if row.kind in ('thunk', 'common'):
            out.append(Placement(row, None))
            continue
        out.append(Placement(row, library.section(row.library, row.member, row.section)))
    for rva, name, unit in zlib_rows:
        member = f'{unit}.obj'
        obj = library.objects.get(('zlib', member))
        hits = [s for s in (obj.symbols.values() if obj else ())
                if s.name == name and s.section > 0 and s.value == 0]
        if len(hits) != 1:
            out.append(Placement(Contribution(rva, 0, 'zlib', member, 0, name,
                                              'unplaced', 'thunk'), None))
            continue
        sec = library.section('zlib', member, hits[0].section)
        out.append(Placement(Contribution(rva, sec.size, 'zlib', member, sec.number, name),
                             sec))
    return out


def verify(pe, rows=None, *, names=None, library=None, zlib_rows=None,
           game_comdats=None, image=None, reloc_sites=None):
    """Verdicts for every contribution; see the module docstring.

    `names` maps a (masked) symbol name to the retail addresses the model
    assigns it outside the runtime-map. `game_comdats(name, rva)` returns
    True when a game object's COMDAT `name` is byte-identical at `rva`.
    `reloc_sites`, the sorted reviewed DIR32 site inventory, must agree with
    every counted section's own DIR32 relocations.
    """
    from homm3.core import msvc_names
    image = image or Image(pe)
    rows = read_inventory() if rows is None else rows
    if zlib_rows is None:
        zlib_rows = [(int(r['rva'], 16), r['name'], r['unit'])
                     for r in read_tsv(ZLIB_MAP)[2] if r.get('kind', 'func') == 'func']
    library = library or load_libraries(zlib_units=[u for _r, _n, u in zlib_rows])
    names = names or {}
    placements = _placements(library, rows, zlib_rows)

    # Where each placed section, and each symbol it defines, lives.
    at = {}                                    # (library, member, number) -> rva
    public_at = defaultdict(set)               # symbol -> {rva}
    for p in placements:
        if p.sec is None:
            if p.row.library != 'zlib':
                public_at[p.row.symbol].add(p.row.rva)
            continue
        key = (p.row.library, p.row.member, p.row.section)
        if key in at and at[key] != p.row.rva:
            raise ValueError(f'{key} placed twice: {at[key]:#x} and {p.row.rva:#x}')
        at[key] = p.row.rva
        obj = p.sec.obj
        for sym in obj.symbols.values():
            if sym.section == p.row.section and sym.storage_class == 2:
                public_at[sym.name].add(p.row.rva + sym.value)
    for rva, name, _unit in zlib_rows:
        public_at[name].add(rva)

    def named(name, target):
        return target in names.get(name, ()) or \
            target in names.get(msvc_names.mask(name), ())

    data_refs = defaultdict(lambda: defaultdict(list))   # key -> address -> [rows]
    verdicts = []
    for p in placements:
        v = Verdict(p.row)
        verdicts.append(v)
        if p.row.kind == 'common':
            _verify_common(v, image, library, game_comdats)
            continue
        if p.sec is None:
            _verify_thunk(v, image, library)
            continue
        sec = p.sec
        v.align = sec.align
        v.symbols = [(p.row.rva + sym.value, sym.name) for sym in sec.obj.symbols.values()
                     if sym.section == sec.number and sym.storage_class in (2, 3)
                     and not sym.aux_count]
        kind = 'code' if sec.code else 'bss' if sec.bss else 'data'
        if p.row.kind != kind and not (p.row.kind == 'alias' and kind == 'code'):
            v.verdict = 'mismatch'
            v.reasons.append(f'{p.row.kind} row names a {kind} section')
            continue
        if p.row.size != sec.size:
            v.verdict = 'mismatch'
            v.reasons.append(f'size {p.row.size} != section {sec.size}')
            continue
        if p.row.rva % sec.align:
            v.verdict = 'mismatch'
            v.reasons.append(f'not {sec.align}-byte aligned')
            continue
        retail = image.read(p.row.rva, sec.size)
        if kind == 'bss':
            if image.section_of(p.row.rva, max(sec.size, 1)) != '.data' or retail != bytes(sec.size):
                v.verdict = 'mismatch'
                v.reasons.append('uninitialized section is not zero-filled .data')
            continue
        if kind == 'data' and image.section_of(p.row.rva, max(sec.size, 1)) not in ('.rdata', '.data'):
            v.verdict = 'mismatch'
            v.reasons.append('data section outside .rdata/.data')
            continue
        wrong = differences(sec, retail)
        if wrong:
            v.verdict = 'mismatch'
            v.reasons.append(f'{len(wrong)} byte(s) differ, first +{wrong[0]:#x}')
            continue
        obj = sec.obj
        if reloc_sites is not None and p.row.kind != 'alias':
            import bisect
            own = {p.row.rva + offset for offset, typ, index in sec.relocations
                   if typ == DIR32 and obj.symbols[index].section != -1
                   and obj.symbols[index].name not in library.absolute}
            lo = bisect.bisect_left(reloc_sites, p.row.rva)
            hi = bisect.bisect_left(reloc_sites, p.row.rva + sec.size)
            listed = set(reloc_sites[lo:hi])
            for site in sorted(own - listed):
                v.reasons.append(f'+{site - p.row.rva:#x}: DIR32 site missing from relocs.tsv')
            for site in sorted(listed - own):
                v.reasons.append(f'+{site - p.row.rva:#x}: relocs.tsv site has no COFF relocation')
        for offset, typ, index in sec.relocations:
            sym = obj.symbols[index]
            if typ not in (DIR32, DIR32NB, REL32) or offset + 4 > sec.size:
                v.reasons.append(f'+{offset:#x}: relocation type {typ}')
                continue
            addend = struct.unpack_from('<i', sec.data, offset)[0]
            word = struct.unpack_from('<I', retail, offset)[0]
            if typ == REL32:
                target = p.row.rva + offset + 4 + struct.unpack('<i', struct.pack('<I', word))[0] - addend
                absolute = None
            else:
                absolute = (word - addend) & 0xffffffff
                target = (absolute - (image.base if typ == DIR32 else 0)) & 0xffffffff
            why = _resolve(v, sym, obj, p, target, absolute, image, library, at,
                           public_at, named, data_refs, game_comdats)
            if why:
                v.reasons.append(f'+{offset:#x} {sym.name}: {why}')

    # A datum is resolved only if every reference implies the same base and
    # the retail bytes there are the member's initialized bytes.
    conflicts = {key for key, addresses in data_refs.items() if len(addresses) > 1}
    fits = {}
    for key, addresses in data_refs.items():
        for address in addresses:
            fits[(key, address)] = _data_fits(key, address, image, library)
    for v in verdicts:
        for key, address in v.data:
            if key in conflicts:
                v.reasons.append(f'{key[-1]}: data identity has {len(data_refs[key])} bases')
            elif not fits[(key, address)]:
                v.reasons.append(f'{key[-1]}: bytes at {address:#x} do not match')
        if v.verdict == 'exact' and v.reasons:
            v.verdict = 'unresolved'
    data = [(key, address, fits[(key, address)], len(refs))
            for key, addresses in data_refs.items() if key not in conflicts
            for address, refs in addresses.items()]
    return verdicts, data


def _resolve(v, sym, obj, p, target, absolute, image, library, at, public_at,
             named, data_refs, game_comdats):
    """None when the relocation target is proven; else the reason."""
    row = p.row
    if sym.section > 0:
        sec = library.section(row.library, row.member, sym.section)
        if sym.storage_class == 2 and sec.comdat is not None:
            # A public COMDAT: the linker keeps one copy, not necessarily
            # this member's. Resolve it by name like an external.
            pass
        elif sec.code:
            placed = at.get((row.library, row.member, sym.section))
            if placed is None:
                return 'target section not placed'
            return None if placed + sym.value == target else f'expected {placed + sym.value:#x}'
        else:
            placed = at.get((row.library, row.member, sym.section))
            if placed is not None:
                return None if placed + sym.value == target else f'expected {placed + sym.value:#x}'
            key = ('section', row.library, row.member, sym.section)
            v.data.append((key, target - sym.value))
            data_refs[key][target - sym.value].append(row)
            return None
    if sym.section == -1:
        return None if absolute == sym.value else 'absolute value differs'
    name = sym.name
    if sym.storage_class == WEAK_EXTERNAL and name not in public_at and name not in library.public:
        tag = struct.unpack_from('<I', obj.data, sym.offset + 18)[0]
        default = obj.symbols[tag]
        return _resolve(v, default, obj, p, target, absolute, image, library, at,
                        public_at, named, data_refs, game_comdats)
    if name.startswith('__imp_'):
        slot = image.import_slot(name)
        return None if slot == target else 'import slot differs'
    if target in public_at.get(name, ()):
        return None
    if name in library.absolute:
        return None if absolute == library.absolute[name] else 'absolute value differs'
    if named(name, target):
        return None
    if game_comdats and game_comdats(name, target):
        return None
    if name in public_at:
        return f'expected {", ".join(hex(a) for a in sorted(public_at[name]))}'
    defs = library.public.get(name, [])
    data_defs = [d for d in defs if not library.section(d[0], d[1], d[2]).code]
    if defs and len(data_defs) == len(defs):
        key = ('symbol', name)
        v.data.append((key, target))
        data_refs[key][target].append(row)
        return None
    if not defs and name in library.common:
        if image.section_of(target, max(library.common[name])) != '.data':
            return 'COMMON outside .data'
        key = ('common', name)
        v.data.append((key, target))
        data_refs[key][target].append(row)
        return None
    if defs:
        return 'code symbol not at a placed definition'
    return 'unresolved external'


def _data_fits(key, address, image, library):
    if key[0] == 'common':
        return True
    if key[0] == 'section':
        candidates = [(library.section(key[1], key[2], key[3]), 0)]
    else:
        candidates = [(library.section(lib, member, number), value)
                      for lib, member, number, value in library.public.get(key[1], [])]
    for sec, value in candidates:
        base = address - value
        where = image.section_of(base, sec.size)
        if where not in ('.rdata', '.data'):
            continue
        if sec.bss:
            if where == '.data':
                return True
            continue
        if not differences(sec, image.read(base, sec.size)):
            return True
    return False


def common_alignment(size: int) -> int:
    """The linker's COMMON alignment: the size's power of two, at most 16."""
    align = 1
    while align < size and align < 16:
        align *= 2
    return align


def _verify_common(v, image, library, game_comdats):
    """A linker-allocated COMMON: largest declared size, zero-filled .data."""
    row = v.row
    sizes = set(library.common.get(row.symbol, ()))
    if game_comdats is not None and hasattr(game_comdats, 'common_sizes'):
        sizes |= game_comdats.common_sizes(row.symbol)
    v.align = common_alignment(row.size)
    v.symbols = [(row.rva, row.symbol)]
    if not sizes or row.size != max(sizes):
        v.verdict = 'mismatch'
        v.reasons.append(f'size {row.size} != largest declaration {max(sizes) if sizes else "-"}')
    elif row.rva % v.align:
        v.verdict = 'mismatch'
        v.reasons.append(f'not {v.align}-byte aligned')
    elif image.section_of(row.rva, row.size) != '.data' or image.read(row.rva, row.size) != bytes(row.size):
        v.verdict = 'mismatch'
        v.reasons.append('COMMON is not zero-filled .data')


def _verify_thunk(v, image, library):
    """A linker import thunk: `jmp dword ptr [__imp_X]` from an import member."""
    row = v.row
    if row.library == 'zlib':
        v.verdict, v.reasons = 'unresolved', ['zlib symbol has no unique section']
        return
    parsed = library.imports.get(row.symbol)
    if parsed is None or parsed[0] != row.library.upper():
        v.verdict = 'unresolved'
        v.reasons.append(f'{row.symbol} is not an import member of {row.library}')
        return
    _lib, symbol, dll, _hint, _name_type = parsed
    slot = image.import_slot('__imp_' + symbol, dll)
    retail = image.read(row.rva, row.size)
    if slot is None or row.size != 6 or retail != b'\xff\x25' + struct.pack('<I', image.base + slot):
        v.verdict = 'mismatch'
        v.reasons.append('not jmp [IAT slot of the import]')


# ------------------------------------------------------------------ ranges --

def ranges(verdicts, pe):
    """[(start, end, category, identity)] for exact contributions and fill."""
    # An empty section still aligns the next position, so it keeps its fill.
    exact = sorted({(v.row.rva, v.row.rva + v.row.size): v for v in verdicts
                    if v.verdict == 'exact' and v.row.kind != 'alias'}.items())
    out = []
    for (start, end), v in exact:
        if end > start and v.row.kind != 'alias':
            category = 'library-vendor' if v.row.library == 'zlib' else 'library-runtime'
            out.append((start, end, category, identity(v.row)))
    # Linker fill: the 0xCC run right before a verified contribution, shorter
    # than that contribution's alignment and ending on its aligned start.
    align = {}
    for v in verdicts:
        if v.verdict == 'exact':
            align[v.row.rva] = max(align.get(v.row.rva, 1), v.align)
    # Data fill is zero, which alone proves nothing: it is credited only when
    # it runs exactly from a verified contribution's end to the next aligned
    # start.
    previous_end = 0
    for (start, end), v in exact:
        category = 'library-vendor' if v.row.library == 'zlib' else 'library-runtime'
        if v.row.kind in ('code', 'thunk'):
            size = 0
            while size + 1 < align[start] and start - size - 1 >= previous_end and \
                    pe.read(start - size - 1, 1) == bytes([FILL]):
                size += 1
            if size and start % align[start] == 0:
                out.append((start - size, start, category, f'link fill before {identity(v.row)}'))
        else:
            gap = start - previous_end
            if 0 < gap < align[start] and gap == (-previous_end) % align[start] and \
                    pe.read(previous_end, gap) == bytes(gap):
                out.append((previous_end, start, category, f'link fill before {identity(v.row)}'))
        previous_end = max(previous_end, end)
    return out


def identity(row: Contribution) -> str:
    if row.kind == 'thunk':
        return f'{row.library}:{row.symbol} import thunk'
    if row.kind == 'common':
        return f'{row.library}:COMMON {row.symbol}'
    return f'{row.library}:{row.member}#{row.section}:{row.symbol}'


# ------------------------------------------------------------ game COMDATs --

class GameComdats:
    """Game-emitted COMDAT copies of library templates (linked first).

    Game objects precede the libraries on the link line, so a COMDAT (code or
    data) that a game object also emits is linked from the game object (or
    ICF-folded into an identical game function). `(name, rva)` is accepted when a compiled
    game object emits COMDAT `name` whose unrelocated bytes equal retail at a
    census function start `rva`.
    """

    def __init__(self, pe, starts, base_dir: Path | None = None, exclude=()):
        self.pe, self.starts = pe, set(starts)
        self.base_dir = Path(base_dir or BUILD / 'objdiff/base')
        self.exclude = set(exclude)
        self._index = None
        self._cache = {}

    def _build(self):
        from homm3.compare.canonicalize import CoffObject
        index, commons = defaultdict(list), defaultdict(set)
        for path in sorted(self.base_dir.glob('*.obj')):
            if path.stem in self.exclude:
                continue
            try:
                obj = CoffObject(path.read_bytes())
            except (ValueError, struct.error):
                continue
            for number, name in _comdat_names(obj).items():
                if name:
                    index[name].append((path.stem, obj, number))
            for sym in obj.symbols.values():
                if sym.storage_class == 2 and sym.section == 0 and sym.value:
                    commons[sym.name].add(sym.value)
        self._index, self._commons = index, commons

    def common_sizes(self, name):
        if self._index is None:
            self._build()
        return self._commons.get(name, set())

    def __call__(self, name, rva):
        key = (name, rva)
        if key not in self._cache:
            if self._index is None:
                self._build()
            self._cache[key] = None
            for unit, obj, number in self._index.get(name, ()):
                sec = section('game', unit, obj, number)
                # A census start, or the COMDAT's own section alignment.
                if rva not in self.starts and rva % sec.align:
                    continue
                retail = self.pe.read(rva, sec.size)
                if sec.bss and retail != bytes(sec.size):
                    continue
                if not differences(sec, retail):
                    self._cache[key] = f'{unit}.obj'
                    break
        return self._cache[key] is not None


# ----------------------------------------------------------------- reports --

def _data_row(key, address, fits, references):
    kind, rest = key[0], key[1:]
    if kind == 'section':
        library, member, number = rest
        name = f'{library}:{member}#{number}'
    else:
        name = rest[0]
    return dict(kind=kind, name=name, rva=f'{address:#x}', fits=fits, references=references)


def summary(verdicts, data, game=None):
    """JSON-ready report: totals, every non-exact row, implied library data."""
    exact = [v for v in verdicts if v.verdict == 'exact']
    return {
        'inventory': str(INVENTORY.relative_to(INVENTORY.parents[2])),
        'contributions': len(verdicts),
        'exact': len(exact),
        'exact_bytes': sum(v.row.size for v in exact),
        'findings': [dict(rva=f'{v.row.rva:#x}', size=v.row.size, library=v.row.library,
                          member=v.row.member, section=v.row.section, symbol=v.row.symbol,
                          verdict=v.verdict, reasons=v.reasons)
                     for v in verdicts if v.verdict != 'exact'],
        'game_comdats': [dict(name=name, rva=f'{rva:#x}', object=obj)
                         for (name, rva), obj in sorted((game._cache.items() if game else ()),
                                                        key=lambda item: item[0][::-1])
                         if obj],
        'data': sorted((_data_row(*row) for row in data), key=lambda r: int(r['rva'], 16)),
    }


def main(argv=None) -> int:
    import argparse
    import json
    parser = argparse.ArgumentParser(prog='homm3 verify library-code', description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--tsv', action='store_true', help='per-contribution verdicts')
    parser.add_argument('--data', action='store_true', help='implied library data placements')
    parser.add_argument('--gate', action='store_true', help='nonzero unless every row is exact')
    args = parser.parse_args(argv)
    from homm3.core.pe import image
    from homm3.model import resolve
    from homm3.verify.byte_accounting import library_ranges
    ranges, report, _names = library_ranges(image(), resolve())
    if args.data:
        print('kind\tname\trva\tfits\treferences')
        for row in report['data']:
            print(f"{row['kind']}\t{row['name']}\t{row['rva']}\t{int(row['fits'])}\t{row['references']}")
        return 0
    if args.tsv:
        print(json.dumps(report['findings'], indent=1))
        return 0
    covered = sum(r.end - r.start for r in ranges)
    print(f"library code: {report['exact']}/{report['contributions']} contributions exact, "
          f"{report['exact_bytes']:,} section bytes; {covered:,} bytes credited "
          f"including alignment fill")
    print(f"  {len(report['game_comdats'])} reference(s) resolved to game-emitted COMDATs; "
          f"{len(report['data'])} library data placement(s) implied")
    for row in report['findings']:
        print(f"  {row['rva']} {row['library']}:{row['member']}#{row['section']} "
              f"{row['symbol']}: {row['verdict']} - {'; '.join(row['reasons'][:3])}")
    return 1 if args.gate and report['findings'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
