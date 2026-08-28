#!/usr/bin/env python3
"""Hermetic controls for paired equivalent-relocation normalization."""
from __future__ import annotations

import struct
import unittest

from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.normalize_objs import (
    DIR32, FUNCTION_TYPE,
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


if __name__ == "__main__":
    unittest.main()
