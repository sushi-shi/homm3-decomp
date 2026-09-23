"""Contract checks for bounded TU-wide AST source generation."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from homm3.match.status import MatchRow
from homm3.vc6 import ast_variants


class AstVariantsTests(unittest.TestCase):
    def test_tu_manifest_collects_distinct_functions_and_prioritizes_lost_max(self):
        source_text = ("#define VA(a,b)\n"
                       "VA(0x00400100, 4)\n"
                       "int first(int x, int y) { return x + y; }\n"
                       "VA(0x00400200, 4)\n"
                       "int second(int x, int y) { return x * y; }\n")
        with tempfile.TemporaryDirectory() as raw:
            source = Path(raw) / "sample.cpp"
            source.write_text(source_text)
            rows = {("sample", "first"): MatchRow(80, 80, 100, 0x100),
                    ("sample", "second"): MatchRow(50, 50, 100, 0x200)}
            with patch.object(ast_variants, "_compilation_args", return_value=["-std=c++98"]), \
                    patch.object(ast_variants.status, "load_baseline", return_value=rows):
                result = ast_variants.manifest_tu(
                    "sample", "first", source, families=("commutative_order",),
                    depth=1, limit=8)
        options = result["axes"][0]["options"]
        self.assertEqual(options[0]["name"], "baseline")
        self.assertEqual(result["generator"]["source_functions"], 2)
        self.assertTrue(options[1]["name"].startswith("00400200:"))
        self.assertTrue(any(option["name"].startswith("00400100:") for option in options))
        self.assertEqual(len({option["name"] for option in options}), len(options))


if __name__ == "__main__":
    unittest.main()
