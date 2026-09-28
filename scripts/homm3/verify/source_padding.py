"""Credit VC6-emitted COMDAT alignment bytes after source-owned code bodies.

VC6 pads every function COMDAT to its section alignment inside the section's
raw payload. The bytes after a reviewed retail extent are credited only when a
current raw object emits the same bytes at the same section offsets, and the
padded section ends exactly at the next reviewed function boundary. Retail
bytes that are zero, NOP or INT3 are never treated as padding by content:
without a matching emitted section the gap stays missing.
"""
from __future__ import annotations

from bisect import bisect_right
from collections import defaultdict
from dataclasses import dataclass
from hashlib import sha256

from homm3.core import compile_receipt, msvc_names
from homm3.delink.coffx import Obj

CODE = 0x20
COMDAT_CODE_FILL = 0x90


@dataclass(frozen=True)
class Entry:
    """One retail body whose emitting object and symbol are already known."""
    unit: str
    symbol: str
    rva: int
    size: int
    owner: str


class Objects:
    """Raw objects with a current content receipt, loaded once per unit."""

    def __init__(self, project):
        from homm3.core.cc_wrap import scan_header_deps
        self.project = project
        self.units = {u['unit']: u for u in project.manifest['unit']}
        self.compiler = [p for p in (project.toolchain / 'bin').iterdir()
                         if p.is_file() and p.suffix.lower() in ('.exe', '.dll')]
        self.scan = scan_header_deps
        self.hashes, self.loaded, self.receipts, self.aliases = {}, {}, {}, {}

    def path(self, unit):
        return self.project.root / f'build/objdiff/base/{unit}.obj'

    def get(self, unit):
        if unit in self.loaded:
            return self.loaded[unit]
        self.loaded[unit] = None
        spec, path = self.units.get(unit), self.path(unit)
        if spec is None or not path.is_file():
            return None
        root = self.project.root
        required = [root / spec['source'], root / 'config/units.toml',
                    root / 'config/project.toml', *self.compiler,
                    *self.scan(root / spec['source'], self.project.toolchain / 'include',
                               *self.project.includes)]
        flags = self.project.manifest['flags'][spec['flags']]
        inputs = compile_receipt.current(path, flags=flags, required=required,
                                         hashes=self.hashes)
        obj = Obj(path)
        if inputs is None or compile_receipt.digest(path) != sha256(obj.buf).hexdigest():
            return None
        self.receipts[unit] = (path, flags, inputs, sha256(obj.buf).hexdigest())
        self.loaded[unit] = inspect(obj)
        return self.loaded[unit]

    def compgen_symbol(self, unit, name):
        """Raw emitted name for a compiler-function claim, or None.

        Normalization renames anonymous compiler functions (`$E<n>`) to the
        source claim's `__h3cg$` spelling and records the pair in its sidecar.
        The pair is used only when the stamp names this exact raw object and
        the sidecar content is the stamped one.
        """
        import csv
        import json
        if unit not in self.aliases:
            self.aliases[unit] = {}
            base = self.project.root / 'build/objdiff/normalized/base'
            stamp, sidecar = base / f'{unit}.obj.stamp.json', base / f'{unit}.symbols.tsv'
            try:
                record = json.loads(stamp.read_text())
                raw = record['inputs']['raw']['sha256']
                if (unit in self.receipts and raw == self.receipts[unit][3]
                        and record['sidecar_sha256'] == compile_receipt.digest(sidecar)):
                    with sidecar.open(newline='') as stream:
                        for row in csv.DictReader(stream, delimiter='\t'):
                            if row['family'] == 'compgen':
                                self.aliases[unit].setdefault(
                                    row['canonical_name'], set()).add(row['original_name'])
            except (OSError, ValueError, KeyError):
                pass
        names = self.aliases[unit].get(name, set())
        return next(iter(names)) if len(names) == 1 else None

    def fresh(self):
        """Units whose receipts still hold after the whole comparison."""
        hashes, result = {}, set()
        for unit, (path, flags, inputs, object_hash) in self.receipts.items():
            if (compile_receipt.current(path, flags=flags, required=inputs,
                                        hashes=hashes) == inputs
                    and compile_receipt.digest(path) == object_hash):
                result.add(unit)
        return result


def inspect(obj):
    """(object, defined symbols, sections with interior definitions, fill proof)."""
    symbols, interior = defaultdict(list), set()
    for index, value, section in obj.iter_symbols():
        if section > 0:
            symbols[msvc_names.mask(obj.sym_name(index))].append((value, section))
            # Code labels (class 6) are not separate definitions.
            if value and obj.buf[obj.symptr + index*18 + 16] in (2, 3):
                interior.add(section)
    return obj, symbols, interior, fills_code(obj)


def fills_code(obj):
    """Whether the object's code COMDATs pad to alignment with NOP fill.

    Proof comes from the object itself: some code section's final
    instruction decodes as `ret`/`jmp` followed only by fill bytes up to the
    section's aligned length.
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    for section in obj.section_table:
        if not section['characteristics'] & CODE or section['name'] != '.text':
            continue
        raw = obj.section_payload(section['index']) or b''
        body = raw.rstrip(bytes([COMDAT_CODE_FILL]))
        if (not body or body == raw or len(raw) % section['alignment']
                or len(raw) - len(body) >= section['alignment']):
            continue
        last = None
        for address, size, mnemonic, _ in disassembler.disasm_lite(body, 0):
            last = (address + size, mnemonic)
        if last and last[0] == len(body) and last[1] in ('ret', 'jmp'):
            return True
    return False


def section_of(loaded, symbol):
    """The symbol's own exclusive code section, or None."""
    obj, symbols, interior, _ = loaded
    hits = symbols.get(msvc_names.mask(symbol), [])
    if len(hits) != 1 or hits[0][0] != 0:
        return None
    number = hits[0][1]
    section = obj.section_table[number-1]
    if not section['characteristics'] & CODE:
        return None
    # A section defining another symbol is not one body plus its padding.
    if number in interior:
        return None
    return number, section


def emitted_padding(entry, loaded, pe, relocs_in, starts):
    """Return ((start, end), kind, reason) for the bytes after one body.

    `exact`: the emitted section is exactly as long as the retail span up to
    the next reviewed boundary, and every byte past the retail extent agrees.
    `aligned`: the emitted section of the same function is aligned the same
    way but its body length differs (a non-exact implementation). The gap is
    then credited only when the retail extent is placed on that alignment,
    the next reviewed start is the very next aligned address, the object
    demonstrably pads its code COMDATs with the same fill byte, and every
    retail gap byte is that fill. Relocations never occur in either kind.
    """
    found = section_of(loaded, entry.symbol)
    if not found:
        return None, '', 'no exclusive emitted code section'
    number, section = found
    obj = loaded[0]
    raw = obj.section_payload(number) or b''
    alignment = section['alignment']
    start = entry.rva + entry.size
    following = -(-start // alignment) * alignment
    index = bisect_right(starts, entry.rva)
    if entry.rva % alignment:
        return None, '', 'retail placement disagrees with the section alignment'
    if following == start:
        return None, '', 'retail extent already ends aligned'
    if index >= len(starts) or starts[index] != following:
        return None, '', 'next reviewed boundary is not the next aligned address'
    if relocs_in(start, following):
        return None, '', 'retail relocation inside the padding'
    retail = pe.read(start, following - start)
    if (len(raw) == following - entry.rva and retail == raw[entry.size:]
            and all(offset + 4 <= entry.size for offset in obj.typed_relocations(number))):
        return (start, following), 'exact', ''
    if not loaded[3]:
        return None, '', 'object does not demonstrate its COMDAT code fill'
    if retail != bytes([COMDAT_CODE_FILL]) * (following - start):
        return None, '', 'retail bytes are not the compiler COMDAT fill'
    return (start, following), 'aligned', ''


def compare(project, pe, entries, objects=None):
    """Check each retail body in its claiming units, then recheck receipts.

    Entries at one retail address are alternative witnesses (a header body
    claimed from several units); the first verified witness is credited.
    """
    from homm3.delink.image import Image
    from homm3.retail_labels.censuses import functions
    starts = sorted(row['rva'] for row in functions())
    image = Image(pe)
    objects = objects or Objects(project)
    by_rva = defaultdict(list)
    for entry in entries:
        by_rva[entry.rva].append(entry)
    matches, withheld = [], []
    for rva, options in sorted(by_rva.items()):
        reasons = []
        for entry in options:
            loaded = objects.get(entry.unit)
            if loaded is None:
                reasons.append(f'{entry.unit}: raw object missing or stale')
                continue
            if entry.symbol.startswith('__h3cg$'):
                emitted = objects.compgen_symbol(entry.unit, entry.symbol)
                if emitted is None:
                    reasons.append(f'{entry.unit}: compiler-function claim has no '
                                   'stamped emitted symbol')
                    continue
                entry = Entry(entry.unit, emitted, entry.rva, entry.size, entry.owner)
            extent, kind, reason = emitted_padding(entry, loaded, pe,
                                                   image.relocs_in, starts)
            if extent:
                matches.append(dict(entry.__dict__, start=extent[0], end=extent[1],
                                    kind=kind))
                break
            reasons.append(f'{entry.unit}: {reason}')
        else:
            withheld.append(dict(options[0].__dict__, reason='; '.join(reasons)))
    fresh = objects.fresh()
    for row in matches:
        if row['unit'] not in fresh:
            withheld.append(dict(row, reason='source inputs changed during comparison'))
    matches = [m for m in matches if m['unit'] in fresh]
    return dict(matches=matches, withheld=withheld)


def function_entries(model):
    """Source-owned function bodies with each claiming unit as a witness."""
    result = []
    for b in model.functions:
        if b.channel not in ('src', 'src_compgen', 'src_dyninit') or not b.size:
            continue
        seen = []
        for unit in [b.unit, *(a.unit for a in b.aliases)]:
            if unit and unit not in seen:
                seen.append(unit)
                result.append(Entry(unit, b.name, b.rva, b.size, f'{unit}:{b.name}'))
    return result


# VC6 LINK 6.00.8447 fills the alignment gap between separate code
# contributions with INT3 (0xcc); the compiler's own NOP fill lives inside a
# COMDAT's raw payload. docs/tooling/data-matching.md records the link test.
LINK_CODE_FILL = 0xcc
STUB_SIZE = 10          # `mov eax, FuncInfo` + `jmp ___CxxFrameHandler`
REL32, DIR32 = 20, 6


def eh_section(loaded, parent):
    """The parent COMDAT's associative `.text$x` section, or None."""
    found = section_of(loaded, parent)
    if not found:
        return None
    obj = loaded[0]
    hits = [s for s in obj.section_table if s['name'] == '.text$x'
            and s['comdat'] == 5 and s['assoc'] == found[0]]
    return hits[0] if len(hits) == 1 else None


def eh_padding(group, name, loaded, pe, relocs_in, starts):
    """Return the linker fill after one source `.text$x` contribution.

    VC6 emits each parent's cleanup funclets followed by its registration
    stub in one associative `.text$x` section, so the retail contribution
    ends with the decoded stub. LINK then aligns the next contribution to 16
    bytes and fills the gap with INT3. The gap must reach exactly that
    aligned start, which must itself be a reviewed contribution start.
    Whether the emitted cleanup bytes also agree is reported separately.
    """
    section = eh_section(loaded, name)
    if not section:
        return None, 'no associative .text$x section', ''
    obj = loaded[0]
    raw = obj.section_payload(section['index']) or b''
    relocations = obj.typed_relocations(section['index'])
    size = len(raw)
    if (size < STUB_SIZE or relocations.get(size - 4, ('', 0))[1] != REL32
            or relocations.get(size - 9, ('', 0))[1] != DIR32
            or raw[size - STUB_SIZE] != 0xb8 or raw[size - 5] != 0xe9):
        return None, 'emitted section does not end with its registration stub', ''
    end = group.stub + STUB_SIZE
    following = -(-end // 16) * 16
    if following == end:
        return None, 'contribution already ends aligned', ''
    if following not in starts:
        return None, 'gap does not reach a reviewed contribution start', ''
    if pe.read(end, following - end) != bytes([LINK_CODE_FILL]) * (following - end):
        return None, 'gap is not the linker code fill', ''
    start = end - size
    retail = pe.read(start, size)
    covered = set()
    for offset in relocations:
        covered.update(range(offset, offset + 4))
    required = {offset for offset, (_, kind) in relocations.items() if kind == DIR32}
    if start % section['alignment'] or start != group.start:
        content = 'emitted section length differs from the retail cleanup group'
    elif retail is None or any(raw[i] != retail[i] for i in range(size) if i not in covered):
        content = 'emitted unrelocated bytes differ from retail'
    elif {site - start for site in relocs_in(start, end)} != required:
        content = 'absolute relocation sites differ'
    else:
        content = 'exact'
    return (end, following), '', content


def compare_eh(project, pe, groups, objects=None):
    """Check linker fill after each verified source `.text$x` contribution."""
    from homm3.delink.image import Image
    from homm3.retail_labels.censuses import functions
    image = Image(pe)
    starts = {row['rva'] for row in functions()} | {g.stub for g, _, _ in groups}
    objects = objects or Objects(project)
    matches, withheld = [], []
    for group, name, unit in groups:
        loaded = objects.get(unit)
        entry = dict(unit=unit, symbol=name, rva=group.stub,
                     owner=f'{unit}:.text$x of {name}')
        if loaded is None:
            withheld.append(dict(entry, reason='raw object missing or stale'))
            continue
        extent, reason, content = eh_padding(group, name, loaded, pe,
                                             image.relocs_in, starts)
        if extent:
            matches.append(dict(entry, start=extent[0], end=extent[1],
                                contribution=content))
        else:
            withheld.append(dict(entry, reason=reason))
    fresh = objects.fresh()
    for row in matches:
        if row['unit'] not in fresh:
            withheld.append(dict(row, reason='source inputs changed during comparison'))
    return dict(matches=[m for m in matches if m['unit'] in fresh], withheld=withheld)
