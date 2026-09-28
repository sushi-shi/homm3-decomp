"""Linker-produced structures: the import tables, bounded and verified.

VC6 LINK merges `.idata` into `.rdata`: the IAT (`.idata$5`) opens `.rdata`
at the IAT data directory, and the descriptors (`.idata$2`), their null
terminator (`.idata$3`), the lookup tables (`.idata$4`) and the hint/name
records and DLL names (`.idata$6`) sit at the import data directory. Every
structure is bounded by the PE import format itself: descriptors up to the
null descriptor, each lookup table and IAT up to its zero terminator, each
hint/name record and DLL name up to its NUL, padded to an even address.

A structure is `linker-import` when the image reproduces it from the pinned
toolchain import libraries: the IAT equals the lookup table (the image is
not bound), each by-name import is a member of the pinned library for that
DLL with the same hint and name, each ordinal import is that library's own
ordinal binding, the DLL name is the library's spelling, and every IAT slot
is referenced by a reviewed relocation. Anything the PE format bounds but a
pinned library cannot reproduce (vendor DLLs whose import libraries are not
pinned, hints or names that differ from the pinned library) is
`import-structure`, and each reason is reported. Bytes inside the import
region that no structure reaches remain missing.
"""

from __future__ import annotations

import struct
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path

from homm3.core.paths import RETAIL, msvc_dir
from homm3.core.tsv import read as read_tsv

IMPORT_DIRECTORY, IAT_DIRECTORY = 1, 12


@dataclass
class Record:
    start: int
    size: int
    kind: str                 # descriptor | null-descriptor | lookup | iat | hint-name | dll-name | pad
    name: str
    reasons: list = field(default_factory=list)

    @property
    def category(self):
        return 'import-structure' if self.reasons else 'linker-import'


# ------------------------------------------------------------ import libs --

def _import_members(path: Path):
    """{(dll lowercase, name or '#ordinal'): (dll, hint, library)} of one import library."""
    from homm3.compare.canonicalize import CoffObject
    from homm3.delink.implib import _short_import
    from homm3.verify.library_code import archive_members
    out = {}
    for member, body in archive_members(path):
        short = _short_import(body) if body[:4] == b'\0\0\xff\xff' else None
        if short is not None:
            symbol, dll, hint, name_type = short
            if name_type == 0:
                key = f'#{hint}'
            elif name_type == 1:
                key = symbol
            else:
                key = symbol[1:] if symbol[:1] in ('_', '@', '?') and name_type in (2, 3) else symbol
                if name_type == 3:
                    key = key.split('@')[0]
            out[(dll.lower(), key)] = (dll, hint if name_type else None, path.name)
            continue
        try:
            obj = CoffObject(body)
        except (ValueError, struct.error):
            continue
        dll = member
        for sec in obj.sections:
            data = obj.section_bytes(sec)
            if sec.name == '.idata$6' and len(data) > 2 and \
                    any(r.section == sec.index for r in obj.relocations) is False:
                hint = struct.unpack_from('<H', data, 0)[0]
                name = data[2:data.index(b'\0', 2)].decode('latin1')
                if name and not name.lower().endswith('.dll'):
                    out[(dll.lower(), name)] = (dll, hint, path.name)
            if sec.name == '.idata$4' and len(data) >= 4:
                value = struct.unpack_from('<I', data, 0)[0]
                if value & 0x80000000:
                    out[(dll.lower(), f'#{value & 0xffff}')] = (dll, None, path.name)
    return out


def pinned_imports(root: Path | None = None):
    table = {}
    root = Path(root or msvc_dir()) / 'lib'
    for path in sorted(root.glob('*.LIB')) + sorted(root.glob('*.lib')):
        try:
            for key, value in _import_members(path).items():
                table.setdefault(key, value)
        except (ValueError, OSError):
            continue
    return table


# --------------------------------------------------------------- imports --

def _directory(data, index):
    header = struct.unpack_from('<I', data, 0x3c)[0]
    return struct.unpack_from('<II', data, header + 24 + 96 + 8 * index)


def import_records(pe, pinned=None, referenced=None):
    """Records tiling the import tables, with the reasons any is not exact."""
    pinned = pinned if pinned is not None else pinned_imports()
    data = pe.data

    def raw(rva, size):
        chunk = pe.read(rva, size)
        if chunk is None:
            raise ValueError(f'import structure outside the image at {rva:#x}')
        return chunk

    def cstring(rva):
        out = bytearray()
        while True:
            byte = raw(rva + len(out), 1)
            if byte == b'\0':
                return bytes(out)
            out += byte

    directory, _size = _directory(data, IMPORT_DIRECTORY)
    iat_directory, iat_size = _directory(data, IAT_DIRECTORY)
    records = []
    index = 0
    while True:
        at = directory + 20 * index
        lookup, stamp, forwarder, name_rva, iat = struct.unpack('<IIIII', raw(at, 20))
        if not any((lookup, stamp, forwarder, name_rva, iat)):
            records.append(Record(at, 20, 'null-descriptor', 'import directory terminator'))
            break
        index += 1
        dll = cstring(name_rva).decode('latin1')
        descriptor = Record(at, 20, 'descriptor', dll)
        if forwarder:
            descriptor.reasons.append(f'ForwarderChain {forwarder:#x}')
        if stamp:
            # An unbound image has zero here; LINK writes zero.
            descriptor.reasons.append(f'TimeDateStamp {stamp:#x} is not written by LINK')
        records.append(descriptor)
        dll_record = Record(name_rva, len(dll) + 1, 'dll-name', dll)
        records.append(dll_record)
        entries = []
        while True:
            value = struct.unpack('<I', raw(lookup + 4 * len(entries), 4))[0]
            if not value:
                break
            entries.append(value)
        lookup_record = Record(lookup, 4 * (len(entries) + 1), 'lookup', dll)
        iat_record = Record(iat, 4 * (len(entries) + 1), 'iat', dll)
        if raw(iat, iat_record.size) != raw(lookup, lookup_record.size):
            iat_record.reasons.append('IAT differs from the lookup table (bound image?)')
        if not iat_directory <= iat < iat + iat_record.size <= iat_directory + iat_size:
            iat_record.reasons.append('IAT outside the IAT data directory')
        records += [lookup_record, iat_record]
        spellings = set()
        for slot, value in enumerate(entries):
            slot_rva = iat + 4 * slot
            if referenced is not None and slot_rva not in referenced:
                iat_record.reasons.append(f'slot {slot_rva:#x} is not referenced')
            if value & 0x80000000:
                key = f'#{value & 0xffff}'
                hit = pinned.get((dll.lower(), key))
                if hit is None:
                    iat_record.reasons.append(f'{dll} ordinal {value & 0xffff} not in a pinned library')
                else:
                    spellings.add(hit[0])
                continue
            hint = struct.unpack('<H', raw(value, 2))[0]
            name = cstring(value + 2).decode('latin1')
            size = 2 + len(name) + 1
            record = Record(value, size, 'hint-name', f'{dll}!{name}')
            hit = pinned.get((dll.lower(), name))
            if hit is None:
                record.reasons.append('no pinned import library exports it')
            else:
                spellings.add(hit[0])
                if hit[1] is not None and hit[1] != hint:
                    record.reasons.append(f'hint {hint} != pinned {hit[2]} hint {hit[1]}')
            records.append(record)
        if not spellings:
            dll_record.reasons.append('no pinned import library for this DLL')
        elif dll not in spellings:
            dll_record.reasons.append(f'DLL name differs from the pinned library ({sorted(spellings)})')
    # The linker pads each .idata$6 record to an even address with a zero.
    ends = sorted((r.start + r.size) for r in records if r.kind in ('hint-name', 'dll-name'))
    starts = {r.start for r in records}
    for end in ends:
        if end % 2 and end + 1 in starts and raw(end, 1) == b'\0':
            records.append(Record(end, 1, 'pad', 'even alignment of .idata$6'))
    records.sort(key=lambda r: (r.start, r.size))
    overlaps = [(a.start, b.start) for a, b in zip(records, records[1:])
                if b.start < a.start + a.size]
    if overlaps:
        raise ValueError(f'overlapping import structures: {overlaps[:4]}')
    return records


def referenced_slots():
    """IAT slots named by reviewed relocations (instruction operands or thunks)."""
    rows = read_tsv(RETAIL / 'reloc-evidence.tsv')[2]
    return {int(r['value'], 16) - 0x400000 for r in rows if r['disposition'].startswith('kept')}


def summary(records):
    by = Counter()
    reasons = Counter()
    for r in records:
        by[r.category] += r.size
        for why in r.reasons:
            reasons[why.split(' ')[0] if why.startswith(('slot', 'hint')) else why] += 1
    return {'bytes': dict(by), 'records': len(records),
            'findings': [dict(start=f'{r.start:#x}', size=r.size, kind=r.kind, name=r.name,
                              reasons=r.reasons) for r in records if r.reasons]}


# ------------------------------------------------------------ CRT tables --

def crt_slots(pe, exact_bodies, library_symbols, library_ranges, startup=None,
              base_dir=None):
    """Game `.CRT$XCU` entries between `___xc_a` and the library entries.

    `__initterm` walks every word from `___xc_a` to `___xc_z`; both markers
    and the library entries are verified library sections, so every other
    word in between is a game initializer pointer. A slot is verified when
    its pointer is the start of an exact initializer body. A startup body's
    own unit must emit a `.CRT$XCU` relocation to the matched symbol; a
    header copy's slots must repeat one owner sequence block by block.
    Returns (verified rows [(start, end, identity)], findings).
    """
    import bisect
    from homm3.compare.canonicalize import CoffObject
    from homm3.core.paths import BUILD
    names = {name: rva for rva, ns in library_symbols.items() for name in ns}
    xc_a, xc_z = names.get('___xc_a'), names.get('___xc_z')
    if xc_a is None or xc_z is None:
        return [], ['CRT markers ___xc_a/___xc_z are not verified']
    covered = sorted(library_ranges)
    starts = [lo for lo, _hi in covered]

    def library_word(address):
        k = bisect.bisect_right(starts, address) - 1
        return k >= 0 and covered[k][0] <= address and address + 4 <= covered[k][1]
    base = pe.image_base
    slots = [a for a in range(xc_a + 4, xc_z, 4) if not library_word(a)]
    bodies = dict(exact_bodies)                   # rva -> (identity, owner key or None)
    emitted = {}
    root = Path(base_dir or BUILD / 'objdiff/base')

    def unit_xcu(unit):
        if unit not in emitted:
            path = root / f'{unit}.obj'
            names_ = set()
            if path.is_file():
                obj = CoffObject(path.read_bytes())
                names_ = {obj.symbols[r.symbol_index].name for r in obj.relocations
                          if obj.sections[r.section - 1].name == '.CRT$XCU'}
            emitted[unit] = names_
        return emitted[unit]

    rows, findings, blocks, block = [], [], [], []
    for index, address in enumerate(slots):
        target = struct.unpack('<I', pe.read(address, 4))[0] - base
        body = bodies.get(target)
        if body is None:
            findings.append(f'slot {index} {address:#x} -> {target:#x}: no exact initializer body')
            if block:
                blocks.append(block); block = []
            continue
        identity, header_owner, unit, symbol = body
        if unit and symbol and symbol not in unit_xcu(unit):
            findings.append(f'slot {index} {address:#x}: {unit} emits no .CRT$XCU entry for {symbol}')
            continue
        if header_owner:
            block.append((address, header_owner, identity))
            continue
        if block:
            blocks.append(block); block = []
        rows.append((address, address + 4, f'.CRT$XCU slot {index} -> {identity}'))
    if block:
        blocks.append(block)
    if blocks:
        reference = [owner for _a, owner, _i in max(blocks, key=len)]
        for block in blocks:
            owners = [owner for _a, owner, _i in block]
            if owners != reference:
                findings.append(f'header initializer block at {block[0][0]:#x} is not in '
                                f'declaration order ({len(owners)} of {len(reference)})')
                continue
            rows += [(a, a + 4, f'.CRT$XCU slot -> {i}') for a, _o, i in block]
    return rows, findings


def crt_group_fill(pe, library_symbols, next_alignment):
    """The zero fill between `___xt_z` (end of the `.CRT$` group) and `.data`."""
    names = {name: rva for rva, ns in library_symbols.items() for name in ns}
    end = names.get('___xt_z')
    if end is None:
        return []
    end += 4
    align = next_alignment(end)
    if not align:
        return []
    gap = (-end) % align
    if gap and pe.read(end, gap) == bytes(gap):
        return [(end, end + gap, f'.CRT$ group fill before the {align}-aligned .data')]
    return []


# ---------------------------------------------------------- section tails --

def data_alignment_tails(pe):
    """Header-proven zero tails of initialized data sections.

    As for code sections: the bytes between VirtualSize and the raw
    FileAlignment end (file and image), and the loader zero fill up to the
    SectionAlignment boundary when the next section starts there. Only zero
    bytes count; `.rsrc`/`.reloc` are already structural.
    """
    data = pe.data
    header = struct.unpack_from('<I', data, 0x3c)[0]
    section_alignment, file_alignment = struct.unpack_from('<II', data, header + 24 + 32)
    image_size = struct.unpack_from('<I', data, header + 24 + 56)[0]
    sections = pe.sections
    file_ranges, image_ranges = [], []
    for i, s in enumerate(sections):
        if s['name'] in ('.rsrc', '.reloc', '.text'):
            continue
        va, vsize, rsize, rptr = s['va'], s['vsize'], s['rsize'], s['rptr']
        following = sections[i + 1] if i + 1 < len(sections) else None
        virtual_end = -(-vsize // section_alignment) * section_alignment
        if (following['va'] if following else image_size) != va + virtual_end:
            continue
        if rsize > vsize:
            raw_end = -(-vsize // file_alignment) * file_alignment
            if rsize != raw_end or data[rptr + vsize:rptr + rsize] != bytes(rsize - vsize):
                continue
            file_ranges.append((rptr + vsize, rptr + rsize, f'{s["name"]} FileAlignment tail'))
            image_ranges.append((va + vsize, va + rsize, f'{s["name"]} FileAlignment tail'))
            if va + rsize < va + virtual_end:
                image_ranges.append((va + rsize, va + virtual_end, f'{s["name"]} SectionAlignment tail'))
        elif va + vsize < va + virtual_end:
            image_ranges.append((va + vsize, va + virtual_end, f'{s["name"]} SectionAlignment tail'))
    return file_ranges, image_ranges
