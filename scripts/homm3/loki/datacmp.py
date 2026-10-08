"""Data comparison of the Loki h3maped image: each project object's data
against its slice of the linked image.

GNU ld 2.9.5's default script concatenates input sections in link order:
`.rodata` is every object's `.rodata` and then the kept `.gnu.linkonce.r*`,
`.data` every `.data` and then the kept `.gnu.linkonce.d*` (vtables, SGI
allocator statics), `.bss` the copy-relocated `.dynbss`, every `.bss` and
then COMMON (g++ 2.95's `__ti` type_info nodes). `.gcc_except_table`,
`.ctors` and `.dtors` follow link order too. So a compiled object's
section maps to one base address in the image:

- `.text` starts at the object's census span, a kept linkonce body at its
  census address, the object's `.eh_frame` at its CIE;
- a data section's base is voted by every absolute field that relocates
  against it from code and frames whose base is known (the image holds the
  linked address, the object the section offset), and from exported
  symbols defined in it; a section nothing refers to follows the previous
  object's, aligned;
- a kept linkonce section sits at its symbol's exported address; it is
  compared in the first project object defining it, as the linker keeps
  that copy.

Each slice runs from the object's base to the next object's base in the same
output section (the compiled bytes are zero-padded to it, as ld pads), so
missing or extra data, wrong order and wrong alignment all show as differing
bytes. A relocated field matches when the image word is the address of the
same target: the paired symbol, or the same offset in the object's own
section. A `.bss` byte matches while every code reference into its symbol
votes for the section's base. Jump tables (`.rodata` words relocated to a
label inside a function) are compared with their function and excluded here.
"""
from __future__ import annotations

import collections
from dataclasses import dataclass, field
import hashlib
from pathlib import Path
import re
import struct

from homm3.loki import cmpobj
from homm3.loki.elf import (Elf, R_386_32, SHF_ALLOC, SHF_EXECINSTR, SHT_NOBITS, SHT_REL, SHT_SYMTAB,
                            STB_LOCAL, STT_FUNC, STT_OBJECT, STT_SECTION)
from homm3.loki.ehframe import frames
from homm3.loki.image import LokiImage

SHN_COMMON = 0xfff2
# Output sections whose per-object inputs follow link order.
SEQUENTIAL = (".rodata", ".data", ".bss", ".gcc_except_table", ".ctors", ".dtors")
LINKONCE = {".gnu.linkonce.d.": ".data", ".gnu.linkonce.r.": ".rodata"}
_ANONYMOUS = re.compile(r"(_GLOBAL_\.N\.[\w.+-]*?\.(?:cpp|cc|c))[A-Za-z0-9]{6}")


def symbol_key(name: str) -> str:
    """A symbol's name without the six random characters g++ 2.95 appends
    to an anonymous namespace (`_GLOBAL_.N.<file><random>`). The compiled
    and the retail file names agree (units compile by bare file name), so
    the length prefixes agree too."""
    return _ANONYMOUS.sub(r"\1######", name)


@dataclass
class Item:
    """One compared symbol, or one anonymous gap between symbols."""
    section: str
    name: str
    offset: int
    size: int
    matched: int = 0
    excluded: int = 0

    @property
    def total(self) -> int:
        return self.size - self.excluded


@dataclass
class UnitData:
    unit: str
    obj: int
    items: list[Item] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)
    fingerprint: str = "-"

    @property
    def matched(self) -> int:
        return sum(i.matched for i in self.items)

    @property
    def total(self) -> int:
        return sum(i.total for i in self.items)

    def differing(self) -> list[Item]:
        return [i for i in self.items if i.matched < i.total]


class ImageIndex:
    def __init__(self, image: LokiImage):
        self.image = image
        elf = image.elf
        self.sections = {s.name: s for s in elf.sections}
        by_key: dict[str, set[tuple[int, int]]] = collections.defaultdict(set)
        for symbol in elf.dynsym:
            if symbol.shndx and symbol.bind != STB_LOCAL:
                by_key[symbol_key(symbol.name)].add((symbol.value, symbol.size))
        self.symbols = {k: next(iter(v)) for k, v in by_key.items() if len(v) == 1}
        self.plt = {name: address for address, name in elf.plt_slots.items()}
        cies, _ = frames(elf)
        self.cies = cies

    def address(self, name: str) -> int | None:
        found = self.symbols.get(symbol_key(name))
        if found is not None:
            return found[0]
        return self.plt.get(name)

    def read(self, address: int, size: int) -> bytes:
        section = self.image.elf.section_at(address)
        if section is None:
            return b""
        size = min(size, section.addr + section.size - address)
        if section.type == SHT_NOBITS:
            return bytes(size)
        return self.image.elf.read(address, size)

    def word(self, address: int) -> int | None:
        raw = self.read(address, 4)
        return struct.unpack("<I", raw)[0] if len(raw) == 4 else None


@dataclass
class Compiled:
    unit: str
    obj: int
    elf: Elf
    symbols: list
    rels: dict[int, list]


def load(unit: str, obj: int, path: Path) -> Compiled | None:
    if not path.is_file():
        return None
    elf = Elf(path.read_bytes())
    symtab = next(s for s in elf.sections if s.type == SHT_SYMTAB)
    rels = {s.info: elf.rels(s) for s in elf.sections if s.type == SHT_REL}
    return Compiled(unit, obj, elf, elf.symbols(symtab), rels)


def linkonce_kind(name: str) -> str | None:
    for prefix, output in LINKONCE.items():
        if name.startswith(prefix):
            return output
    return None


def is_data(section) -> bool:
    return bool(section.flags & SHF_ALLOC) and not section.flags & SHF_EXECINSTR and section.size > 0 \
        and section.name != ".eh_frame"


class Layout:
    """Image base of every compiled section of the project objects."""

    def __init__(self, objects: list[Compiled], index: ImageIndex, text_start: dict[int, int],
                 functions: dict[str, tuple[int, int]]):
        self.objects = sorted(objects, key=lambda c: c.obj)
        self.index = index
        self.base: dict[tuple[int, int], int | None] = {}
        self.votes: dict[tuple[int, int], collections.Counter] = {}
        self.owner: dict[str, int] = {}
        self.discarded: set[tuple[int, int]] = set()
        self.common_owner: dict[str, int] = {}
        for compiled in self.objects:
            for header in compiled.elf.sections:
                if linkonce_kind(header.name) or header.name.startswith(".gnu.linkonce.t."):
                    self.owner.setdefault(symbol_key(header.name), compiled.obj)
            for symbol in compiled.symbols:
                if symbol.shndx == SHN_COMMON:
                    self.common_owner.setdefault(symbol_key(symbol.name), compiled.obj)
        eh = index.sections[".eh_frame"]
        for compiled in self.objects:
            for header in compiled.elf.sections:
                key = (compiled.obj, header.index)
                if header.name == ".text":
                    self.base[key] = text_start.get(compiled.obj)
                elif header.name.startswith(".gnu.linkonce.t."):
                    found = functions.get(symbol_key(header.name[len(".gnu.linkonce.t."):]))
                    self.base[key] = found[0] if found and found[1] == compiled.obj else None
                    if self.owner[symbol_key(header.name)] != compiled.obj:
                        self.discarded.add(key)
                elif header.name == ".eh_frame" and compiled.obj < len(index.cies):
                    self.base[key] = eh.addr + index.cies[compiled.obj]
                elif linkonce_kind(header.name):
                    if self.owner[symbol_key(header.name)] == compiled.obj:
                        first = self._defined_at(compiled, header.index, 0)
                        self.base[key] = index.address(first.name) if first is not None else None
                    else:
                        self.base[key] = None
                        self.discarded.add(key)
        self._vote()
        self._chain()

    @staticmethod
    def _defined_at(compiled: Compiled, shndx: int, offset: int):
        for symbol in compiled.symbols:
            if symbol.shndx == shndx and symbol.value == offset and symbol.type in (STT_OBJECT, STT_FUNC) \
                    and symbol.name:
                return symbol
        return None

    def _vote(self) -> None:
        for _ in range(2):   # a data section placed by the first pass votes in the second
            votes: dict[tuple[int, int], collections.Counter] = collections.defaultdict(collections.Counter)
            for compiled in self.objects:
                for index, rels in compiled.rels.items():
                    base = self.base.get((compiled.obj, index))
                    if base is None:
                        continue
                    raw = compiled.elf.bytes(compiled.elf.sections[index])
                    for rel in rels:
                        if rel.type != R_386_32:
                            continue
                        symbol = compiled.symbols[rel.symbol]
                        if not 0 < symbol.shndx < 0xff00:
                            continue
                        target = compiled.elf.sections[symbol.shndx]
                        if not is_data(target) or linkonce_kind(target.name):
                            continue
                        if symbol.type not in (STT_SECTION, STT_OBJECT):
                            continue
                        (addend,) = struct.unpack_from("<i", raw, rel.offset)
                        word = self.index.word(base + rel.offset)
                        if word is None:
                            continue
                        place = (0 if symbol.type == STT_SECTION else symbol.value) + addend
                        votes[(compiled.obj, symbol.shndx)][(word - place) & 0xffffffff] += 1
                for symbol in compiled.symbols:
                    if symbol.bind != STB_LOCAL and 0 < symbol.shndx < 0xff00 and symbol.type == STT_OBJECT:
                        header = compiled.elf.sections[symbol.shndx]
                        address = self.index.address(symbol.name)
                        if is_data(header) and not linkonce_kind(header.name) and address is not None:
                            votes[(compiled.obj, symbol.shndx)][(address - symbol.value) & 0xffffffff] += 4
            self.votes = votes
            for key, counter in votes.items():
                if self.base.get(key) is None:
                    self.base[key] = counter.most_common(1)[0][0]

    def sequence(self, output: str) -> list[tuple[Compiled, object]]:
        out = []
        for compiled in self.objects:
            for header in compiled.elf.sections:
                if header.name == output and header.size > 0:
                    out.append((compiled, header))
        return out

    def _chain(self) -> None:
        for output in SEQUENTIAL:
            previous = None
            for compiled, header in self.sequence(output):
                key = (compiled.obj, header.index)
                if self.base.get(key) is None:
                    self.base[key] = self._search(compiled, header, output)
                if self.base.get(key) is None:
                    if previous is not None:
                        align = max(header.align, 1)
                        self.base[key] = (previous + align - 1) // align * align
                    elif output in (".ctors", ".dtors"):
                        # crtbegin.o's list head (-1) precedes the first object's entries.
                        self.base[key] = self.index.sections[output].addr + 4
                base = self.base.get(key)
                previous = None if base is None else base + header.size

    def _search(self, compiled: Compiled, header, output: str) -> int | None:
        """The one place in the output section holding the section's bytes
        with every relocation resolved (`.ctors` entries, unreferenced
        tables with pointers)."""
        relocs = compiled.rels.get(header.index, ())
        if header.type == SHT_NOBITS or not relocs:
            return None
        expected = bytearray(compiled.elf.bytes(header))
        for rel in relocs:
            target = self.resolve(compiled, compiled.symbols[rel.symbol])
            if rel.type != R_386_32 or target is None:
                return None
            (addend,) = struct.unpack_from("<i", expected, rel.offset)
            struct.pack_into("<I", expected, rel.offset, (target + addend) & 0xffffffff)
        section = self.index.sections[output]
        data = self.index.image.elf.bytes(section)
        found = data.find(expected)
        while found >= 0 and found % max(header.align, 1):
            found = data.find(expected, found + 1)
        if found < 0 or data.find(expected, found + 1) >= 0:
            return None
        return section.addr + found

    def resolve(self, compiled: Compiled, symbol) -> int | None:
        if (compiled.obj, symbol.shndx) in self.discarded and symbol.bind == STB_LOCAL:
            # ld 2.9.5 sends a discarded linkonce section to address 0: what
            # still refers to it (exception ranges, local labels) keeps the
            # bare offset.
            return symbol.value if symbol.type != STT_SECTION else 0
        if symbol.type == STT_SECTION:
            return self.base.get((compiled.obj, symbol.shndx))
        if symbol.bind != STB_LOCAL or symbol.shndx in (0, SHN_COMMON):
            address = self.index.address(symbol.name)
            if address is not None or symbol.shndx in (0, SHN_COMMON):
                return address
        base = self.base.get((compiled.obj, symbol.shndx))
        return None if base is None else base + symbol.value


def _items(compiled: Compiled, header, size: int) -> list[Item]:
    """Symbols of a section, and the anonymous gaps between them, over `size` bytes."""
    marks = sorted({(s.value, s.size, s.name) for s in compiled.symbols
                    if s.shndx == header.index and s.type == STT_OBJECT and s.name})
    items, at = [], 0
    for value, length, name in marks:
        if value < at:
            continue
        if value > at:
            items.append(Item(header.name, f"{header.name}+0x{at:x}", at, value - at))
        length = max(length, 1)
        items.append(Item(header.name, name, value, length))
        at = value + length
    if at < size:
        items.append(Item(header.name, f"{header.name}+0x{at:x}", at, size - at))
    return items


def compare(compiled: Compiled, layout: Layout) -> UnitData:
    index = layout.index
    out = UnitData(compiled.unit, compiled.obj)
    digest = hashlib.sha1()
    starts = {f for f in layout_function_starts(layout)}
    for output in SEQUENTIAL + tuple(LINKONCE):
        for header in compiled.elf.sections:
            if not is_data(header):
                continue
            kind = linkonce_kind(header.name)
            if (kind is None and header.name != output) or (kind is not None and not header.name.startswith(output)):
                continue
            key = (compiled.obj, header.index)
            if kind is not None and layout.owner[symbol_key(header.name)] != compiled.obj:
                continue
            base = layout.base.get(key)
            size = header.size
            if kind is None:
                following = _next_base(layout, compiled, header)
                if base is not None and following is not None and base <= following <= base + 2 * header.size + 64:
                    size = following - base
                elif base is not None and following is not None:
                    out.notes.append(f"{header.name}: next object at 0x{following:x}, base 0x{base:x}")
            elif base is not None:
                first = Layout._defined_at(compiled, header.index, 0)
                found = index.symbols.get(symbol_key(first.name)) if first is not None else None
                if found is not None and found[1] > size:
                    size = found[1]
            items = _items(compiled, header, max(size, header.size))
            out.items += items
            raw = compiled.elf.bytes(header)
            digest.update(header.name.encode() + raw)
            if base is None:
                out.notes.append(f"{header.name}: no image base")
                continue
            expected = bytearray(raw) + bytes(max(0, size - len(raw)))
            known = bytearray(b"\1") * len(expected)
            excluded = bytearray(len(expected))
            for rel in compiled.rels.get(header.index, ()):
                symbol = compiled.symbols[rel.symbol]
                (addend,) = struct.unpack_from("<i", raw, rel.offset)
                digest.update(f"{rel.offset}:{rel.type}:{symbol_key(symbol.name)}:{addend};".encode())
                target = layout.resolve(compiled, symbol)
                target_header = compiled.elf.sections[symbol.shndx] if 0 < symbol.shndx < 0xff00 else None
                if (output == ".rodata" and target_header is not None and target_header.flags & SHF_EXECINSTR
                        and symbol.type == STT_SECTION and target is not None
                        and (target + addend) not in starts):
                    for i in range(4):
                        excluded[rel.offset + i] = 1
                if target is None:
                    for i in range(4):
                        known[rel.offset + i] = 0
                    continue
                struct.pack_into("<I", expected, rel.offset, (target + addend) & 0xffffffff)
            actual = index.read(base, size)
            bad = _bss_mismatches(compiled, header, layout, base) if header.type == SHT_NOBITS else set()
            for item in items:
                if any(item.offset <= place < item.offset + item.size for place in bad):
                    continue
                for at in range(item.offset, min(item.offset + item.size, size)):
                    if excluded[at]:
                        item.excluded += 1
                    elif at < len(actual) and known[at] and expected[at] == actual[at]:
                        item.matched += 1
            if size != header.size and kind is None:
                out.notes.append(f"{header.name}: compiled 0x{header.size:x} bytes, image slice 0x{size:x}")
    # COMMON symbols (type_info nodes): allocated by the linker; the name and size must agree.
    for symbol in compiled.symbols:
        if symbol.shndx != SHN_COMMON or layout.common_owner.get(symbol_key(symbol.name)) != compiled.obj:
            continue
        found = index.symbols.get(symbol_key(symbol.name))
        bss = index.sections[".bss"]
        if found is not None and not bss.contains(found[0]):
            continue   # defined by a library; the object only refers to it
        item = Item("COMMON", symbol.name, 0, symbol.size)
        if found is not None and found[1] == symbol.size:
            item.matched = symbol.size
        out.items.append(item)
        digest.update(f"COM:{symbol.name}:{symbol.size};".encode())
    out.fingerprint = digest.hexdigest()[:12]
    return out


_STARTS: dict[int, set[int]] = {}


def layout_function_starts(layout: Layout) -> set[int]:
    cached = _STARTS.get(id(layout))
    if cached is None:
        cached = {f.address for f in layout.index.image.functions}
        _STARTS.clear()
        _STARTS[id(layout)] = cached
    return cached


def _next_base(layout: Layout, compiled: Compiled, header) -> int | None:
    sequence = layout.sequence(header.name)
    for position, (other, other_header) in enumerate(sequence):
        if other.obj == compiled.obj and other_header.index == header.index:
            for later, later_header in sequence[position + 1:]:
                found = layout.base.get((later.obj, later_header.index))
                if found is not None:
                    return found
            return None
    return None


def _bss_mismatches(compiled: Compiled, header, layout: Layout, base: int) -> set[int]:
    """Offsets in a `.bss` whose code references vote for another base."""
    bad = set()
    for index, rels in compiled.rels.items():
        source = layout.base.get((compiled.obj, index))
        if source is None:
            continue
        raw = compiled.elf.bytes(compiled.elf.sections[index])
        for rel in rels:
            symbol = compiled.symbols[rel.symbol]
            if rel.type != R_386_32 or symbol.shndx != header.index:
                continue
            (addend,) = struct.unpack_from("<i", raw, rel.offset)
            place = (0 if symbol.type == STT_SECTION else symbol.value) + addend
            word = layout.index.word(source + rel.offset)
            if word is not None and (word - place) & 0xffffffff != base:
                bad.add(place)
    return bad


def run(objects: dict[str, tuple[int, Path]], selected: set[str] | None = None,
        image: LokiImage | None = None) -> list[UnitData]:
    """Compare the data of `selected` units (all when None); every available
    project object takes part in the layout (linkonce ownership, slices)."""
    from homm3.loki.delink import census
    from homm3.loki.delink import objects as census_objects
    image = image or LokiImage()
    index = ImageIndex(image)
    loaded = [c for name, (obj, path) in objects.items() if (c := load(name, obj, path)) is not None]
    text_start = {obj: start for obj, (start, _end, _file) in census_objects().items()}
    functions = {symbol_key(f.name): (f.address, f.obj) for f in census() if f.name}
    layout = Layout(loaded, index, text_start, functions)
    return [compare(c, layout) for c in layout.objects if selected is None or c.unit in selected]
