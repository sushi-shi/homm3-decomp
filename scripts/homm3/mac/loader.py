"""Symbolic PEF loader relocations, without assigning invented runtime VAs.

Format: Apple's PEFBinaryFormat.h and Mac OS Runtime Architectures, chapter 8:
https://dev.os9.ca/techpubs/mac/runtimehtml/RTArch-98.html
Each relocated pointer resolves to a section offset or a named library import.
"""
from __future__ import annotations

from dataclasses import dataclass
import struct

from homm3.mac.pef import PEF, PEFError
from homm3.mac.relocations import Address


@dataclass(frozen=True)
class Import:
    index: int
    library: str
    name: str
    symbol_class: int
    weak: bool


@dataclass(frozen=True)
class ImportedAddress:
    symbol: Import
    addend: int


class Loader:
    def __init__(self, pef: PEF):
        self.pef = pef
        sections = [s for s in pef.sections if s.kind == 4]
        if len(sections) != 1:
            raise PEFError("expected one PEF loader section")
        self.data = pef.contents(sections[0].index)
        (main_section, main_offset, init_section, init_offset, term_section, term_offset,
         libraries, imports, relocations, reloc_offset, strings, exports, _, _) = self._unpack(
             ">iIiIiI8I", 0)
        table_end = 56 + libraries * 24 + imports * 4 + relocations * 12
        if not table_end <= reloc_offset <= strings <= exports <= len(self.data):
            raise PEFError("invalid PEF loader table bounds")
        self.entry = self._entry(main_section, main_offset)
        self.init = self._entry(init_section, init_offset)
        self.term = self._entry(term_section, term_offset)

        def name(offset):
            start = strings + offset
            if not strings <= start < exports:
                raise PEFError("PEF loader string is outside string table")
            end = self.data.find(b"\0", start, exports)
            if end < 0:
                raise PEFError("unterminated PEF loader string")
            return self.data[start:end].decode("mac_roman")

        owners = [None] * imports
        for index in range(libraries):
            library_name, _, _, count, first, _, reserved, reserved2 = self._unpack(
                ">5IBBH", 56 + 24 * index)
            if reserved or reserved2 or first + count > imports:
                raise PEFError("invalid PEF imported library")
            library = name(library_name)
            for symbol_index in range(first, first + count):
                if owners[symbol_index] is not None:
                    raise PEFError("overlapping PEF imported library ranges")
                owners[symbol_index] = library
        self.imports = []
        for index, owner in enumerate(owners):
            if owner is None:
                raise PEFError("PEF import has no owning library")
            word, = self._unpack(">I", 56 + libraries * 24 + index * 4)
            self.imports.append(Import(index, owner, name(word & 0xffffff),
                                       (word >> 24) & 0x7f, bool(word & 0x80000000)))

        self.pointers: dict[Address, Address | ImportedAddress] = {}
        seen = set()
        for index in range(relocations):
            section, reserved, count, offset = self._unpack(
                ">HHII", 56 + libraries * 24 + imports * 4 + index * 12)
            self._section(section)
            if reserved or section in seen or offset % 2 or reloc_offset + offset + 2 * count > strings:
                raise PEFError("invalid PEF relocation header")
            seen.add(section)
            chunks = self._unpack(f">{count}H", reloc_offset + offset)
            self._relocate(section, chunks)

    def _unpack(self, fmt: str, offset: int) -> tuple:
        if offset < 0 or offset + struct.calcsize(fmt) > len(self.data):
            raise PEFError("truncated PEF loader")
        return struct.unpack_from(fmt, self.data, offset)

    def _section(self, index: int):
        if not 0 <= index < self.pef.instantiated:
            raise PEFError(f"relocation refers to noninstantiated section {index}")
        return self.pef.section(index)

    def _entry(self, section: int, offset: int) -> Address | None:
        if section == -1:
            return None
        self._section(section)
        self.pef.read(section, offset, 8)
        return Address(section, offset)

    def toc(self, entry: Address | None = None) -> Address:
        """Read the TOC pointer from a relocated entry transition vector."""
        entry = entry or self.entry or self.init
        if entry is None:
            raise PEFError("PEF has no entry transition vector for TOC discovery")
        code = self.pointers.get(entry)
        toc = self.pointers.get(Address(entry.section, entry.offset + 4))
        if (not isinstance(code, Address) or self.pef.section(code.section).kind != 0
                or not isinstance(toc, Address) or self.pef.section(toc.section).kind not in (1, 2)):
            raise PEFError("entry transition vector lacks relocated code/TOC pointers")
        return toc

    def _relocate(self, section: int, chunks: tuple[int, ...]) -> None:
        pos, import_index = 0, 0
        sect_c = 0 if self.pef.instantiated else None
        sect_d = 1 if self.pef.instantiated > 1 else None
        section_size = self.pef.section(section).total_size

        def move(value):
            nonlocal pos
            if not 0 <= value <= section_size:
                raise PEFError(f"PEF relocation position out of section {section}: {value:#x}")
            pos = value

        def fix(target_section=None, symbol_index=None):
            nonlocal pos
            raw = int.from_bytes(self.pef.read(section, pos, 4), "big")
            location = Address(section, pos)
            if location in self.pointers:
                raise PEFError(f"multiple PEF relocations at {location}")
            if symbol_index is not None:
                if not 0 <= symbol_index < len(self.imports):
                    raise PEFError(f"PEF import index out of range: {symbol_index}")
                target = ImportedAddress(self.imports[symbol_index], raw)
            elif target_section is not None:
                info = self._section(target_section)
                offset = (raw - info.default_address) & 0xffffffff
                if offset > info.total_size:
                    raise PEFError(f"PEF pointer outside target section {target_section}: {offset:#x}")
                target = Address(target_section, offset)
            else:
                # An uninitialized sectionC/sectionD contributes zero.
                move(pos + 4)
                return
            self.pointers[location] = target
            move(pos + 4)

        # Decode instruction boundaries before replaying any repeat blocks.
        instructions = {}
        pc = 0
        while pc < len(chunks):
            word = chunks[pc]
            op = word >> 9
            wide = op in (0x50, 0x51, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b)
            width = 2 if wide else 1
            if pc + width > len(chunks):
                raise PEFError("truncated two-chunk PEF relocation")
            instructions[pc] = (word, chunks[pc + 1] if wide else 0, width)
            pc += width

        operations = 0

        def execute(start, end, replay=False):
            nonlocal sect_c, sect_d, import_index, operations
            pc = start
            while pc < end:
                operations += 1
                if operations > 10_000_000:
                    raise PEFError("excessive PEF relocation operations")
                if pc not in instructions:
                    raise PEFError("PEF repeat splits a relocation instruction")
                word, second, width = instructions[pc]
                if pc + width > end:
                    raise PEFError("PEF repeat ends inside a relocation instruction")
                op = word >> 9
                low = word & 0x1ff
                if op < 0x20:
                    move(pos + ((word >> 6) & 0xff) * 4)
                    for _ in range(word & 63):
                        fix(sect_d)
                elif 0x20 <= op <= 0x25:
                    for _ in range(low + 1):
                        if op in (0x20, 0x22, 0x23):
                            fix(sect_c)
                        if op in (0x21, 0x22, 0x23, 0x24):
                            fix(sect_d)
                        if op in (0x22, 0x24):
                            move(pos + 4)
                        if op == 0x25:
                            fix(symbol_index=import_index)
                            import_index += 1
                elif op in (0x30, 0x52, 0x53):
                    index = low if op == 0x30 else ((word & 0x3ff) << 16) | second
                    fix(symbol_index=index)
                    import_index = index + 1
                elif op in (0x31, 0x32, 0x33, 0x5a, 0x5b):
                    if op >= 0x5a:
                        action = (word >> 6) & 15
                        index = ((word & 63) << 16) | second
                    else:
                        action = {0x31: 1, 0x32: 2, 0x33: 0}[op]
                        index = low
                    self._section(index)
                    if action == 0:
                        fix(index)
                    elif action == 1:
                        sect_c = index
                    elif action == 2:
                        sect_d = index
                    else:
                        raise PEFError(f"unknown PEF section relocation subopcode {action}")
                elif 0x40 <= op <= 0x47:
                    move(pos + (word & 0xfff) + 1)
                elif op in (0x50, 0x51):
                    move(((word & 0x3ff) << 16) | second)
                elif 0x48 <= op <= 0x4f or op in (0x58, 0x59):
                    if replay:
                        raise PEFError("nested PEF relocation repeat")
                    count = ((word >> (6 if width == 2 else 8)) & 15) + 1
                    repeats = (((word & 63) << 16) | second) if width == 2 else (word & 255) + 1
                    if pc - count not in instructions:
                        raise PEFError("PEF relocation repeat starts outside instruction boundary")
                    for _ in range(repeats):
                        execute(pc - count, pc, replay=True)
                else:
                    raise PEFError(f"unknown PEF relocation opcode {op:#x}")
                pc += width

        execute(0, len(chunks))
