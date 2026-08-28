#!/usr/bin/env python3
"""Hermetic controls for paired VC6 EH handler-owner normalization.

Run with ``python3 -m homm3.build.test_eh_handler_normalization``.  The
positive fixture proves that ``handler`` and ``last funclet + size`` resolve
to the same byte before the disposable comparison copy is rewritten.  The
negative fixtures prove that a different retail cleanup size, malformed
handler thunk, or missing final funclet is left visible.
"""
from __future__ import annotations

import struct
import unittest
from dataclasses import dataclass

from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.normalize_objs import (
    CNT_CODE, DIR32, FUNCTION_TYPE, REL32,
    _canonicalize_matching_eh_handler_owners,
)


CODE_COMDAT = CNT_CODE | 0x20000000 | 0x40000000 | 0x00001000


@dataclass(frozen=True)
class FixtureSection:
    name: str
    raw: bytes
    relocations: tuple[tuple[int, int, int], ...]


def _symbol(name: str, value: int, section: int, typ: int,
            storage: int, aux: bytes = b"") -> bytes:
    assert len(name.encode("ascii")) <= 8
    assert not aux or len(aux) % 18 == 0
    count = len(aux) // 18
    return (name.encode("ascii").ljust(8, b"\0") +
            struct.pack("<IhHBB", value, section, typ, storage, count) + aux)


def _section_aux(length: int, relocation_count: int,
                 parent: int = 0, selection: int = 1) -> bytes:
    row = bytearray(18)
    struct.pack_into("<IHHIhB", row, 0, length, relocation_count, 0, 0,
                     parent, selection)
    return bytes(row)


def _coff(sections: tuple[FixtureSection, ...], symbols: tuple[bytes, ...]) -> bytes:
    data = bytearray(20 + 40 * len(sections))
    layouts = []
    for section in sections:
        raw_offset = len(data)
        data.extend(section.raw)
        relocation_offset = len(data) if section.relocations else 0
        for site, symbol, typ in section.relocations:
            data.extend(struct.pack("<IIH", site, symbol, typ))
        layouts.append((raw_offset, relocation_offset))
    symbol_offset = len(data)
    symbol_table = b"".join(symbols)
    assert len(symbol_table) % 18 == 0
    data.extend(symbol_table)
    data.extend(struct.pack("<I", 4))
    struct.pack_into("<HHIIIHH", data, 0, 0x14C, len(sections), 0,
                     symbol_offset, len(symbol_table) // 18, 0, 0)
    for index, (section, layout) in enumerate(zip(sections, layouts)):
        offset = 20 + index * 40
        raw_offset, relocation_offset = layout
        data[offset:offset + 8] = section.name.encode("ascii").ljust(8, b"\0")
        struct.pack_into("<IIIIIIHHI", data, offset + 8,
                         0, 0, len(section.raw), raw_offset,
                         relocation_offset, 0, len(section.relocations), 0,
                         CODE_COMDAT)
    return bytes(data)


def _base(*, handler_opcode: int = 0xB8,
          cleanup_storage: int = 6) -> bytes:
    parent = bytearray(b"\x55\x8b\xec\x6a\xff\x68" + bytes(26))
    child = bytearray(21)
    child[0] = 0xC3
    child[11] = handler_opcode
    child[16] = 0xE9
    sections = (
        FixtureSection(".text", bytes(parent), (
            (6, 6, DIR32),       # direct handler label
            (20, 7, DIR32),      # unrelated DIR32 negative control
        )),
        FixtureSection(".text$x", bytes(child), (
            (12, 7, DIR32),      # mov eax, xdata
            (17, 8, REL32),      # jmp __CxxFrameHandler
        )),
    )
    symbols = (
        _symbol(".text", 0, 1, 0, 3, _section_aux(32, 2)),      # 0,1
        _symbol("ctor", 0, 1, FUNCTION_TYPE, 2),                # 2
        _symbol(".text$x", 0, 2, 0, 3,
                _section_aux(21, 2, parent=1, selection=5)),     # 3,4
        _symbol("cleanup", 0, 2, 0, cleanup_storage),           # 5
        _symbol("handler", 11, 2, 0, 6),                        # 6
        _symbol("xdata", 0, 0, 0, 2),                           # 7
        _symbol("frame", 0, 0, FUNCTION_TYPE, 2),               # 8
    )
    return _coff(sections, symbols)


def _target(funclet_size: int = 11) -> bytes:
    text = bytearray(b"\x55\x8b\xec\x6a\xff\x68" + bytes(26))
    struct.pack_into("<I", text, 6, funclet_size)
    sections = (FixtureSection(".text", bytes(text), ((6, 1, DIR32),)),)
    symbols = (
        _symbol("ctor", 0, 1, FUNCTION_TYPE, 2),
        _symbol("unwind13", 0, 0, FUNCTION_TYPE, 2),
    )
    return _coff(sections, symbols)


class EhHandlerNormalizationTest(unittest.TestCase):
    def test_equivalent_owner_addend_is_canonicalized(self):
        before = _base()
        after, rewrites = _canonicalize_matching_eh_handler_owners(
            before, _target())
        self.assertEqual(len(rewrites), 1)
        self.assertEqual(rewrites[0].canonical_name, "unwind13")
        original = CoffObject(before)
        normalized = CoffObject(after)
        original_relocation = next(
            row for row in original.relocations if row.site == 6)
        normalized_relocation = next(
            row for row in normalized.relocations if row.site == 6)
        original_handler = original.symbols[original_relocation.symbol_index]
        normalized_owner = normalized.symbols[normalized_relocation.symbol_index]
        normalized_addend, = struct.unpack_from(
            "<I", normalized.section_bytes(normalized.sections[0]), 6)
        self.assertEqual(normalized_owner.name, "unwind13")
        self.assertEqual(normalized_addend, 11)
        self.assertEqual(original_handler.value,
                         normalized_owner.value + normalized_addend)
        ordinary = next(row for row in normalized.relocations if row.site == 20)
        self.assertEqual(ordinary.symbol_index, 7)

    def test_different_retail_funclet_size_stays_visible(self):
        before = _base()
        after, rewrites = _canonicalize_matching_eh_handler_owners(
            before, _target(8))
        self.assertEqual(rewrites, ())
        self.assertEqual(after, before)

    def test_malformed_handler_thunk_stays_visible(self):
        before = _base(handler_opcode=0x90)
        after, rewrites = _canonicalize_matching_eh_handler_owners(
            before, _target())
        self.assertEqual(rewrites, ())
        self.assertEqual(after, before)

    def test_missing_final_funclet_stays_visible(self):
        before = _base(cleanup_storage=3)
        after, rewrites = _canonicalize_matching_eh_handler_owners(
            before, _target())
        self.assertEqual(rewrites, ())
        self.assertEqual(after, before)


if __name__ == "__main__":
    unittest.main()
