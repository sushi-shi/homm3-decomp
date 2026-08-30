#!/usr/bin/env python3
"""Controls for reviewed reloc-alias precedence over DATA placeholders."""
from __future__ import annotations

import unittest

from homm3.model import _upgrade_dense_data_alias
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


if __name__ == "__main__":
    unittest.main()
