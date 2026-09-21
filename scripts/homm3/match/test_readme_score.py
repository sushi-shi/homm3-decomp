"""README MAX counts describe the current implementation, not HIST."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.match import status


class ReadmeScoreTest(unittest.TestCase):
    def test_exact_max_uses_current_implementation_not_historical_peaks(self):
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
                    patch.object(status, "function_rvas", return_value={
                        ("unit", fn["name"]): i for i, fn in enumerate(functions)
                    }), \
                    patch("homm3.build.configure.load_manifest", return_value=(
                        {}, {}, [{"unit": "unit", "source": "src/unit.cpp"}])), \
                    patch("homm3.match.universe.summary", return_value=(
                        dict.fromkeys(range(6), "target"), {},
                        {"target": (6, 60)})), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.write_readme(report)
                first = readme.read_text()
                status.write_readme(report)
                self.assertEqual(readme.read_text(), first)

        self.assertIn("**Executable MAX: 65.00%**", first)
        self.assertIn("**CUR diagnostics** — 1 / 6 functions exact (16.7%)", first)
        self.assertIn("**Function exact MAX** — 2 / 6 current implementations "
                      "(33.3%)", first)
        table = [[c.strip() for c in line.strip("|").split("|")]
                 for line in first.splitlines() if line.startswith("|")]
        self.assertIn("Function exact MAX", table[0])
        self.assertEqual(table[2][2:4], ["2 / 4 (50.0%)", "1 / 4 (25.0%)"])
        self.assertEqual(table[2][4:], ["97.50%", "56.25%"])
        self.assertEqual(table[3][2:4], ["0 / 2 (0.0%)", "0 / 2 (0.0%)"])
        self.assertTrue(first.startswith("before\n"))
        self.assertTrue(first.endswith("\nafter\n"))

    def test_exact_max_follows_source_label_rename_by_retail_rva(self):
        report = {"units": [{"name": "unit", "functions": [{
            "name": "?CompilerSpelling@@YAXXZ", "size": 10,
            "fuzzy_match_percent": 25,
        }]}]}
        baseline = {
            ("unit", "normalizedSourceName"): status.MatchRow(
                25, 100, 100, 0x1234),
        }
        with tempfile.TemporaryDirectory() as tmp:
            readme = Path(tmp) / "README.md"
            readme.write_text("before\n")
            with patch.object(status, "README_PATH", readme), \
                    patch.object(status, "load_baseline",
                                 return_value=baseline), \
                    patch.object(status, "function_rvas", return_value={
                        ("unit", "?CompilerSpelling@@YAXXZ"): 0x1234,
                    }), \
                    patch("homm3.build.configure.load_manifest", return_value=(
                        {}, {}, [{"unit": "unit", "source": "src/unit.cpp"}])), \
                    patch("homm3.match.universe.summary", return_value=(
                        {0x1234: "target"}, {}, {"target": (1, 10)})), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.write_readme(report)
                text = readme.read_text()

        self.assertIn("**Executable MAX: 100.00%**", text)
        self.assertIn("**CUR diagnostics** — 0 / 1 functions exact", text)
        self.assertIn("**Function exact MAX** — 1 / 1 current implementations",
                      text)
        self.assertRegex(text, r"\|\s*100\.00% \|\s*25\.00% \|")


    def test_excluded_linked_bodies_do_not_inflate_progress(self):
        categories = {1: "target", 2: "zlib", 3: "init-thunk",
                      4: "import-thunk", 5: "runtime", 6: "eh-funclet"}
        functions = [{"name": str(rva), "size": 10,
                      "fuzzy_match_percent": 50 if rva == 1 else 100}
                     for rva in categories]
        report = {"units": [{"name": "unit", "functions": functions}]}
        # Fall back to the baseline RVA when the current symbol map lacks it.
        baseline = {("unit", str(rva)): status.MatchRow(100, 100, 100, rva)
                    for rva in categories if rva != 1}
        with tempfile.TemporaryDirectory() as tmp:
            readme = Path(tmp) / "README.md"
            readme.write_text("")
            with patch.object(status, "README_PATH", readme), \
                    patch.object(status, "load_baseline", return_value=baseline), \
                    patch.object(status, "function_rvas",
                                 return_value={("unit", "1"): 1}), \
                    patch("homm3.build.configure.load_manifest", return_value=(
                        {}, {}, [{"unit": "unit", "source": "src/unit.cpp"}])), \
                    patch("homm3.match.universe.summary", return_value=(
                        categories, {}, {cat: (1, 10)
                                         for cat in categories.values()})), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.write_readme(report)
                text = readme.read_text()

        self.assertIn("**Executable MAX: 75.00%**", text)
        self.assertIn("**Function exact MAX** — 1 / 2", text)
        self.assertIn("**CUR diagnostics** — 1 / 2", text)
        self.assertIn("(2 in linked units)", text)
        self.assertNotIn("`(unmatched)`", text)
        self.assertRegex(text, r"\|\s*75\.00% \|\s*75\.00% \|")

if __name__ == "__main__":
    unittest.main()
