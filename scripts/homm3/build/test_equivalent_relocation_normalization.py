#!/usr/bin/env python3
"""Hermetic controls for paired equivalent-relocation normalization."""
from __future__ import annotations

import struct
import unittest

from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.normalize_objs import (
    DIR32, FUNCTION_TYPE,
    _canonicalize_except_list_literals,
    _canonicalize_equivalent_relocations,
)
from homm3.build.test_eh_handler_normalization import (
    FixtureSection, _coff, _symbol,
)


def _base(*, literal: int = 0x00401020,
          field_addend: int = 4, anchor_addend: int = 0,
          literal_relocation_type: int | None = None) -> bytes:
    text = (b"\x05" + struct.pack("<I", literal) +
            b"\xa1" + struct.pack("<I", field_addend) +
            b"\xa1" + struct.pack("<I", anchor_addend) + b"\xc3")
    relocations = []
    if literal_relocation_type is not None:
        relocations.append((1, 1, literal_relocation_type))
    relocations.extend(((6, 1, DIR32), (11, 1, DIR32)))
    return _coff(
        (FixtureSection(".text", text, tuple(relocations)),),
        (
            _symbol("probe", 0, 1, FUNCTION_TYPE, 2),
            _symbol("source", 0, 0, 0, 2),
        ),
    )


def _target(*, literal_addend: int = 0x20,
            field_addend: int = 0, anchor_addend: int = 0) -> bytes:
    text = (b"\x05" + struct.pack("<I", literal_addend) +
            b"\xa1" + struct.pack("<I", field_addend) +
            b"\xa1" + struct.pack("<I", anchor_addend) + b"\xc3")
    return _coff(
        (FixtureSection(".text", text, (
            (1, 1, DIR32),
            (6, 2, DIR32),
            (11, 3, DIR32),
        )),),
        (
            _symbol("probe", 0, 1, FUNCTION_TYPE, 2),
            _symbol("code", 0, 0, 0, 2),
            _symbol("field", 0, 0, 0, 2),
            _symbol("owner", 0, 0, 0, 2),
        ),
    )


AUTHORITY = {
    "code": (0x1000, "code"),
    "field": (0x2004, "data"),
    "owner": (0x2000, "data"),
}


def _except_base(*, symbol_name: str = "__except_list",
                 operand: int = 0, prefix: bytes = b"\x64\xa1") -> bytes:
    text = prefix + struct.pack("<I", operand) + b"\xc3"
    short_name = symbol_name if len(symbol_name) <= 8 else "except"
    payload = bytearray(_coff(
        (FixtureSection(".text", text, ((len(prefix), 1, DIR32),)),),
        (
            _symbol("probe", 0, 1, FUNCTION_TYPE, 2),
            _symbol(short_name, 0, 0, 0, 2),
        ),
    ))
    if len(symbol_name) > 8:
        symbol_offset, = struct.unpack_from("<I", payload, 8)
        payload[symbol_offset + 18:symbol_offset + 26] = struct.pack(
            "<II", 0, 4)
        encoded = symbol_name.encode("ascii") + b"\0"
        payload[-4:] = struct.pack("<I", 4 + len(encoded)) + encoded
    return bytes(payload)


def _except_target(*, operand: int = 0,
                   prefix: bytes = b"\x64\xa1") -> bytes:
    text = prefix + struct.pack("<I", operand) + b"\xc3"
    return _coff(
        (FixtureSection(".text", text, ()),),
        (_symbol("probe", 0, 1, FUNCTION_TYPE, 2),),
    )


class EquivalentRelocationNormalizationTest(unittest.TestCase):
    def assert_same_relocation_semantics(self, original, normalized):
        self.assertEqual(
            (original.section, original.site, original.symbol_index,
             original.typ),
            (normalized.section, normalized.site, normalized.symbol_index,
             normalized.typ))

    def test_literal_and_field_forms_are_canonicalized(self):
        normalized_payload, literals, aggregates = \
            _canonicalize_equivalent_relocations(
                _base(), _target(), AUTHORITY)
        self.assertEqual((literals, aggregates), (1, 1))
        normalized = CoffObject(normalized_payload)
        rows = {row.site: row for row in normalized.relocations}
        text = normalized.section_bytes(normalized.sections[0])
        literal, = struct.unpack_from("<I", text, 1)
        field, = struct.unpack_from("<I", text, 6)
        self.assertEqual(literal, 0x00401020)
        self.assertNotIn(1, rows)
        self.assertEqual(len(normalized.relocations), 2)
        self.assertEqual(field, 4)
        self.assertEqual(normalized.symbols[rows[6].symbol_index].name,
                         "owner")
        self.assertEqual(rows[11].symbol_index, 3)

    def test_wrong_candidate_literal_stays_visible(self):
        before = _target()
        after, literals, _aggregates = _canonicalize_equivalent_relocations(
            _base(literal=0x00401024), before, AUTHORITY)
        self.assertEqual(literals, 0)
        original = CoffObject(before)
        normalized = CoffObject(after)
        self.assert_same_relocation_semantics(
            next(row for row in original.relocations if row.site == 1),
            next(row for row in normalized.relocations if row.site == 1))

    def test_candidate_relocation_of_another_type_stays_visible(self):
        before = _target()
        after, literals, _aggregates = _canonicalize_equivalent_relocations(
            _base(literal_relocation_type=0x0014), before, AUTHORITY)
        self.assertEqual(literals, 0)
        original = CoffObject(before)
        normalized = CoffObject(after)
        self.assert_same_relocation_semantics(
            next(row for row in original.relocations if row.site == 1),
            next(row for row in normalized.relocations if row.site == 1))

    def test_missing_equal_addend_anchor_keeps_field_split_visible(self):
        before = _target(anchor_addend=8)
        after, _literals, aggregates = \
            _canonicalize_equivalent_relocations(
                _base(), before, AUTHORITY)
        self.assertEqual(aggregates, 0)
        original = CoffObject(before)
        normalized = CoffObject(after)
        self.assert_same_relocation_semantics(
            next(row for row in original.relocations if row.site == 6),
            next(row for row in normalized.relocations if row.site == 6))

    def test_different_resolved_field_address_stays_visible(self):
        authority = dict(AUTHORITY)
        authority["field"] = (0x2010, "data")
        before = _target()
        after, _literals, aggregates = \
            _canonicalize_equivalent_relocations(
                _base(), before, authority)
        self.assertEqual(aggregates, 0)
        original = CoffObject(before)
        normalized = CoffObject(after)
        self.assert_same_relocation_semantics(
            next(row for row in original.relocations if row.site == 6),
            next(row for row in normalized.relocations if row.site == 6))

    def test_except_list_literal_zero_is_canonicalized(self):
        normalized_payload, count = _canonicalize_except_list_literals(
            _except_base(), _except_target())
        self.assertEqual(count, 1)
        self.assertEqual(CoffObject(normalized_payload).relocations, ())

    def test_nonzero_retail_except_list_operand_stays_visible(self):
        before = _except_base()
        after, count = _canonicalize_except_list_literals(
            before, _except_target(operand=4))
        self.assertEqual(count, 0)
        self.assertEqual(after, before)

    def test_other_candidate_symbol_stays_visible(self):
        before = _except_base(symbol_name="ordinary")
        after, count = _canonicalize_except_list_literals(
            before, _except_target())
        self.assertEqual(count, 0)
        self.assertEqual(after, before)

    def test_non_fs_operand_stays_visible(self):
        before = _except_base(prefix=b"\x90\xa1")
        after, count = _canonicalize_except_list_literals(
            before, _except_target(prefix=b"\x90\xa1"))
        self.assertEqual(count, 0)
        self.assertEqual(after, before)


if __name__ == "__main__":
    unittest.main()
