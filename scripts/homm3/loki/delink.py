"""Delink one Loki object into a comparison object, and canonicalize a compiled base.

The linked ELF has no relocations; references are recovered by decoding each
function (homm3.loki.cmpobj) and naming every address through the census,
the PLT and the exported symbols. A compiled GCC 2.95 object goes through the
same canonical form (see cmpobj) so objdiff compares like with like.
"""
from __future__ import annotations

from dataclasses import dataclass
import struct

from homm3.core import common
from homm3.loki import cmpobj
from homm3.loki.cmpobj import CodeSection, Function, put
from homm3.loki.elf import (Elf, R_386_32, R_386_PC32, SHF_ALLOC, SHF_EXECINSTR, SHT_REL,
                            SHT_SYMTAB, STT_FUNC, STT_OBJECT, STT_SECTION)
from homm3.loki.image import IMAGE, LokiImage

RETAIL = common.HOMM3_DIR / "config/retail" / IMAGE


@dataclass(frozen=True)
class CensusFunction:
    address: int
    size: int
    obj: int
    linkonce: bool
    bind: str
    name: str


def census() -> list[CensusFunction]:
    rows = []
    for line in (RETAIL / "functions.tsv").read_text().splitlines():
        if line.startswith("#"):
            continue
        address, size, obj, placement, bind, name, _ = line.split("\t")
        rows.append(CensusFunction(int(address, 16), int(size), int(obj), placement == "linkonce", bind, name))
    return rows


def objects() -> dict[int, tuple[int, int, str]]:
    out = {}
    for line in (RETAIL / "objects.tsv").read_text().splitlines():
        if line.startswith("#"):
            continue
        obj, _, start, end, *_rest = line.split("\t")
        out[int(obj)] = (int(start, 16), int(end, 16), _rest[3])
    return out


def _symbol_name(function: CensusFunction) -> str:
    return function.name or f"sub_{function.address:08x}"


class Target:
    """Retail side: name every address of the Loki image."""

    def __init__(self, image: LokiImage, functions: list[CensusFunction]):
        self.image = image
        self.functions = {f.address: f for f in functions}
        self.plt = image.elf.plt_slots
        self.text = image.elf.section(".text")
        self.rodata = image.elf.section(".rodata")

    def function_name(self, address: int) -> tuple[str, int] | None:
        owner = self.image.function_at(address)
        if owner is not None and owner.address in self.functions:
            return _symbol_name(self.functions[owner.address]), address - owner.address
        return None

    def name(self, value: int, field: cmpobj.Field, function: CensusFunction, tables: list[int]) -> tuple[str, int] | None:
        if value in self.plt:
            return self.plt[value], 0
        exported = self.image.object_at(value)
        if exported is not None:
            return exported.name, value - exported.value
        if self.text.contains(value):
            return self.function_name(value)
        if self.rodata.contains(value):
            (entry,) = struct.unpack("<I", self.image.elf.read(value, 4))
            if function.address <= entry < function.address + function.size and field.kind == "disp":
                if value not in tables:
                    tables.append(value)
                return f"{_symbol_name(function)}$jt{tables.index(value)}", 0
            return cmpobj.literal_for(self.image.elf.read, value, field.access_size), 0
        section = self.image.elf.section_at(value)
        if section is not None:
            return f"data_{value:08x}", 0
        return None

    def section(self, name: str, start: int, end: int, members: list[CensusFunction]) -> CodeSection:
        data = bytearray(self.image.elf.read(start, end - start))
        section = CodeSection(name, data)
        for function in members:
            section.functions.append(Function(_symbol_name(function), function.address - start,
                                              function.size, function.bind != "static"))
            tables: list[int] = []
            local_start = function.address - start
            for field in cmpobj.fields(bytes(data), local_start, local_start + function.size):
                if field.kind == "rel":
                    destination = start + field.end + field.value
                    if function.address <= destination < function.address + function.size:
                        continue
                    resolved = self.name(destination, field, function, tables)
                    if resolved is None:
                        continue
                    put(section, field.offset, R_386_PC32, resolved[0], resolved[1] - 4)
                    continue
                resolved = self.name(field.value, field, function, tables)
                if resolved is not None:
                    put(section, field.offset, R_386_32, resolved[0], resolved[1])
        return section


def target_sections(obj: int, image: LokiImage | None = None) -> list[CodeSection]:
    image = image or LokiImage()
    functions = census()
    start, end, _ = objects()[obj]
    target = Target(image, functions)
    own = [f for f in functions if f.obj == obj and not f.linkonce]
    sections = [target.section(".text", start, end, own)]
    for function in functions:
        if function.obj == obj and function.linkonce:
            sections.append(target.section(f".gnu.linkonce.t.{_symbol_name(function)}",
                                           function.address, function.address + function.size, [function]))
    return sections


def base_sections(data: bytes) -> list[CodeSection]:
    """Compiled side: canonicalize a GCC 2.95 relocatable object."""
    elf = Elf(data)
    symtab = next(s for s in elf.sections if s.type == SHT_SYMTAB)
    symbols = elf.symbols(symtab)
    rels = {s.info: elf.rels(s) for s in elf.sections if s.type == SHT_REL}

    def read_section(index: int):
        raw = elf.bytes(elf.sections[index])
        return lambda address, size: raw[address:address + size]

    by_section: dict[int, list] = {}
    for symbol in symbols:
        if symbol.shndx and symbol.shndx < 0xff00 and symbol.type in (STT_FUNC, STT_OBJECT):
            by_section.setdefault(symbol.shndx, []).append(symbol)

    def containing(index: int, offset: int):
        for symbol in by_section.get(index, ()):
            if symbol.value <= offset < symbol.value + max(symbol.size, 1):
                return symbol
        return None

    def is_jump_table(index: int, offset: int) -> bool:
        return any(r.offset == offset and symbols[r.symbol].type == STT_SECTION
                   and elf.sections[symbols[r.symbol].shndx].flags & SHF_EXECINSTR
                   for r in rels.get(index, ()))

    out = []
    for index, header in enumerate(elf.sections):
        if not (header.flags & SHF_EXECINSTR and header.flags & SHF_ALLOC) or header.size == 0:
            continue
        section = CodeSection(header.name, bytearray(elf.bytes(header)))
        functions = sorted((s for s in by_section.get(index, ()) if s.type == STT_FUNC), key=lambda s: s.value)
        for symbol in functions:
            section.functions.append(Function(symbol.name, symbol.value, symbol.size, symbol.bind != 0))
        relocated = {r.offset: r for r in rels.get(index, ())}
        for symbol in functions:
            tables: list[int] = []
            for field in cmpobj.fields(bytes(section.data), symbol.value, symbol.value + symbol.size):
                rel = relocated.get(field.offset)
                if rel is None:
                    if field.kind == "rel":
                        destination = field.end + field.value
                        if symbol.value <= destination < symbol.value + symbol.size:
                            continue
                        callee = containing(index, destination)
                        if callee is not None:
                            put(section, field.offset, R_386_PC32, callee.name, destination - callee.value - 4)
                    continue
                target = symbols[rel.symbol]
                (addend,) = struct.unpack_from("<i", section.data, field.offset)
                if target.type != STT_SECTION:
                    put(section, field.offset, rel.type, target.name, addend)
                    continue
                place = addend + 4 if rel.type == R_386_PC32 else addend
                kind = elf.sections[target.shndx]
                if kind.flags & SHF_EXECINSTR:
                    owner = containing(target.shndx, place)
                    name, delta = (owner.name, place - owner.value) if owner else (f"{kind.name}+{place:x}", 0)
                elif is_jump_table(target.shndx, place):
                    if place not in tables:
                        tables.append(place)
                    name, delta = f"{symbol.name}$jt{tables.index(place)}", 0
                elif (owner := containing(target.shndx, place)) is not None:
                    name, delta = owner.name, place - owner.value
                elif kind.name.startswith(".rodata"):
                    name, delta = cmpobj.literal_for(read_section(target.shndx), place, field.access_size), 0
                else:
                    name, delta = f"{kind.name}+{place:x}", 0
                put(section, field.offset, rel.type, name, delta - 4 if rel.type == R_386_PC32 else delta)
        out.append(section)
    return out


def pair_statics(base: list[CodeSection], target: list[CodeSection]) -> list[str]:
    """Name the retail object's file-static functions after the compiled ones.

    The image keeps no names for file-static functions (`sub_<address>`).
    g++ 2.95 at -O0 emits an object's .text in source order, so the named
    functions both sides share are anchors, and between two consecutive
    anchors the k-th retail static is the k-th compiled local function when
    both gaps hold the same number. A gap with different counts stays
    unpaired (and is returned for the report): the bytes still decide every
    pairing, since objdiff compares the paired bodies."""
    base_text = next((s for s in base if s.name == ".text"), None)
    target_text = next((s for s in target if s.name == ".text"), None)
    if base_text is None or target_text is None:
        return []
    shared = {f.name for f in base_text.functions} & {f.name for f in target_text.functions}

    def gaps(functions: list[Function], unnamed) -> dict[str | None, list[Function]]:
        out: dict[str | None, list[Function]] = {}
        anchor = None
        for function in sorted(functions, key=lambda f: f.offset):
            if function.name in shared:
                anchor = function.name
            elif unnamed(function):
                out.setdefault(anchor, []).append(function)
        return out

    retail = gaps(target_text.functions, lambda f: f.name.startswith("sub_"))
    compiled = gaps(base_text.functions, lambda f: not f.bind_global)
    unpaired = []
    for anchor, statics in retail.items():
        locals_ = compiled.get(anchor, [])
        if len(locals_) != len(statics):
            unpaired += [f.name for f in statics]
            continue
        for static, local in zip(statics, locals_):
            renamed = {static.name: local.name}
            static.name = local.name
            for section in target:
                for reloc in section.relocs:
                    reloc.target = renamed.get(reloc.target, reloc.target)
    return unpaired
