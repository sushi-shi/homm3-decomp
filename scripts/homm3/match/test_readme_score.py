"""README exact counts retain peaks through current-score dips."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.match import status


class ReadmeScoreTest(unittest.TestCase):
    def test_exact_max_survives_dips_missing_scores_and_historical_peaks(self):
        functions = [
            {"name": "exact", "size": 10, "fuzzy_match_percent": 100},
            {"name": "dipped", "size": 10, "fuzzy_match_percent": 25},
            {"name": "historic", "size": 10},
            {"name": "near", "size": 10, "fuzzy_match_percent": 99.9995},
        ]
        baseline = {
            ("unit", "dipped"): status.MatchRow(25, 100, 100),
            ("unit", "historic"): status.MatchRow(None, 90, 100),
            ("unit", "near"): status.MatchRow(99.9995, 99.9995, 99.9995),
        }
        report = {"units": [{"name": "unit", "functions": functions}]}
        with tempfile.TemporaryDirectory() as tmp:
            readme = Path(tmp) / "README.md"
            readme.write_text(f"before\n{status.RM_START}\nold\n{status.RM_END}\nafter\n")
            with patch.object(status, "README_PATH", readme), \
                    patch.object(status, "load_baseline", return_value=baseline), \
                    patch("homm3.build.configure.load_manifest", return_value=(
                        {}, {}, [{"unit": "unit", "source": "src/unit.cpp"}])), \
                    patch("homm3.match.universe.summary", return_value=(
                        {}, {}, {"target": (6, 60)})), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.write_readme(report)
                first = readme.read_text()
                status.write_readme(report)
                self.assertEqual(readme.read_text(), first)

        self.assertIn("**Match score** — 1 / 6 functions exact (16.7%)", first)
        self.assertIn("**Function exact MAX** — 3 / 6 functions (50.0%)", first)
        table = [[c.strip() for c in line.strip("|").split("|")]
                 for line in first.splitlines() if line.startswith("|")]
        self.assertIn("Function exact MAX", table[0])
        self.assertEqual(table[2][2:4], ["1 / 4 (25.0%)", "3 / 4 (75.0%)"])
        self.assertEqual(table[2][4:], ["56.25%", "100.00%"])
        self.assertEqual(table[3][2:4], ["0 / 2 (0.0%)", "0 / 2 (0.0%)"])
        self.assertTrue(first.startswith("before\n"))
        self.assertTrue(first.endswith("\nafter\n"))


if __name__ == "__main__":
    unittest.main()
