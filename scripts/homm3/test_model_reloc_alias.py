#!/usr/bin/env python3
"""Controls for reviewed reloc-alias precedence over DATA placeholders."""
from __future__ import annotations

import unittest

from homm3.model import _compgen_rows, _upgrade_dense_data_alias
from homm3.retail_labels import Claim


class RelocAliasPrecedenceTest(unittest.TestCase):
    def claim(self, name: str = "_owner") -> Claim:
        return Claim(0x200, name, "data", "reloc-alias", None, "", {})

    def test_reviewed_alias_upgrades_dense_source_data(self):
        row = {
            "rva": 0x200,
            "name": "data_200",
            "unit": "probe",
            "size": "",
            "kind": "data",
            "provenance": "src-DATA",
        }
        upgraded = _upgrade_dense_data_alias(row, self.claim())
        self.assertEqual(upgraded["name"], "_owner")
        self.assertEqual(upgraded["provenance"], "reloc-alias")
        self.assertEqual(row["name"], "data_200")

    def test_different_source_name_stays_fatal(self):
        row = {
            "name": "real_name",
            "provenance": "src-DATA",
        }
        with self.assertRaisesRegex(ValueError, "conflicts"):
            _upgrade_dense_data_alias(row, self.claim())

    def test_non_source_placeholder_stays_fatal(self):
        row = {
            "name": "data_200",
            "provenance": "reloc-target",
        }
        with self.assertRaisesRegex(ValueError, "conflicts"):
            _upgrade_dense_data_alias(row, self.claim())


class CompgenManifestNamesTest(unittest.TestCase):
    def claim(self, rva: int, name: str, *, compgen: bool = True) -> Claim:
        meta = {"raw": name}
        if compgen:
            meta.update(ckind="VECTOR_INSERT", owner="Thing")
        return Claim(rva, name, "func", "src-VA_COMPGEN", 4, "probe", meta)

    def test_duplicate_raw_name_gets_model_rva_suffix(self):
        first = self.claim(0x1000, "__h3cg$probe$vector_insert$Thing")
        second = self.claim(0x2000, "__h3cg$probe$vector_insert$Thing")
        self.assertEqual(
            [name for _claim, name in _compgen_rows([first, second])],
            ["__h3cg$probe$vector_insert$Thing",
             "__h3cg$probe$vector_insert$Thing_2000"])

    def test_dedup_matches_main_model_across_non_compgen_claims(self):
        ordinary = self.claim(0x1000, "shared", compgen=False)
        compiler = self.claim(0x2000, "shared")
        self.assertEqual(_compgen_rows([ordinary, compiler])[0][1],
                         "shared_2000")

if __name__ == "__main__":
    unittest.main()
