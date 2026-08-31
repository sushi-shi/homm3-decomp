#!/usr/bin/env python3
"""Negative controls for banked-MAX queue routing.

Run with ``python3 -m homm3.vc6.test_queue``.
"""
from __future__ import annotations

import unittest

from homm3.vc6 import queue


def _report(*functions):
    return {"units": [{"name": "unit", "functions": list(functions)}]}


class BankedMaxRouting(unittest.TestCase):

    def test_banked_exact_current_dip_is_not_actionable(self):
        # NEGATIVE CONTROL: this is the defect that reopened a source-correct
        # function solely because its current score dipped below its MAX.
        fn = {"name": "banked", "size": 100,
              "fuzzy_match_percent": 99.9983}
        targets, dips, undiffable = queue._partition_targets(
            _report(fn), {("unit", "banked"): 100.0})
        self.assertEqual(targets, [])
        self.assertEqual(len(dips), 1)
        self.assertEqual(undiffable, [])

    def test_effective_max_uses_banked_peak(self):
        fn = {"name": "dipped", "size": 100,
              "fuzzy_match_percent": 90.0}
        targets, _dips, _undiffable = queue._partition_targets(
            _report(fn), {("unit", "dipped"): 95.0})
        self.assertEqual(targets[0][2], 95.0)

    def test_new_current_improvement_wins_until_banked(self):
        fn = {"name": "improved", "size": 100,
              "fuzzy_match_percent": 96.0}
        targets, _dips, _undiffable = queue._partition_targets(
            _report(fn), {("unit", "improved"): 95.0})
        self.assertEqual(targets[0][2], 96.0)

    def test_hardest_effective_max_sorts_first(self):
        functions = (
            {"name": "easy", "size": 100, "fuzzy_match_percent": 90.0},
            {"name": "hard", "size": 10, "fuzzy_match_percent": 20.0},
            {"name": "unclaimed", "size": 5},
        )
        targets, _dips, _undiffable = queue._partition_targets(
            _report(*functions), {})
        self.assertEqual([row[1] for row in targets],
                         ["unclaimed", "hard", "easy"])

    def test_mangled_scoreless_row_remains_undiffable(self):
        fn = {"name": "?claimed@@YAXXZ", "size": 12}
        targets, dips, undiffable = queue._partition_targets(
            _report(fn), {})
        self.assertEqual(targets, [])
        self.assertEqual(dips, [])
        self.assertEqual(undiffable, [("unit", "?claimed@@YAXXZ", 12)])


if __name__ == "__main__":
    unittest.main()
