"""Ledger history queries: `status hist-gap` and `status last-exact`."""
import contextlib
import io
import json
import unittest
from unittest.mock import patch

from homm3.core import common
from homm3.match import score_history, status

ROW = status.MatchRow
MARK = score_history.COMMIT_MARK


def _row(unit, fn, cur, rva):
    return f"+{unit}\t{fn}\t{cur:.4f}\t{cur:.4f}\t{cur:.4f}\t{rva}\t-"


HISTORY = "\n".join([
    f"{MARK}aaa 2026-09-01 Add the window",
    "+++ b/config/match_baseline.tsv",
    "+view\t?draw@@\t100.0",                       # legacy row, resolved by name
    _row("town", "?build@@", 90.0, "0x200"),
    f"{MARK}bbb 2026-09-02 Bank the window",
    "-view\t?draw@@\t100.0",
    _row("view", "?draw@@", 100.0, "0x100"),
    _row("town", "?build@@", 100.0, "0x200"),
    f"{MARK}ccc 2026-09-03 Clean the helpers",
    _row("view", "?draw@@", 92.5, "0x100"),
    f"{MARK}ddd 2026-09-04 Rename the builder",
    _row("town", "?construct@@", 99.0, "0x200"),
])


class TimelineTest(unittest.TestCase):
    def setUp(self):
        self.timeline = score_history.parse_history(
            HISTORY, {("view", "?draw@@"): 0x100})

    def test_series_pair_by_rva_and_resolve_legacy_labels(self):
        self.assertEqual([c.sha for c in self.timeline.commits],
                         ["aaa", "bbb", "ccc", "ddd"])
        self.assertEqual([entry[:2] for entry in self.timeline.series[0x100]],
                         [(0, 100.0), (1, 100.0), (2, 92.5)])
        self.assertEqual([entry[:2] for entry in self.timeline.series[0x200]],
                         [(0, 90.0), (1, 100.0), (3, 99.0)])

    def test_peak_is_the_last_commit_at_target_and_drop_the_next(self):
        event = score_history.peak_event(self.timeline, 0x100, 100.0)
        self.assertEqual((event.peak.sha, event.drop.sha, event.drop_score),
                         ("bbb", "ccc", 92.5))
        renamed = score_history.peak_event(self.timeline, 0x200, 100.0)
        self.assertEqual((renamed.peak.sha, renamed.drop.sha), ("bbb", "ddd"))
        self.assertEqual(renamed.drop.subject, "Rename the builder")

    def test_inherited_peak_falls_back_to_the_banked_columns(self):
        text = "\n".join([
            f"{MARK}eee 2026-09-05 Seed the ledger",
            "+hero\t?move@@\t90.0000\t95.0000\t99.0000\t0x300\t-",
            f"{MARK}fff 2026-09-06 Edit the mover",
            "+hero\t?move@@\t88.0000\t88.0000\t99.0000\t0x300\t-"])
        timeline = score_history.parse_history(text)
        event = score_history.peak_event(timeline, 0x300, 99.0)
        self.assertTrue(event.inherited)
        self.assertEqual(event.peak.sha, "fff")
        self.assertIsNone(event.drop)
        measured = score_history.peak_event(timeline, 0x300, 90.0)
        self.assertFalse(measured.inherited)
        self.assertEqual((measured.peak.sha, measured.drop.sha), ("eee", "fff"))

    def test_no_peak_and_no_drop(self):
        never = score_history.peak_event(self.timeline, 0x200, 100.5)
        self.assertIsNone(never.peak)
        held = score_history.peak_event(self.timeline, 0x100, 92.5)
        self.assertEqual(held.peak.sha, "ccc")
        self.assertIsNone(held.drop)


class GapRowsTest(unittest.TestCase):
    ROWS = {("view", "?draw@@"): ROW(92.5, 92.5, 100.0, 0x100),
            ("town", "?build@@"): ROW(99.0, 99.0, 99.5, 0x200),
            ("town", "?noise@@"): ROW(98.0, 98.0, 98.01, 0x300),
            ("hero", "?move@@"): ROW(100.0, 100.0, 100.0, 0x400)}

    def test_min_gap_and_exact_filters(self):
        keys = [key for key, _ in score_history.gap_rows(self.ROWS)]
        self.assertEqual(keys, [("view", "?draw@@"), ("town", "?build@@")])
        exact = [key for key, _ in score_history.gap_rows(self.ROWS, exact_only=True)]
        self.assertEqual(exact, [("view", "?draw@@")])
        loose = score_history.gap_rows(self.ROWS, min_gap=0.0)
        self.assertEqual(len(loose), 3)


SOURCE = """\
VA(0x%08x, 0x20)
int window::draw(int x)
{
    if (x) { return 1; }
    return 0;
}

VA(0x%08x, 0x10)
void window::idle() {}
"""


class DefinitionTest(unittest.TestCase):
    def test_definition_at_follows_the_va_claim(self):
        text = SOURCE % (common.IMAGE_BASE + 0x100, common.IMAGE_BASE + 0x140)
        definition = score_history.definition_at(text, 0x100, "?draw@window@@QAEHH@Z")
        self.assertTrue(definition.startswith("int window::draw(int x)"))
        self.assertTrue(definition.rstrip().endswith("return 0;\n}"))
        self.assertIsNone(score_history.definition_at(text, 0x180, "?x@@"))

    def test_function_diff(self):
        diff = score_history.function_diff(
            "a\nb\n", "a\nc\n", old_name="old", new_name="new")
        self.assertIn("-b", diff)
        self.assertIn("+c", diff)
        self.assertEqual(score_history.function_diff(
            "a\n", "a\n", old_name="o", new_name="n"), "")


class CommandTest(unittest.TestCase):
    ROWS = {("view", "?draw@@"): ROW(92.5, 92.5, 100.0, 0x100),
            ("town", "?construct@@"): ROW(99.0, 99.0, 100.0, 0x200)}

    def setUp(self):
        timeline = score_history.parse_history(HISTORY, {})
        self.enterContext(patch.object(status, "load_baseline", return_value=self.ROWS))
        self.enterContext(patch.object(score_history, "load_timeline",
                                       return_value=timeline))
        self.enterContext(patch.object(score_history, "source_at",
                                       return_value=(None, None)))

    def run_status(self, *argv):
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc = status.main(list(argv))
        return rc, out.getvalue()

    def test_hist_gap_json(self):
        rc, out = self.run_status("hist-gap", "--json")
        self.assertEqual(rc, 0)
        records = json.loads(out)
        self.assertEqual([(r["function"], r["peak"]["commit"], r["drop"]["commit"])
                          for r in records],
                         [("?draw@@", "bbb", "ccc"), ("?construct@@", "bbb", "ddd")])

    def test_last_exact_selects_by_address_or_name(self):
        rc, out = self.run_status("last-exact", "0x400100", "--json")
        self.assertEqual(rc, 0)
        (record,) = json.loads(out)
        self.assertEqual((record["function"], record["peak"]["commit"]),
                         ("?draw@@", "bbb"))
        rc, out = self.run_status("last-exact", "construct")
        self.assertEqual(rc, 0)
        self.assertIn("last at 100.0000: bbb", out)
        self.assertIn("dropped: ddd", out)

    def test_last_exact_refuses_no_match_and_too_many(self):
        self.assertEqual(self.run_status("last-exact", "nothing")[0], 1)
        rc, out = self.run_status("last-exact", "?", "--limit", "1")
        self.assertEqual(rc, 1)
        self.assertIn("2 rows match", out)


if __name__ == "__main__":
    unittest.main()
