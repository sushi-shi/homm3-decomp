"""Tooling contracts of the C2 coverage locator (no compiler runs)."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from homm3.vc6 import c2_cover


class CoverTest(unittest.TestCase):
    def test_parse_and_difference(self):
        a = c2_cover.parse_cover("# c2shim call=1\ncover 0003dea7 3\ncover 00001000 65\n")
        b = c2_cover.parse_cover("cover 00001000 65\n")
        self.assertEqual(a, {0x3dea7: 3, 0x1000: 65})
        self.assertEqual(c2_cover.difference(b, a), [(0x3dea7, 0, 3)])

    def test_function_entries_are_sorted_and_unique(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "funcs.tsv"
            path.write_text("entry_rva\tsize\tname\n0x20\t1\tb\n0x10\t1\ta\n0x20\t1\tb\n")
            self.assertEqual(c2_cover.function_entries(path), [0x10, 0x20])


if __name__ == "__main__":
    unittest.main()
