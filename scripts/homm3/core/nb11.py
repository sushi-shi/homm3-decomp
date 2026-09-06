"""Read the Dreamcast PE's embedded NB11 source-navigation records.

No cvdump installation or separate symbol file is needed. Layouts follow
Microsoft's CodeView definitions (PROCSYM32, BLOCKSYM32, REGREL32, DATASYM32):
https://github.com/microsoft/microsoft-pdb/blob/master/include/cvinfo.h
The older OMF directory and source-module tables retain section-relative offsets.
This reader supplies navigation facts; the existing corpus retains decoded types.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from bisect import bisect_left
from functools import cached_property, lru_cache
import struct

from homm3.core.inputs import InputError


class NB11Error(InputError):
    pass


class _View:
    def __init__(self, data: bytes):
        self.data = data

    def part(self, offset: int, size: int) -> bytes:
        if offset < 0 or size < 0 or offset + size > len(self.data):
            raise NB11Error(f"truncated PE/NB11 record at {offset:#x}, size {size:#x}")
        return self.data[offset:offset + size]

    def unpack(self, fmt: str, offset: int = 0) -> tuple:
        return struct.unpack(fmt, self.part(offset, struct.calcsize(fmt)))

    def string(self, offset: int) -> str:
        size, = self.unpack("<B", offset)
        return self.part(offset + 1, size).decode("latin1")


@dataclass
class Variable:
    name: str
    type_index: int
    kind: str
    storage: str | None
    scope: int | None
    record_offset: int


@dataclass
class Scope:
    record_offset: int
    parent: int
    end_record: int
    address: int
    size: int
    name: str


@dataclass
class Procedure:
    name: str
    size: int
    locals: list[tuple[str, int, str]] = field(default_factory=list)
    scopes: list[tuple[int, int]] = field(default_factory=list)
    module: str = ""
    type_index: int = 0
    record_offset: int = 0
    debug_start: int = 0
    debug_end: int = 0
    flags: int = 0
    variables: list[Variable] = field(default_factory=list)
    lexical_scopes: list[Scope] = field(default_factory=list)
    endarg: bool = False


@dataclass
class Symbols:
    procedures: dict[int, Procedure] = field(default_factory=dict)
    names: dict[int, str] = field(default_factory=dict)
    source_lines: dict[str, list[tuple[str, int, int]]] = field(default_factory=dict)
    type_records: dict[int, bytes] = field(default_factory=dict)
    module_info: dict[str, list[dict]] = field(default_factory=dict)

    @cached_property
    def _lines(self):
        rows = sorted((addr, line, source)
                      for lines in self.source_lines.values()
                      for source, line, addr in lines)
        return [row[0] for row in rows], rows

    def line_table(self, offset: int, size: int) -> list[tuple[int, int, str]]:
        addresses, rows = self._lines
        return rows[bisect_left(addresses, offset):bisect_left(addresses, offset + size)]


def _stream(data: bytes) -> tuple[_View, dict[int, int]]:
    image = _View(data)
    if image.part(0, 2) != b"MZ":
        raise NB11Error("Dreamcast executable is not a PE image")
    pe, = image.unpack("<I", 0x3c)
    if image.part(pe, 4) != b"PE\0\0":
        raise NB11Error("missing PE signature")
    machine, count = image.unpack("<HH", pe + 4)
    optional_size, = image.unpack("<H", pe + 20)
    optional = pe + 24
    magic, = image.unpack("<H", optional)
    if machine != 0x1a6 or magic != 0x10b or optional_size < 152:
        raise NB11Error("expected a Dreamcast SH4 PE32 image with a debug directory")
    image_base, = image.unpack("<I", optional + 28)
    sections = []
    bases = {}
    for index in range(count):
        entry = optional + optional_size + index * 40
        _virtual_size, rva, size, raw = image.unpack("<4I", entry + 8)
        sections.append((rva, size, raw))
        bases[index + 1] = image_base + rva

    debug_rva, debug_size = image.unpack("<II", optional + 96 + 6 * 8)
    if not debug_size or debug_size % 28:
        raise NB11Error("missing or malformed PE debug directory")
    debug_raw = next((raw + debug_rva - rva for rva, size, raw in sections
                      if rva <= debug_rva and debug_rva + debug_size <= rva + size), None)
    if debug_raw is None:
        raise NB11Error("PE debug directory is outside file-backed sections")
    directory = _View(image.part(debug_raw, debug_size))
    for entry in range(0, debug_size, 28):
        kind, size, _rva, raw = directory.unpack("<4I", entry + 12)
        if kind == 2:
            stream = _View(image.part(raw, size))
            if stream.part(0, 4) != b"NB11":
                raise NB11Error("expected embedded NB11 CodeView symbols")
            return stream, bases
    raise NB11Error("Dreamcast executable has no CodeView debug entry")


def _entries(stream: _View) -> list[tuple[int, int, _View]]:
    offset, = stream.unpack("<I", 4)
    header_size, entry_size, count, next_directory, _flags = stream.unpack("<HHIII", offset)
    if header_size != 16 or entry_size != 12 or next_directory:
        raise NB11Error("unsupported NB11 directory layout")
    directory = _View(stream.part(offset + header_size, entry_size * count))
    out = []
    for index in range(count):
        kind, module, raw, size = directory.unpack("<HHII", index * entry_size)
        out.append((kind, module, _View(stream.part(raw, size))))
    return out


def _records(view: _View, start: int):
    offset = start
    while offset < len(view.data):
        size, kind = view.unpack("<HH", offset)
        if size < 2:
            raise NB11Error(f"invalid NB11 symbol record length at {offset:#x}")
        record = _View(view.part(offset, size + 2))
        yield offset, kind, record
        offset += size + 2


def _symbols(view: _View, start: int, result: Symbols, bases: dict[int, int],
             module: str = "") -> None:
    current = None
    procedure_end = None
    scope_stack: list[Scope] = []
    after_arguments = False
    for offset, kind, record in _records(view, start):
        if kind in (0x100a, 0x100b):  # S_LPROC32_ST / S_GPROC32_ST
            end, = record.unpack("<I", 8)
            size, = record.unpack("<I", 16)
            address, segment = record.unpack("<IH", 32)
            name = record.string(39).strip()
            current = Procedure(name, size)
            current.module = module
            current.record_offset = offset
            current.debug_start, current.debug_end, current.type_index = record.unpack("<III", 20)
            current.flags, = record.unpack("<B", 38)
            procedure_end = end
            scope_stack = []
            after_arguments = False
            if segment == 1:
                result.procedures.setdefault(address, current)
                result.names.setdefault(bases[segment] + address, name)
        elif kind == 0x0006 and offset == procedure_end:  # S_END of the procedure
            current = None
            procedure_end = None
            scope_stack = []
        elif kind == 0x0006 and scope_stack:
            scope_stack.pop()
        elif kind == 0x000a and current is not None:  # S_ENDARG
            after_arguments = True
            current.endarg = True
        elif kind == 0x0207 and current is not None:  # S_BLOCK32_ST
            size, address, segment = record.unpack("<IIH", 12)
            if segment == 1:
                current.scopes.append((address, size))
                parent, end = record.unpack("<II", 4)
                scope = Scope(offset, parent, end, address, size, record.string(22))
                current.lexical_scopes.append(scope)
                scope_stack.append(scope)
        elif kind == 0x100d and current is not None:  # S_REGREL32_ST
            address, typ, register = record.unpack("<IIH", 4)
            # CV_SH3_IntR0..IntR15 are 10..25, also used for SH4 locals.
            reg = f"r{register - 10}" if 10 <= register <= 25 else f"reg{register}"
            reg = {24: "fp", 25: "sp"}.get(register, reg)
            current.locals.append((reg, address, record.string(14)))
            signed = address if address < 0x80000000 else address - 0x100000000
            current.variables.append(Variable(
                record.string(14), typ, "local" if after_arguments else "param",
                f"{reg}{signed:+#x}",
                scope_stack[-1].record_offset if scope_stack else None, offset))
        elif kind in (0x1002, 0x1003):  # S_CONSTANT_ST / S_UDT_ST
            typ, = record.unpack("<I", 4)
            item = {"kind": "constant" if kind == 0x1002 else "typedef",
                    "type_index": typ, "record_offset": offset}
            if kind == 0x1002:
                value, end = numeric(record, 8)
                item.update(value=value, name=record.string(end))
            else:
                item["name"] = record.string(8)
            item["procedure"] = current.record_offset if current else None
            item["scope"] = scope_stack[-1].record_offset if scope_stack else None
            result.module_info.setdefault(module, []).append(item)
        elif kind in (0x0001, 0x1013):  # S_COMPILE / S_COMPILE2_ST
            item = {"kind": "compiler", "record_kind": kind}
            if kind == 1:
                machine, = record.unpack("<B", 4)
                item.update(machine=machine, flags=int.from_bytes(record.part(5, 3), "little"),
                            version=record.string(8))
            else:
                flags, machine, *versions = record.unpack("<IH6H", 4)
                item.update(machine=machine, flags=flags, version=record.string(22),
                            frontend=versions[:3], backend=versions[3:])
            result.module_info.setdefault(module, []).append(item)
        elif kind in (0x1007, 0x1008, 0x1009):  # data / public, length-prefixed names
            address, segment = record.unpack("<IH", 8)
            if segment in bases:
                result.names.setdefault(bases[segment] + address, record.string(14))
            if kind != 0x1009:
                typ, = record.unpack("<I", 4)
                result.module_info.setdefault(module, []).append({
                    "kind": "global" if kind == 0x1008 else "static",
                    "name": record.string(14), "type_index": typ,
                    "segment": segment, "offset": address,
                    "procedure": current.record_offset if current else None,
                    "scope": scope_stack[-1].record_offset if scope_stack else None,
                })


def numeric(view: _View, offset: int) -> tuple[int | float, int]:
    """CodeView numeric leaf plus the offset of the next field."""
    leaf, = view.unpack("<H", offset)
    if leaf < 0x8000:
        return leaf, offset + 2
    formats = {0x8000: "b", 0x8001: "h", 0x8002: "H", 0x8003: "i",
               0x8004: "I", 0x8005: "f", 0x8006: "d", 0x8009: "q", 0x800a: "Q"}
    fmt = formats.get(leaf)
    if fmt is None:
        raise NB11Error(f"unsupported numeric leaf {leaf:#x}")
    value, = view.unpack("<" + fmt, offset + 2)
    return value, offset + 2 + struct.calcsize(fmt)


def _source_lines(view: _View) -> list[tuple[str, int, int]]:
    files, segments = view.unpack("<HH")
    # Validate the whole module header, including its segment ranges and indices.
    view.part(4, files * 4 + segments * 10)
    result = []
    for index in range(files):
        file_offset, = view.unpack("<I", 4 + index * 4)
        count, _reserved = view.unpack("<HH", file_offset)
        source = view.string(file_offset + 4 + count * 12)
        for segment_index in range(count):
            line_offset, = view.unpack("<I", file_offset + 4 + segment_index * 4)
            segment, pairs = view.unpack("<HH", line_offset)
            offsets = view.unpack(f"<{pairs}I", line_offset + 4)
            lines = view.unpack(f"<{pairs}H", line_offset + 4 + pairs * 4)
            if segment == 1:
                result.extend((source, line, address) for address, line in zip(offsets, lines))
    return result


@lru_cache(maxsize=1)
def parse(data: bytes) -> Symbols:
    """Parse verified image bytes; cache only by the complete byte content."""
    stream, bases = _stream(data)
    entries = _entries(stream)
    modules = {}
    for kind, module, view in entries:
        if kind == 0x120:  # sstModule
            segments, = view.unpack("<H", 4)
            name = view.string(8 + segments * 12)
            modules[module] = name.replace("/", "\\").rsplit("\\", 1)[-1]
    result = Symbols()
    # Match cvdump's name precedence: public linkage names before source names.
    order = {0x12a: 0, 0x125: 1, 0x129: 2, 0x134: 3}
    for kind, module, view in sorted(entries, key=lambda entry: order.get(entry[0], 4)):
        if kind == 0x125:  # sstAlignSym, followed by C11 symbol records
            signature, = view.unpack("<I")
            if signature != 2:
                raise NB11Error("expected C11 aligned symbols in the NB11 stream")
            _symbols(view, 4, result, bases, modules.get(module, ""))
        elif kind in (0x129, 0x12a, 0x134):  # global/static symbols and publics
            _symbol_hash, _address_hash, size = view.unpack("<HHI")
            _symbols(_View(view.part(16, size)), 0, result, bases)
        elif kind == 0x127:  # sstSrcModule
            if module not in modules:
                raise NB11Error(f"source lines refer to missing module {module}")
            result.source_lines.setdefault(modules[module], []).extend(_source_lines(view))
        elif kind == 0x12b:  # sstGlobalTypes (C11, 32-bit type indices)
            flags, count = view.unpack("<II")
            if flags != 2:
                raise NB11Error(f"unsupported global type flags {flags:#x}")
            base = 8 + count * 4
            for index, offset in enumerate(view.unpack(f"<{count}I", 8)):
                length, = view.unpack("<H", base + offset)
                if length < 2:
                    raise NB11Error("invalid global type record length")
                result.type_records[0x1000 + index] = view.part(base + offset, length + 2)
    if not result.procedures or not result.source_lines:
        raise NB11Error("NB11 stream has no procedures or source lines")
    return result
