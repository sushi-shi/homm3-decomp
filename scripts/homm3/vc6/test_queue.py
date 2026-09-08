#!/usr/bin/env python3
"""Negative controls for banked-MAX queue routing.

Run with ``python3 -m homm3.vc6.test_queue``.
"""
from __future__ import annotations

import unittest
import contextlib
import io
from pathlib import Path
import tempfile
from types import SimpleNamespace
from unittest.mock import patch

from homm3.match.status import MatchRow
from homm3.vc6 import queue


def _report(*functions):
    return {"units": [{"name": "unit", "functions": list(functions)}]}


class BankedMaxRouting(unittest.TestCase):

    def test_default_and_polish_alias_route_to_polishing(self):
        from homm3.vc6.__main__ import _build_parser
        for argv in (["queue"], ["queue", "--polish"]):
            with patch.object(queue, "_run_polish", return_value=0) as polish, \
                    patch.object(queue, "_run_admission") as admission:
                self.assertEqual(queue.run(_build_parser().parse_args(argv)), 0)
                polish.assert_called_once()
                admission.assert_not_called()

    def test_smallest_routes_to_combined_queue(self):
        from homm3.vc6.__main__ import _build_parser
        args = _build_parser().parse_args(["queue", "--smallest"])
        with patch.object(queue, "_run_smallest", return_value=0) as smallest, \
                patch.object(queue, "_run_polish") as polish, \
                patch.object(queue, "_run_admission") as admission:
            self.assertEqual(queue.run(args), 0)
            smallest.assert_called_once()
            polish.assert_not_called()
            admission.assert_not_called()

    def test_display_limit_keeps_complete_generated_census(self):
        targets = [("unit", f"fn{i}", score, 90.0, 100)
                   for i, score in enumerate((10.0, 20.0, 30.0))]
        with tempfile.TemporaryDirectory() as tmp, patch.object(queue._common, "REPO", Path(tmp)), \
                patch.object(queue, "_load_maxima", return_value={}), \
                patch.object(queue, "_targets", return_value=targets), \
                patch.object(queue.diagnose, "route", return_value=None):
            for limit, shown in ((1, 1), (2, 2), (0, 3)):
                with contextlib.redirect_stdout(io.StringIO()) as out:
                    queue.run(SimpleNamespace(unit=None, quiet=True, limit=limit, diagnose=True))
                self.assertEqual(out.getvalue().count("%  unit:fn"), shown)
                census = (Path(tmp) / "evidence/wall-census.tsv").read_text()
                self.assertIn("fn0", census)
                self.assertIn("fn2", census)
                self.assertNotIn("reconstruct", census)

    def test_default_ranking_does_not_run_expensive_diagnosis(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(queue._common, "REPO", Path(tmp)), \
                patch.object(queue, "_load_maxima", return_value={}), \
                patch.object(queue, "_targets", return_value=[("unit", "body", 25.0, 10.0, 100)]), \
                patch.object(queue.diagnose, "route") as diagnose, \
                contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(queue.run(SimpleNamespace(unit=None, limit=20)), 0)
            diagnose.assert_not_called()
            census = (Path(tmp) / "evidence/wall-census.tsv").read_text()
            self.assertIn(
                "25.0000\t25.0000\t0.0000\t10.0000\t100\tunit\tbody",
                census)

    def test_banked_exact_dips_and_missing_scores_are_excluded(self):
        for current in ({"fuzzy_match_percent": 12.0}, {}):
            fn = dict(name="banked", size=100, **current)
            self.assertEqual(queue._partition_targets(
                _report(fn), {("unit", "banked"): 100.0},
                {("unit", "banked")}), [])

    def test_current_scores_do_not_reorder_banked_maxima(self):
        data = _report(
            {"name": "easy", "size": 1000, "fuzzy_match_percent": 1.0},
            {"name": "hard", "size": 10, "fuzzy_match_percent": 99.0},
            {"name": "tie_larger", "size": 20, "fuzzy_match_percent": 70.0})
        maxima = {("unit", "easy"): 90.0, ("unit", "hard"): 20.0,
                  ("unit", "tie_larger"): 20.0}
        targets = queue._partition_targets(data, maxima, set(maxima))
        self.assertEqual([r[1] for r in targets], ["tie_larger", "hard", "easy"])
        self.assertEqual([r[2] for r in targets], [20.0, 20.0, 90.0])

    def test_name_or_report_score_does_not_prove_compiled_body(self):
        data = _report({"name": "flat", "size": 5},
                       {"name": "?disabled@@YAXXZ", "size": 100},
                       {"name": "stale", "size": 200, "fuzzy_match_percent": 90})
        targets = queue._partition_targets(data, {}, {("unit", "flat")})
        self.assertEqual(targets, [("unit", "flat", 0.0, None, 5)])

    def test_mangled_scoreless_compiled_body_is_not_hidden(self):
        fn = {"name": "?claimed@@YAXXZ", "size": 12}
        self.assertEqual(queue._partition_targets(
            _report(fn), {}, {("unit", fn["name"])}),
            [("unit", fn["name"], 0.0, None, 12)])

    def test_history_does_not_hide_work_below_current_implementation_max(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "baseline.tsv"
            path.write_text("unit\tbanked\t12\t90\t100\t0x100\t-\n"
                            "unit\tnear\t99.9995\t99.9995\t99.9995\t0x200\t-\n")
            maxima = queue._load_maxima(path)
            history = queue._load_history(path)
        self.assertEqual(maxima[("unit", "banked")], 90.0)
        self.assertEqual(history[("unit", "banked")], 100.0)
        targets = queue._partition_targets(
            _report({"name": "banked"}, {"name": "near"}), maxima, set(maxima))
        self.assertEqual([r[1] for r in targets], ["banked", "near"])

    def test_compiled_inventory_requires_defined_executable_function(self):
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.build.canonicalize_data_symbols import FUNCTION_TYPE
        with tempfile.TemporaryDirectory() as tmp, patch.object(queue._common, "REPO", Path(tmp)):
            obj = Path(tmp) / "build/objdiff/normalized/base/unit.obj"
            obj.parent.mkdir(parents=True)
            obj.write_bytes(_coff((FixtureSection(".text", b"\xc3", ()),), (
                _symbol("body", 0, 1, FUNCTION_TYPE, 2),
                _symbol("?undef", 0, 0, FUNCTION_TYPE, 2),
                _symbol("label", 0, 1, 0, 2))))
            self.assertEqual(queue._compiled_functions(_report()), {("unit", "body")})


class AdmissionRouting(unittest.TestCase):

    def test_explicit_admission_excludes_every_admitted_source_body(self):
        # NEGATIVE CONTROL: the admission campaign must not spend time on a
        # scored residual or on a mangled body objdiff merely cannot score.
        data = _report(
            {"name": "flat_carve", "size": 100},
            {"name": "?undiffable@@YAXXZ", "size": 90},
            {"name": "?residual@@YAXXZ", "size": 80,
             "fuzzy_match_percent": 12.0},
        )
        baseline = "\n".join((
            "unit\tflat_carve\t0\t0\t0\t0x100\t-",
            "unit\t?undiffable@@YAXXZ\t0\t0\t0\t0x200\t-",
            "unit\t?residual@@YAXXZ\t12\t12\t12\t0x300\t-",
        ))
        links = "\n".join((
            "rva\tsize\trelation\towner_or_bracket\tcandidates\tlabel",
            "0x100\t100\tin-span\tunit\tunit\tflat_carve",
            "0x400\t1000\tbracketed\ta..b\ta,b\tnew_largest",
            "0x500\t2000\tin-span\truntime\truntime\tnot_a_target",
        ))
        category = {0x100: "target", 0x200: "target", 0x300: "target",
                    0x400: "target", 0x500: "runtime"}
        sizes = {0x100: 100, 0x200: 90, 0x300: 80,
                 0x400: 1000, 0x500: 2000}
        rows = queue._admission_rows_from_text(
            data, baseline, links, category, sizes)
        self.assertEqual([r["rva"] for r in rows], [0x400, 0x100])
        self.assertEqual(rows[0]["state"], "bracketed")
        self.assertEqual(rows[1]["state"], "carcass")


class SmallestFirstRouting(unittest.TestCase):

    def test_combines_states_sorts_by_size_and_deduplicates_rvas(self):
        data = _report(
            {"name": "large", "fuzzy_match_percent": 80},
            {"name": "alias", "fuzzy_match_percent": 90},
            {"name": "small", "fuzzy_match_percent": 95},
            {"name": "exact", "fuzzy_match_percent": 100},
        )
        baseline = {
            ("unit", "large"): MatchRow(80, 80, 80, 0x100),
            ("unit", "alias"): MatchRow(70, 70, 99, 0x100),
            ("unit", "small"): MatchRow(95, 95, 95, 0x200),
            ("unit", "exact"): MatchRow(100, 100, 100, 0x300),
        }
        admission = [{
            "state": "bracketed", "size": 1, "rva": 0x400,
            "relation": "bracketed", "owner": "a..b",
            "candidates": "a,b", "label": "tiny", "action": "resolve",
        }, {
            # An admission alias of compiled RVA 0x200 must not duplicate it.
            "state": "carcass", "size": 50, "rva": 0x200,
            "relation": "in-span", "owner": "unit",
            "candidates": "unit", "label": "stale", "action": "enable",
        }]
        rows = queue._smallest_rows_from_parts(
            data, baseline, admission,
            {0x100: "target", 0x200: "target", 0x300: "target",
             0x400: "target"},
            {0x100: 50, 0x200: 10, 0x300: 5, 0x400: 1},
            set(baseline), set())
        self.assertEqual([row["rva"] for row in rows], [0x400, 0x200, 0x100])
        self.assertEqual(rows[0]["state"], "bracketed")
        self.assertEqual(rows[1]["state"], "compiled")
        self.assertEqual(rows[2]["maximum"], 80)
        self.assertEqual(rows[2]["historical"], 99)

    def test_current_max_not_hist_controls_exact_exclusion(self):
        baseline = {("unit", "lost"): MatchRow(80, 80, 100, 0x100)}
        rows = queue._smallest_rows_from_parts(
            _report({"name": "lost", "fuzzy_match_percent": 80}),
            baseline, [], {0x100: "target"}, {0x100: 12},
            set(baseline), set())
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["historical"], 100)

    def test_banked_exact_admission_alias_is_excluded(self):
        baseline = {("unit", "lost"): MatchRow(0, 100, 100, 0x100)}
        admission = [{
            "state": "carcass", "size": 12, "rva": 0x100,
            "relation": "in-span", "owner": "unit",
            "candidates": "unit", "label": "lost", "action": "enable",
        }]
        rows = queue._smallest_rows_from_parts(
            _report(), baseline, admission, {0x100: "target"},
            {0x100: 12}, set(), set())
        self.assertEqual(rows, [])

    def test_parked_rvas_are_excluded(self):
        baseline = {("unit", "body"): MatchRow(50, 50, 50, 0x100)}
        rows = queue._smallest_rows_from_parts(
            _report({"name": "body", "fuzzy_match_percent": 50}),
            baseline, [{
                "state": "unmapped", "size": 2, "rva": 0x200,
                "relation": "unmapped", "owner": "", "candidates": "",
                "label": "", "action": "locate",
            }], {0x100: "target", 0x200: "target"},
            {0x100: 10, 0x200: 2}, set(baseline), {0x100, 0x200})
        self.assertEqual(rows, [])

    def test_parked_ledger_requires_hex_rvas(self):
        text = "rva\tsize\tstate\n0x10\t1\tparked\n0x20\t2\tunresolved\n"
        self.assertEqual(queue._parked_rvas_from_text(text), {0x10, 0x20})
        with self.assertRaisesRegex(ValueError, "invalid parked-ledger row"):
            queue._parked_rvas_from_text("rva\nnot-hex\n")

    def test_admission_queue_is_largest_first(self):
        rows = queue._admission_rows_from_text(
            _report(), "",
            "rva\tsize\trelation\towner_or_bracket\tcandidates\tlabel\n"
            "0x10\t10\tunmapped\t\t\tshort\n"
            "0x20\t20\tunmapped\t\t\tlong",
            {0x10: "target", 0x20: "target"}, {0x10: 10, 0x20: 20})
        self.assertEqual([r["rva"] for r in rows], [0x20, 0x10])


if __name__ == "__main__":
    unittest.main()
