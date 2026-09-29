#!/usr/bin/env python3
"""Controls for IAT slot spellings no pinned import library proves."""
from __future__ import annotations

import unittest

from homm3.retail_labels.iat import import_symbol


class ImportSymbolTest(unittest.TestCase):
    def test_decorated_vendor_export_is_its_own_symbol(self):
        # smackw32.dll exports `_SmackToBuffer@28`; cl references
        # `__imp__SmackToBuffer@28`, never a third underscore.
        self.assertEqual(import_symbol("_SmackToBuffer@28"),
                         ("__imp__SmackToBuffer@28", "iat-decorated"))
        self.assertEqual(import_symbol("_AIL_serve@0"),
                         ("__imp__AIL_serve@0", "iat-decorated"))

    def test_cplusplus_name_is_its_own_symbol(self):
        self.assertEqual(import_symbol("?f@@YAXXZ"),
                         ("__imp_?f@@YAXXZ", "iat-decorated"))

    def test_undecorated_name_keeps_the_placeholder(self):
        # No stdcall suffix and no leading underscore: nothing proves the
        # decoration, so the cdecl spelling stays a placeholder.
        self.assertEqual(import_symbol("BinkOpen"),
                         ("__imp__BinkOpen", "iat-undecorated"))
        self.assertEqual(import_symbol("_private"),
                         ("__imp___private", "iat-undecorated"))


class OrdinalSymbolTest(unittest.TestCase):
    def test_import_library_binding_names_the_ordinal(self):
        from homm3.retail_labels.iat import ordinal_symbol
        ordinals = {("wsock32.dll", 3): "__imp__closesocket@4"}
        self.assertEqual(ordinal_symbol("WSOCK32.dll", 3, ordinals),
                         ("__imp__closesocket@4", "iat-implib-ordinal"))
        self.assertEqual(ordinal_symbol("WSOCK32.dll", 4, ordinals),
                         ("__imp__wsock32_ordinal_4", "iat-ordinal"))


if __name__ == "__main__":
    unittest.main()
