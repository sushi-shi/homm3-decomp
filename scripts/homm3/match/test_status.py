#!/usr/bin/env python3
"""Hermetic tests for cur/max/hist baseline evolution."""
from __future__ import annotations

import unittest

from homm3.match.status import MatchRow, seed_historical_maxima, update_rows


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

    def test_accept_lowers_max_but_never_history(self):
        key = ("unit", "function")
        old = {key: MatchRow(98.0, 98.0, 99.0, 0x5678)}
        rows, stats = update_rows({key: 80.0}, old, {key: 0x5678}, True)
        self.assertEqual((rows[key].cur, rows[key].max, rows[key].hist),
                         (80.0, 80.0, 99.0))
        self.assertEqual(stats["lowered"], 1)

    def test_unaccepted_drop_keeps_max(self):
        key = ("unit", "function")
        old = {key: MatchRow(98.0, 98.0, 98.0, 0x5678)}
        rows, _stats = update_rows({key: 80.0}, old, {key: 0x5678})
        self.assertEqual((rows[key].cur, rows[key].max, rows[key].hist),
                         (80.0, 98.0, 98.0))

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


if __name__ == "__main__":
    unittest.main()
