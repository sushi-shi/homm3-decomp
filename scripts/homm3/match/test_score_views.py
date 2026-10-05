"""Read-only score queries and the status command line."""
import argparse
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.match import score_views, status

ROW = status.MatchRow


def _report(*functions):
    units = {}
    for unit, name, cur in functions:
        units.setdefault(unit, []).append(
            {"name": name, "fuzzy_match_percent": cur, "size": 16})
    return {"units": [{"name": u, "functions": fns} for u, fns in units.items()]}


class FunctionQueryTest(unittest.TestCase):
    def setUp(self):
        self.report = _report(("hero", "?move@@", 80.0), ("hero", "?draw@@", 100.0),
                              ("town", "?build@@", 99.0))
        self.ledger = {("hero", "?move@@"): ROW(80, 90, 95, 0x124dd0, "a"),
                       ("hero", "?draw@@"): ROW(100, 100, 100, 0x124e00, "b"),
                       ("town", "?build@@"): ROW(99, 100, 100, 0x200, "c")}
        self.enterContext(patch.object(status, "load_baseline", return_value=self.ledger))
        self.enterContext(patch.object(status, "function_rvas", return_value={}))
        self.enterContext(patch.object(status, "_pending_function_records", return_value={}))

    def run_functions(self, *argv, stale=None):
        args = status.build_parser().parse_args(["functions", *argv])
        view = status.ReportView(self.report, stale or {})
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(score_views.cmd_functions(view, args), 0)
        return out.getvalue()

    def names(self, *argv):
        return [r["function"] for r in json.loads(self.run_functions(*argv, "--json"))]

    def test_unit_va_and_substring_filters(self):
        self.assertEqual(self.names("--unit", "hero"), ["?draw@@", "?move@@"])
        self.assertEqual(self.names("--unit", "hero", "--unit", "town"),
                         ["?draw@@", "?move@@", "?build@@"])
        self.assertEqual(self.names("--va", "0x00524dd0"), ["?move@@"])
        self.assertEqual(self.names("--va", "0x124e00"), ["?draw@@"])
        self.assertEqual(self.names("MOVE"), ["?move@@"])

    def test_below_uses_max_unless_cur_is_requested(self):
        self.assertEqual(self.names("--non-exact"), ["?move@@"])
        self.assertEqual(self.names("--non-exact", "--cur"), ["?move@@", "?build@@"])
        self.assertEqual(self.names("--below", "95"), ["?move@@"])
        self.assertEqual(self.names("--below", "85", "--cur"), ["?move@@"])

    def test_json_rows_carry_scores_addresses_and_staleness(self):
        rows = json.loads(self.run_functions("--va", "0x524dd0", "--json",
                                             stale={"hero": "unbuilt source edit"}))
        self.assertEqual(rows, [{"unit": "hero", "function": "?move@@",
                                 "va": "0x00524dd0", "rva": "0x124dd0",
                                 "cur": 80.0, "max": 90.0, "hist": 95.0,
                                 "stale": True}])

    def test_text_marks_stale_rows(self):
        text = self.run_functions("--unit", "hero", stale={"hero": "unbuilt source edit"})
        self.assertIn("hero / ?move@@ *", text)
        self.assertIn("  90.00%    80.00%    95.00%  0x124dd0", text)


class DiffTest(unittest.TestCase):
    def record(self, unit, name, cur, maximum, rva):
        return score_views._record((unit, name), cur, maximum, maximum,
                                   rva, ())

    def test_pairs_by_rva_across_renames_and_classifies(self):
        before = {("u", "old"): self.record("u", "old", 80, 90, 0x10),
                  ("u", "same"): self.record("u", "same", 50, 50, 0x20),
                  ("u", "down"): self.record("u", "down", 70, 70, 0x30),
                  ("u", "gone"): self.record("u", "gone", 10, 10, 0x40)}
        after = {("u", "new"): self.record("u", "new", 95, 95, 0x10),
                 ("u", "same"): self.record("u", "same", 50, 50, 0x20),
                 ("u", "down"): self.record("u", "down", 60, 70, 0x30),
                 ("u", "fresh"): self.record("u", "fresh", 5, 5, 0x50)}
        rows = {r["function"]: r for r in score_views.diff_records(before, after)}
        self.assertEqual({name: row["kind"] for name, row in rows.items()},
                         {"new": "improved", "same": "unchanged", "down": "regressed",
                          "fresh": "new", "gone": "removed"})
        self.assertEqual(rows["new"]["renamed_from"], "old")
        self.assertEqual(rows["new"]["before"]["cur"], 80)

    def test_max_change_counts_when_cur_is_unchanged(self):
        before = {("u", "f"): self.record("u", "f", 80, 90, 0x10)}
        after = {("u", "f"): self.record("u", "f", 80, 80, 0x10)}
        self.assertEqual(score_views.diff_records(before, after)[0]["kind"], "regressed")

    def test_snapshot_round_trip_and_ref_fallback(self):
        report = _report(("u", "f", 75.0))
        view = status.ReportView(report, {})
        projected = {("u", "f"): ROW(75, 80, 90, 0x10, "h")}
        with tempfile.TemporaryDirectory() as tmp, \
                patch.object(status, "projected_rows", return_value=projected), \
                patch.object(status, "fresh_fingerprints", return_value=None), \
                patch.object(score_views, "_head", return_value="abc"), \
                contextlib.redirect_stdout(io.StringIO()) as out:
            path = Path(tmp) / "before.json"
            score_views.cmd_snapshot(view, path)
            payload = json.loads(path.read_text())
            self.assertEqual(payload["functions"][0]["max"], 80)
            projected[("u", "f")] = ROW(100, 100, 100, 0x10, "h2")
            out.seek(0)
            out.truncate()
            score_views.cmd_diff(view, str(path), as_json=True)
            result = json.loads(out.getvalue())
        self.assertEqual(result["counts"]["improved"], 1)
        self.assertEqual(result["functions"][0]["before"]["cur"], 75)
        self.assertEqual(result["functions"][0]["after"]["cur"], 100)
        with patch.object(status, "baseline_at_ref",
                          return_value={("u", "f"): ROW(1, 2, 3, 0x10, "x")}) as at_ref:
            before, label = score_views.load_before("origin/main")
        at_ref.assert_called_once_with("origin/main")
        self.assertEqual(label, "ledger at origin/main")
        self.assertEqual(before[("u", "f")]["max"], 2)


class StatusCommandLineTest(unittest.TestCase):
    """status used to parse argv by hand: `status update --help` ran the
    update, and `status functions --help` treated --help as a filter."""

    def quiet(self, argv):
        with contextlib.redirect_stdout(io.StringIO()) as out, \
                contextlib.redirect_stderr(io.StringIO()) as err:
            rc = status.main(argv)
        return rc, out.getvalue() + err.getvalue()

    def test_help_never_runs_a_subcommand(self):
        with patch.object(status, "refresh_report") as report, \
                patch.object(status, "require_built_sources") as built, \
                patch.object(status, "cmd_update") as update, \
                patch.object(status, "read_report_view") as view:
            for argv in (["--help"], ["update", "--help"], ["functions", "-h"],
                         ["diff", "--help"], ["merge-baseline", "--help"]):
                with self.subTest(argv=argv):
                    rc, text = self.quiet(argv)
                    self.assertEqual(rc, 0)
                    self.assertIn("usage: homm3 status", text)
            for mock in (report, built, update, view):
                mock.assert_not_called()

    def test_unknown_flags_and_commands_are_usage_errors(self):
        with patch.object(status, "refresh_report") as report, \
                patch.object(status, "read_report_view") as view:
            for argv in (["update", "--bogus"], ["show"], ["functions", "--nope"],
                         ["check", "--unit", "x"], ["diff"]):
                with self.subTest(argv=argv):
                    self.assertEqual(self.quiet(argv)[0], 2)
            report.assert_not_called()
            view.assert_not_called()

    def test_existing_invocations_parse_as_before(self):
        parse = status.build_parser().parse_args
        args = parse(["update", "--unit", "a", "--unit", "b", "--write-readme"])
        self.assertEqual((args.command, args.unit, args.write_readme),
                         ("update", ["a", "b"], True))
        args = parse(["--write-readme", "update"])
        self.assertTrue(args.write_readme)
        args = parse(["check", "--baseline-ref", "origin/x"])
        self.assertEqual(args.baseline_ref, "origin/x")
        args = parse(["functions", "?draw@@", "hero"])
        self.assertEqual(args.filters, ["?draw@@", "hero"])
        self.assertEqual(parse(["merge-baseline", "a", "b", "c"]).revisions, ["a", "b", "c"])
        self.assertIsNone(parse([]).command)

    def test_read_only_commands_never_take_the_strict_path(self):
        view = status.ReportView(_report(("u", "f", 1.0)), {})
        with patch.object(status, "require_built_sources") as built, \
                patch.object(status, "refresh_report") as report, \
                patch.object(status, "read_report_view", return_value=view), \
                patch.object(status, "cmd_summary", return_value=0) as summary, \
                patch.object(score_views, "cmd_functions", return_value=0) as functions, \
                patch.object(score_views, "cmd_diff", return_value=0) as diff:
            self.assertEqual(self.quiet([])[0], 0)
            self.assertEqual(self.quiet(["functions", "--unit", "u"])[0], 0)
            self.assertEqual(self.quiet(["diff", "--against", "HEAD", "--unit", "u"])[0], 0)
        built.assert_not_called()
        report.assert_not_called()
        summary.assert_called_once()
        functions.assert_called_once()
        self.assertEqual(diff.call_args.kwargs["units"], {"u"})


if __name__ == "__main__":
    unittest.main()
