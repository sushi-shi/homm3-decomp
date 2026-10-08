"""README score block: a pure function of the banked ledger.

MAX counts describe the current implementation, not HIST; byte weights come
from the retail inventory by RVA, never from a local objdiff report.
"""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.match import status

MANIFEST = ({}, {}, [{"unit": "unit", "source": "src/unit.cpp"},
                     {"unit": "net", "source": "src/net.cpp", "module": "network"}])


class ReadmeCase(unittest.TestCase):
    def render(self, rows, universe, *, accounting=None, readme_text="before\n"):
        with tempfile.TemporaryDirectory() as tmp:
            readme = Path(tmp) / "README.md"
            readme.write_text(readme_text)
            with patch.object(status, "README_PATH", readme), \
                    patch.object(status, "load_baseline", return_value=rows), \
                    patch("homm3.build.configure.load_manifest", return_value=MANIFEST), \
                    patch("homm3.match.universe.summary", return_value=universe), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.write_readme(data_accounting=accounting)
                first = readme.read_text()
                status.write_readme(data_accounting=accounting)
                self.assertEqual(readme.read_text(), first)
            return first


class ReadmeScoreTest(ReadmeCase):
    def test_exact_max_uses_current_implementation_not_historical_peaks(self):
        rows = {
            ("unit", "exact"): status.MatchRow(100, 100, 100, 0),
            ("unit", "dipped"): status.MatchRow(25, 100, 100, 1),
            ("unit", "historic"): status.MatchRow(None, 90, 100, 2),
            ("unit", "near"): status.MatchRow(99.9995, 99.9995, 99.9995, 3),
        }
        universe = (dict.fromkeys(range(6), "target"), dict.fromkeys(range(6), 10),
                    {"target": (6, 60)})
        accounting = dict(totals={'file': {'missing': 23}},
            initializers=[{'verdict': 'exact'}, {'verdict': 'unresolved'}],
            source_initializers={'matches': [{'size': 89}, {'size': 96}]},
            startup_initializers={'matches': [{'size': 125}],
                                  'dependencies': [{'size': 10}]})
        first = self.render(rows, universe, accounting=accounting,
                            readme_text=f"before\n{status.RM_START}\nold\n{status.RM_END}\nafter\n")

        self.assertIn("**Windows `HEROES3.EXE`: 65.00% matched (MAX)** — "
                      "2 / 6 functions exact (33.3%)", first)
        self.assertRegex(first, r"\| CUR\s*\|\s*1 \|\s*37\.50% \| last measured score")
        self.assertRegex(first, r"\| MAX\s*\|\s*2 \|")
        self.assertRegex(first, r"\| HIST\s*\|\s*3 \|")
        table = [[c.strip() for c in line.strip("|").split("|")]
                 for line in first.splitlines() if line.startswith("|")]
        modules = table.index(next(r for r in table if "Functions exact MAX" in r))
        self.assertEqual(table[modules + 2][1:], ["1", "2 / 4 (50.0%)", "97.50%"])
        self.assertEqual(table[modules + 3][2:], ["0 / 2 (0.0%)", "0.0%"])
        self.assertTrue(first.startswith("before\n"))
        self.assertTrue(first.endswith("\nafter\n"))
        self.assertIn('1 / 2 enrolled initializer comparisons exact.', first)
        self.assertIn('3 source-emitted CRT bodies exact (310 bytes).', first)

    def test_retail_sizes_weigh_every_body(self):
        rows = {("unit", "big"): status.MatchRow(100, 100, 100, 1),
                ("net", "small"): status.MatchRow(0, 0, 100, 2)}
        universe = ({1: "target", 2: "target"}, {1: 30, 2: 10}, {"target": (2, 40)})
        text = self.render(rows, universe)
        self.assertIn("75.00% matched (MAX)** — 1 / 2 functions exact", text)
        self.assertRegex(text, r"\| HIST\s*\|\s*2 \|\s*100\.00% \|")
        self.assertRegex(text, r"\| `network` \|\s*1 \|\s*0 / 1 \(0\.0%\) \|\s*0\.00% \|")

    def test_excluded_linked_bodies_do_not_inflate_progress(self):
        categories = {1: "target", 2: "zlib", 3: "init-thunk",
                      4: "import-thunk", 5: "runtime", 6: "eh-funclet"}
        rows = {("unit", str(rva)): status.MatchRow(*(50,) * 3 if rva == 1 else (100,) * 3,
                                                     rva=rva)
                for rva in categories}
        universe = (categories, dict.fromkeys(categories, 10),
                    {cat: (1, 10) for cat in categories.values()})
        banked = {"runtime": {5}, "eh-funclet": set(), "init-thunk": {3, 99}}
        with patch("homm3.verify.generated_code.committed", return_value=banked):
            text = self.render(rows, universe)

        self.assertIn("75.00% matched (MAX)** — 1 / 2 functions exact", text)
        self.assertRegex(text, r"\| CUR\s*\|\s*1 \|")
        self.assertRegex(text, r"\| HIST\s*\|\s*1 \|")
        self.assertNotIn("`(unmatched)`", text)
        self.assertRegex(text, r"\|\s*1 / 2 \(50\.0%\) \|\s*75\.00% \|")
        # Banked verdicts count only census rows of their own category; a
        # category without a banked table stays excluded.
        self.assertRegex(text, r"\| `CRT/C\+\+ runtime`\s*\|\s*1 \|\s*1 \|\s*10 \| library, verified")
        self.assertRegex(text, r"\| `EH unwind funclets`\s*\|\s*1 \|\s*0 \|")
        self.assertRegex(text, r"\| `init/cleanup thunks`\s*\|\s*1 \|\s*1 \|")
        self.assertRegex(text, r"\| `import thunks`\s*\|\s*1 \|\s*— \|\s*10 \| excluded")

    def test_duplicate_retail_body_is_refused(self):
        rows = {("a", "old"): status.MatchRow(1, 1, 1, 0x10),
                ("b", "new"): status.MatchRow(1, 1, 1, 0x10)}
        with self.assertRaisesRegex(ValueError, r"0x10 has several banked labels "
                                                r"\(a old, b new\)"):
            status.ledger_bodies(rows)
        self.assertEqual(status.ledger_bodies({("u", "f"): rows[("a", "old")]}),
                         {0x10: (("u", "f"), rows[("a", "old")])})


class ReadmeDeterminismTest(ReadmeCase):
    """Same ledger, same block: no report, symbol map or fingerprint is read."""

    ROWS = {("unit", "f"): status.MatchRow(80, 90, 100, 1),
            ("unit", "g"): status.MatchRow(100, 100, 100, 2),
            ("net", "h"): status.MatchRow(None, 50, 75, 3)}
    UNIVERSE = ({1: "target", 2: "target", 3: "zlib", 4: "target", 5: "runtime"},
                {1: 17, 2: 5, 3: 11, 4: 3, 5: 2},
                {"target": (3, 25), "zlib": (1, 11), "runtime": (1, 2)})

    def test_report_contents_and_absence_do_not_change_the_block(self):
        blocks = []
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "report.json"
            for contents in (None, {"units": []}, {"units": [{"name": "unit", "functions": [
                    {"name": "f", "size": 999, "fuzzy_match_percent": 1.0},
                    {"name": "extra", "size": 1, "fuzzy_match_percent": 100.0}]}]}):
                if contents is not None:
                    report.write_text(json.dumps(contents))
                with contextlib.ExitStack() as stack:
                    stack.enter_context(patch.object(status, "REPORT", report))
                    for name in ("load_report", "refresh_report", "function_rvas",
                                 "source_hash_pair", "projected_rows"):
                        stack.enter_context(patch.object(
                            status, name, side_effect=AssertionError(name)))
                    blocks.append(self.render(self.ROWS, self.UNIVERSE))
        self.assertEqual(blocks[0], blocks[1])
        self.assertEqual(blocks[0], blocks[2])
        self.assertIn("**Windows `HEROES3.EXE`: 71.67% matched (MAX)** — "
                      "1 / 4 functions exact (25.0%), weighted by size over "
                      "36 bytes of code.", blocks[0])

    def test_build_update_and_render_only_paths_write_identical_blocks(self):
        texts = {}
        for path in ("build", "update", "render-only"):
            with tempfile.TemporaryDirectory() as tmp:
                readme = Path(tmp) / "README.md"
                readme.write_text("intro\n")
                with patch.object(status, "README_PATH", readme), \
                        patch.object(status, "load_baseline", return_value=self.ROWS), \
                        patch("homm3.build.configure.load_manifest", return_value=MANIFEST), \
                        patch("homm3.match.universe.summary", return_value=self.UNIVERSE), \
                        patch.object(status, "require_complete_build"), \
                        patch.object(status, "require_built_sources"), \
                        patch.object(status, "refresh_report", return_value={"units": []}), \
                        patch.object(status, "cmd_update", return_value=0), \
                        patch.object(status, "read_report_view",
                                     return_value=status.ReportView({"units": []}, {})), \
                        patch.object(status, "cmd_summary", return_value=0), \
                        contextlib.redirect_stdout(io.StringIO()):
                    if path == "build":
                        status.write_readme(data_accounting=None)
                    elif path == "update":
                        self.assertEqual(status.main(["update", "--write-readme"]), 0)
                    else:
                        self.assertEqual(status.main(["--write-readme"]), 0)
                texts[path] = readme.read_text()
        self.assertEqual(texts["build"], texts["update"])
        self.assertEqual(texts["build"], texts["render-only"])


if __name__ == "__main__":
    unittest.main()
