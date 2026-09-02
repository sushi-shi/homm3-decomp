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


class AdmissionRouting(unittest.TestCase):

    def test_default_queue_excludes_every_admitted_source_body(self):
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
