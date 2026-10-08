"""Minimal ELF32 little-endian reader for linked i386 executables and objects."""
from __future__ import annotations

from dataclasses import dataclass
from functools import cached_property
import struct

SHT_SYMTAB, SHT_STRTAB, SHT_REL, SHT_NOBITS, SHT_DYNSYM = 2, 3, 9, 8, 11
SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR = 1, 2, 4
STT_NOTYPE, STT_OBJECT, STT_FUNC, STT_SECTION, STT_FILE = 0, 1, 2, 3, 4
STB_LOCAL, STB_GLOBAL, STB_WEAK = 0, 1, 2
R_386_32, R_386_PC32, R_386_JMP_SLOT = 1, 2, 7


class ElfError(ValueError):
    pass


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    type: int
    flags: int
    addr: int
    offset: int
    size: int
    link: int
    info: int
    align: int
    entsize: int

    def contains(self, address: int) -> bool:
        return self.addr <= address < self.addr + self.size


@dataclass(frozen=True)
class Symbol:
    index: int
    name: str
    value: int
    size: int
    bind: int
    type: int
    shndx: int


@dataclass(frozen=True)
class Rel:
    offset: int
    type: int
    symbol: int


class Elf:
    def __init__(self, data: bytes):
        if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 1:
            raise ElfError("not a little-endian ELF32 file")
        self.data = data
        (self.type, self.machine, _, self.entry, _, self.shoff, _, _, _, _,
         self.shentsize, self.shnum, self.shstrndx) = struct.unpack_from("<HHIIIIIHHHHHH", data, 16)
        raw = [struct.unpack_from("<IIIIIIIIII", data, self.shoff + i * self.shentsize)
               for i in range(self.shnum)]
        names = raw[self.shstrndx][4]
        self.sections = [Section(i, self._cstr(names + r[0]), *r[1:]) for i, r in enumerate(raw)]

    def _cstr(self, offset: int) -> str:
        end = self.data.index(b"\0", offset)
        return self.data[offset:end].decode("latin-1")

    def section(self, name: str) -> Section:
        for section in self.sections:
            if section.name == name:
                return section
        raise ElfError(f"no section {name}")

    def section_at(self, address: int) -> Section | None:
        for section in self.sections:
            if section.flags & SHF_ALLOC and section.contains(address):
                return section
        return None

    def bytes(self, section: Section) -> bytes:
        if section.type == SHT_NOBITS:
            return bytes(section.size)
        return self.data[section.offset:section.offset + section.size]

    def read(self, address: int, size: int) -> bytes:
        section = self.section_at(address)
        if section is None or section.type == SHT_NOBITS:
            raise ElfError(f"0x{address:x} is not file-backed")
        start = section.offset + address - section.addr
        return self.data[start:start + size]

    def symbols(self, section: Section) -> list[Symbol]:
        strings = self.sections[section.link].offset
        out = []
        for i in range(section.size // 16):
            name, value, size, info, _, shndx = struct.unpack_from(
                "<IIIBBH", self.data, section.offset + i * 16)
            out.append(Symbol(i, self._cstr(strings + name), value, size, info >> 4, info & 15, shndx))
        return out

    @cached_property
    def dynsym(self) -> list[Symbol]:
        return self.symbols(self.section(".dynsym"))

    def rels(self, section: Section) -> list[Rel]:
        return [Rel(o, i & 0xff, i >> 8) for o, i in
                struct.iter_unpack("<II", self.data[section.offset:section.offset + section.size])]

    @cached_property
    def plt_slots(self) -> dict[int, str]:
        """PLT entry address -> imported symbol name (lazy-binding i386 layout)."""
        plt = self.section(".plt")
        got_names = {rel.offset: self.dynsym[rel.symbol].name
                     for rel in self.rels(self.section(".rel.plt"))}
        slots = {}
        code = self.bytes(plt)
        for entry in range(16, plt.size, 16):
            if code[entry:entry + 2] == b"\xff\x25":
                (got,) = struct.unpack_from("<I", code, entry + 2)
                if got in got_names:
                    slots[plt.addr + entry] = got_names[got]
        return slots

    def comments(self) -> list[str]:
        raw = self.bytes(self.section(".comment"))
        return [part.decode("latin-1") for part in raw.split(b"\0") if part]
