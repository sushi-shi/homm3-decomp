#!/usr/bin/env python3
"""Read VC6 classic COFF source-line records from a ``/Z7`` object.

VC6 does not emit the modern C13 ``DEBUG_S_LINES`` subsection understood by
current LLVM tools.  It stores source locations in each code section's
``IMAGE_LINENUMBER`` table instead.  A zero-line record anchors the table to a
function symbol; later records contain section offsets and line numbers
relative to the function's ``.bf`` (begin-function) line.

The matching objects must remain free of debug symbols, so sema compiles a
parallel ``/Z7`` object and uses this module only for its line map.  Function
bytes are exposed as well, allowing the caller to prove that the debug and
matching compilations have identical code before applying the offsets.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct


class CodeViewError(ValueError):
    """The COFF object contains malformed or unsupported line information."""


@dataclass(frozen=True)
class LineRecord:
    offset: int
    line: int


@dataclass(frozen=True)
class FunctionLines:
    name: str
    ordinal: int
    begin_line: int
    lines: tuple[LineRecord, ...]
    code: bytes


@dataclass(frozen=True)
class _Section:
    index: int
    name: str
    raw_offset: int
    raw_size: int
    line_offset: int
    line_count: int
    characteristics: int


@dataclass(frozen=True)
class _Symbol:
    index: int
    name: str
    value: int
    section: int
    typ: int
    storage: int
    aux_count: int


@dataclass(frozen=True)
class _FunctionDef:
    symbol: _Symbol
    ordinal: int
    size: int
    code: bytes


_COFF_HEADER = 20
_SECTION_HEADER = 40
_SYMBOL_SIZE = 18
_LINE_SIZE = 6
_CNT_CODE = 0x20
_TYPE_FUNCTION = 0x20


def _need(data: bytes, offset: int, size: int, what: str) -> None:
    if offset < 0 or size < 0 or offset + size > len(data):
        raise CodeViewError(
            f"truncated COFF {what} at 0x{offset:x} (need {size} bytes, "
            f"object has 0x{len(data):x})")


def _short_name(raw: bytes) -> str:
    return raw.rstrip(b"\0").decode("latin1", "replace")


class _Coff:
    def __init__(self, data: bytes):
        _need(data, 0, _COFF_HEADER, "header")
        self.data = data
        self.section_count = struct.unpack_from("<H", data, 2)[0]
        self.sym_offset, self.symbol_count = struct.unpack_from("<II", data, 8)
        optional_size = struct.unpack_from("<H", data, 16)[0]
        section_offset = _COFF_HEADER + optional_size
        _need(data, section_offset,
              self.section_count * _SECTION_HEADER, "section table")

        self.sections: list[_Section] = []
        for index in range(self.section_count):
            off = section_offset + index * _SECTION_HEADER
            raw_size, raw_offset = struct.unpack_from("<II", data, off + 16)
            line_offset = struct.unpack_from("<I", data, off + 28)[0]
            line_count = struct.unpack_from("<H", data, off + 34)[0]
            characteristics = struct.unpack_from("<I", data, off + 36)[0]
            if raw_size:
                _need(data, raw_offset, raw_size,
                      f"section {index + 1} data")
            if line_count:
                _need(data, line_offset, line_count * _LINE_SIZE,
                      f"section {index + 1} line table")
            self.sections.append(_Section(
                index=index + 1,
                name=_short_name(data[off:off + 8]),
                raw_offset=raw_offset,
                raw_size=raw_size,
                line_offset=line_offset,
                line_count=line_count,
                characteristics=characteristics,
            ))

        if self.symbol_count:
            _need(data, self.sym_offset,
                  self.symbol_count * _SYMBOL_SIZE, "symbol table")
        self.string_offset = self.sym_offset + self.symbol_count * _SYMBOL_SIZE
        if self.string_offset + 4 <= len(data):
            string_size = struct.unpack_from("<I", data, self.string_offset)[0]
            if string_size < 4:
                raise CodeViewError("invalid COFF string-table size")
            _need(data, self.string_offset, string_size, "string table")
            self.string_size = string_size
        else:
            self.string_size = 0

        self.symbols: dict[int, _Symbol] = {}
        index = 0
        while index < self.symbol_count:
            off = self.sym_offset + index * _SYMBOL_SIZE
            aux_count = data[off + 17]
            if index + aux_count >= self.symbol_count:
                raise CodeViewError(
                    f"symbol {index} has {aux_count} auxiliary records past EOF")
            self.symbols[index] = _Symbol(
                index=index,
                name=self._symbol_name(off),
                value=struct.unpack_from("<I", data, off + 8)[0],
                section=struct.unpack_from("<h", data, off + 12)[0],
                typ=struct.unpack_from("<H", data, off + 14)[0],
                storage=data[off + 16],
                aux_count=aux_count,
            )
            index += 1 + aux_count

        self.functions = self._function_defs()

    def _symbol_name(self, offset: int) -> str:
        raw = self.data[offset:offset + 8]
        if raw[:4] != b"\0\0\0\0":
            return _short_name(raw)
        string_index = struct.unpack_from("<I", raw, 4)[0]
        if not self.string_size or not 4 <= string_index < self.string_size:
            raise CodeViewError(
                f"invalid COFF string-table offset 0x{string_index:x}")
        start = self.string_offset + string_index
        limit = self.string_offset + self.string_size
        end = self.data.find(b"\0", start, limit)
        if end < 0:
            raise CodeViewError("unterminated COFF symbol name")
        return self.data[start:end].decode("latin1", "replace")

    def aux(self, symbol_index: int, ordinal: int = 0) -> bytes:
        symbol = self.symbols.get(symbol_index)
        if symbol is None or ordinal >= symbol.aux_count:
            raise CodeViewError(
                f"symbol {symbol_index} has no auxiliary record {ordinal}")
        off = self.sym_offset + (symbol_index + 1 + ordinal) * _SYMBOL_SIZE
        return self.data[off:off + _SYMBOL_SIZE]

    def _function_defs(self) -> dict[int, _FunctionDef]:
        candidates = [
            symbol for symbol in self.symbols.values()
            if symbol.section > 0 and symbol.typ & _TYPE_FUNCTION
            and self.sections[symbol.section - 1].characteristics & _CNT_CODE
        ]
        candidates.sort(key=lambda s: (s.section, s.value, s.index))
        ordinals: dict[str, int] = {}
        out: dict[int, _FunctionDef] = {}
        for symbol in candidates:
            ordinal = ordinals.get(symbol.name, 0)
            ordinals[symbol.name] = ordinal + 1
            section = self.sections[symbol.section - 1]
            size = (struct.unpack_from("<I", self.aux(symbol.index), 4)[0]
                    if symbol.aux_count else 0)
            if not size:
                later = [candidate.value for candidate in candidates
                         if candidate.section == symbol.section
                         and candidate.value > symbol.value]
                size = ((min(later) if later else section.raw_size)
                        - symbol.value)
            if symbol.value + size > section.raw_size:
                raise CodeViewError(
                    f"function {symbol.name} extends past section "
                    f"{symbol.section}")
            start = section.raw_offset + symbol.value
            out[symbol.index] = _FunctionDef(
                symbol=symbol,
                ordinal=ordinal,
                size=size,
                code=self.data[start:start + size],
            )
        return out


def _read(path: str | Path) -> bytes:
    try:
        return Path(path).read_bytes()
    except OSError as exc:
        raise CodeViewError(f"cannot read {path}: {exc}") from exc


def function_bytes(path: str | Path, name: str, ordinal: int = 0) -> bytes:
    """Return the logical code bytes for one function in a COFF object."""
    coff = _Coff(_read(path))
    matches = [function for function in coff.functions.values()
               if function.symbol.name == name
               and function.ordinal == ordinal]
    if not matches:
        raise CodeViewError(
            f"function {name} occurrence {ordinal} not found in {path}")
    return matches[0].code


def parse_lines(path: str | Path) -> dict[tuple[str, int], FunctionLines]:
    """Return ``{(mangled_name, ordinal): FunctionLines}`` for a ``/Z7`` obj.

    Repeated line entries at the same code offset are deliberately preserved:
    optimized statements can share one instruction boundary, and discarding
    one would hide useful candidate-source evidence.
    """
    coff = _Coff(_read(path))
    accumulated: dict[int, tuple[int, list[LineRecord]]] = {}

    for section in coff.sections:
        if not section.line_count:
            continue
        # VC6 also puts legacy line-number payloads on a few compiler support
        # contributions without the function-symbol anchor described by
        # IMAGE_LINENUMBER.  They cannot be joined safely, so ignore the whole
        # contribution just as debuggers do rather than attributing it to the
        # preceding function.
        _first_value, first_line = struct.unpack_from(
            "<IH", coff.data, section.line_offset)
        if first_line != 0:
            continue
        current: int | None = None
        begin_line = 0
        for row in range(section.line_count):
            off = section.line_offset + row * _LINE_SIZE
            value, stored = struct.unpack_from("<IH", coff.data, off)
            if stored == 0:
                function = coff.functions.get(value)
                if function is None:
                    raise CodeViewError(
                        f"line-table anchor names non-function symbol {value}")
                if function.symbol.section != section.index:
                    raise CodeViewError(
                        f"line-table anchor for {function.symbol.name} is in "
                        f"section {section.index}, symbol is in "
                        f"{function.symbol.section}")
                aux = coff.aux(function.symbol.index)
                tag_index = struct.unpack_from("<I", aux, 0)[0]
                tag = coff.symbols.get(tag_index)
                if tag is None or tag.name != ".bf" or not tag.aux_count:
                    raise CodeViewError(
                        f"function {function.symbol.name} has no resolvable .bf")
                begin_line = struct.unpack_from(
                    "<H", coff.aux(tag_index), 4)[0]
                current = function.symbol.index
                accumulated.setdefault(current, (begin_line, []))
                continue
            if current is None:
                raise CodeViewError(
                    f"section {section.index} line row {row} precedes an anchor")
            function = coff.functions[current]
            relative = value - function.symbol.value
            if relative < 0 or relative >= function.size:
                raise CodeViewError(
                    f"line offset 0x{value:x} is outside "
                    f"{function.symbol.name} (size 0x{function.size:x})")
            accumulated[current][1].append(
                LineRecord(relative, begin_line + stored))

    out: dict[tuple[str, int], FunctionLines] = {}
    for symbol_index, (begin_line, lines) in accumulated.items():
        function = coff.functions[symbol_index]
        key = (function.symbol.name, function.ordinal)
        out[key] = FunctionLines(
            name=function.symbol.name,
            ordinal=function.ordinal,
            begin_line=begin_line,
            lines=tuple(lines),
            code=function.code,
        )
    return out
