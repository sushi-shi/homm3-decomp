#!/usr/bin/env python3
"""Hermetic tests for cur/max/hist baseline evolution."""
from __future__ import annotations

import contextlib
import io
import unittest
from unittest import mock

from homm3.match.banked_rows import missing_rows, parse_history, selftest
from homm3.match.status import (_canonical_definition_text, _definition_text,
                                MatchRow, checkpoint_drops, cmd_check,
                                seed_historical_maxima, update_rows)


class UpdateRowsTest(unittest.TestCase):
    def test_rva_migrates_history_across_label_promotion(self):
        old = {("unit", "flat_name"): MatchRow(75.0, 90.0, 95.0, 0x1234)}
        rows, stats = update_rows(
            {("unit", "?Decorated@@YAXXZ"): 100.0}, old,
            {("unit", "?Decorated@@YAXXZ"): 0x1234})
        row = rows[("unit", "?Decorated@@YAXXZ")]
        self.assertEqual((row.cur, row.max, row.hist, row.rva),
                         (100.0, 100.0, 100.0, 0x1234))
        self.assertNotIn(("unit", "flat_name"), rows)
        self.assertEqual(stats["migrated"], 1)

    def test_dip_never_lowers_checkpoint_or_history(self):
        key = ("unit", "function")
        old = {key: MatchRow(98.0, 98.0, 99.0, 0x5678)}
        rows, stats = update_rows({key: 80.0}, old, {key: 0x5678})
        self.assertEqual((rows[key].cur, rows[key].max, rows[key].hist),
                         (80.0, 98.0, 99.0))
        self.assertNotIn("lowered", stats)

    def test_unaccepted_drop_keeps_max(self):
        key = ("unit", "function")
        old = {key: MatchRow(98.0, 98.0, 98.0, 0x5678)}
        rows, _stats = update_rows({key: 80.0}, old, {key: 0x5678})
        self.assertEqual((rows[key].cur, rows[key].max, rows[key].hist),
                         (80.0, 98.0, 98.0))

    def test_unchanged_below_max_function_is_not_a_drop(self):
        key = ("unit", "function")
        rows = {key: MatchRow(80.0, 98.0, 99.0, 0x5678, "same")}
        self.assertEqual(
            checkpoint_drops({key: 80.0}, {key: "same"}, rows), [])

    def test_changed_function_is_reported_only_when_current_score_falls(self):
        key = ("unit", "function")
        rows = {key: MatchRow(98.0, 98.0, 99.0, 0x5678, "old")}
        self.assertEqual(
            checkpoint_drops({key: 99.0}, {key: "new"}, rows), [])
        self.assertEqual(
            checkpoint_drops({key: 75.0}, {key: "new"}, rows),
            [(key, 98.0, 98.0, 75.0)])

    def test_changed_holdout_below_banked_max_is_never_reported(self):
        # NEGATIVE CONTROL for the standing order "we chase MAX, not cur":
        # a row already below its banked MAX is a holdout; even when its own
        # source changed and it fell further, the tooling stays silent.
        key = ("unit", "function")
        rows = {key: MatchRow(80.0, 98.0, 99.0, 0x5678, "old")}
        self.assertEqual(
            checkpoint_drops({key: 75.0}, {key: "new"}, rows), [])
        self.assertEqual(
            checkpoint_drops({key: None}, {key: "new"}, rows), [])

    def test_unrelated_dip_with_banked_max_never_fails_check(self):
        touched = ("touched.obj", "improved")
        unrelated = ("unrelated.obj", "collateral")
        previous = {
            touched: MatchRow(90.0, 90.0, 90.0, 0x1000, "touched-old"),
            unrelated: MatchRow(
                98.0, 98.0, 99.0, 0x2000, "unrelated-same"),
        }
        current = {touched: 100.0, unrelated: 80.0}
        report = {
            "units": [
                {"name": unit, "functions": [
                    {"name": fn, "fuzzy_match_percent": value}
                ]}
                for (unit, fn), value in current.items()
            ]
        }

        output = io.StringIO()
        with mock.patch("homm3.match.status.load_baseline",
                        return_value=previous), mock.patch(
                            "homm3.match.status.source_hashes",
                            return_value={
                                touched: "touched-new",
                                unrelated: "unrelated-same",
                            }), contextlib.redirect_stdout(output):
            self.assertEqual(cmd_check(report), 0)

        self.assertEqual(previous[unrelated].max, 98.0)
        self.assertNotIn("unrelated.obj collateral", output.getvalue())
        self.assertIn("holdouts below their banked MAX are never reported",
                      output.getvalue())

    def test_changed_regression_is_reported_with_held_max(self):
        key = ("unit", "function")
        report = {"units": [{"name": "unit", "functions": [{
            "name": "function", "fuzzy_match_percent": 75.0,
        }]}]}
        rows = {key: MatchRow(98.0, 98.0, 99.0, 0x5678, "old")}
        output = io.StringIO()
        with mock.patch("homm3.match.status.load_baseline",
                        return_value=rows), mock.patch(
                            "homm3.match.status.source_hashes",
                            return_value={key: "new"}), \
                contextlib.redirect_stdout(output):
            self.assertEqual(cmd_check(report), 0)
        self.assertIn("98.00% -> 75.00% (MAX held at 98.00%)",
                      output.getvalue())

    def test_unknown_fingerprint_is_not_mistaken_for_an_edit(self):
        key = ("unit", "function")
        rows = {key: MatchRow(80.0, 98.0, 99.0, 0x5678, "known")}
        self.assertEqual(checkpoint_drops({key: 70.0}, {}, rows), [])

    def test_definition_extent_excludes_the_following_function(self):
        from homm3.retail_labels.source import mask_lexical_noise

        raw = ("\nvoid first() { const char *s = \"} not a brace\"; }\n"
               "void second() { return; }\n")
        definition = _definition_text(raw, mask_lexical_noise(raw), 0)
        self.assertEqual(
            definition,
            'void first() { const char *s = "} not a brace"; }')

    def test_source_hash_body_is_stable_across_annotated_redeclaration(self):
        from homm3.retail_labels.source import mask_lexical_noise

        direct = "VA(0x00401000, 4)\ninline long helper(int x) { return x; }\n"
        redeclared = ("inline long helper(int x) { return x; }\n"
                      "VA(0x00401000, 4)\nlong helper(int x);\n")
        direct_after = direct.index(")") + 1
        redeclared_after = redeclared.rindex(")") + 1
        self.assertEqual(
            _canonical_definition_text(
                direct, mask_lexical_noise(direct), direct_after,
                "?helper@@YAJH@Z"),
            _canonical_definition_text(
                redeclared, mask_lexical_noise(redeclared),
                redeclared_after, "?helper@@YAJH@Z"))

    def test_missing_legacy_zero_row_is_retired(self):
        old = {("unit", "obsolete_flat_name"): MatchRow(0.0, 0.0, 0.0)}
        rows, stats = update_rows({}, old, {})
        self.assertEqual(rows, {})
        self.assertEqual(stats["retired"], 1)

    def test_missing_positive_history_is_retained(self):
        key = ("unit", "lost")
        rows, _stats = update_rows(
            {}, {key: MatchRow(80.0, 90.0, 95.0, 0x9abc)}, {})
        self.assertEqual(rows[key], MatchRow(None, 90.0, 95.0, 0x9abc))

    def test_legacy_git_peak_seeds_history_not_enforced_max(self):
        key = ("unit", "function")
        rows, recovered = seed_historical_maxima(
            {key: MatchRow(80.0, 80.0, 80.0, 0x1234)}, {key: 100.0})
        self.assertEqual(rows[key], MatchRow(80.0, 80.0, 100.0, 0x1234))
        self.assertEqual(recovered, 1)


class BankedRowsTest(unittest.TestCase):
    """The row that leaves the file entirely - the ratchet's blind spot."""

    PATCH = ("+unit\t?banked@@YAXXZ\t83.2927\t83.2927\t83.2927\t0x428f0\n"
             "+unit\t?kept@@YAXXZ\t100.0000\t100.0000\t100.0000\t0x42880\n")

    def test_silently_dropped_row_is_fatal(self):
        history = parse_history(self.PATCH)
        current = {("unit", "?kept@@YAXXZ"):
                   MatchRow(100.0, 100.0, 100.0, 0x42880)}
        self.assertEqual([row[0] for row in missing_rows(history, current,
                                                         set())],
                         [0x428F0])

    def test_same_rva_relabel_is_not_a_loss(self):
        history = parse_history(self.PATCH)
        current = {("unit", "?kept@@YAXXZ"):
                   MatchRow(100.0, 100.0, 100.0, 0x42880),
                   ("unit", "?banked@@YBAXXZ"):  # const-qualified label
                   MatchRow(92.0, 92.0, 92.0, 0x428F0)}
        self.assertEqual(missing_rows(history, current, set()), [])

    def test_waiver_admits_a_deliberate_withdrawal(self):
        history = parse_history(self.PATCH)
        self.assertEqual(missing_rows(history, {}, {0x428F0, 0x42880}), [])

    def test_gate_negative_control_still_passes(self):
        self.assertEqual(selftest(), [])


if __name__ == "__main__":
    unittest.main()
