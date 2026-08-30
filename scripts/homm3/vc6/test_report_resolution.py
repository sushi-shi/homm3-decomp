#!/usr/bin/env python3
"""Negative controls for silent wall-census public-symbol selection.

Run with ``python3 -m homm3.vc6.test_report_resolution``. The important
control is the flat retail/carve label: until source emits a real public text
symbol, it must be classified as reconstruction work without calling objdump
and printing a false tool failure.
"""
from __future__ import annotations

import unittest

from homm3.vc6 import report


class PublicSymbolResolution(unittest.TestCase):

    def test_exact_mangled_symbol_is_selected(self):
        name = "?BuildBuilding@town@@QAE?AW4type_building_id@@HEE@Z"
        self.assertEqual(
            report._pick_public_symbol(name, name, 0, {name}, {name}),
            (name, 0))

    def test_context_ordinal_is_preserved(self):
        name = "?duplicate@@YAXXZ"
        self.assertEqual(
            report._pick_public_symbol("dc:0x10", name, 2, {name}, {name}),
            (name, 2))

    def test_unique_bare_name_can_select_decorated_symbol(self):
        name = "?SetHeroContext@advManager@@QAEXH_N@Z"
        self.assertEqual(
            report._pick_public_symbol(
                "SetHeroContext", "SetHeroContext", 0, {name}, {name}),
            (name, 0))

    def test_flat_unclaimed_label_is_rejected(self):
        # NEGATIVE CONTROL: this is the measured queue defect. Retail and the
        # synth roster know the address, but the compiled base object exposes
        # no function until source reconstruction lands.
        flat = "TSingleSelectionWindow_TSingleSelectionWindow"
        names = {"?Main@TSingleSelectionWindow@@UAEHXZ"}
        self.assertIsNone(
            report._pick_public_symbol(flat, flat, 0, names, names))

    def test_symbol_missing_on_one_side_is_rejected(self):
        # NEGATIVE CONTROL: never route a solver unless it can read both sides.
        name = "?f@@YAXXZ"
        self.assertIsNone(
            report._pick_public_symbol(name, name, 0, {name}, set()))

    def test_ambiguous_substring_is_rejected(self):
        names = {"?load@game@@QAEXXZ", "?preload@game@@QAEXXZ"}
        self.assertIsNone(
            report._pick_public_symbol("load", "load", 0, names, names))


if __name__ == "__main__":
    unittest.main()
