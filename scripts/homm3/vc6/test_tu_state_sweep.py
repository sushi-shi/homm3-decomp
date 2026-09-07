from __future__ import annotations

import unittest

from homm3.match.status import MatchRow
from homm3.vc6.tu_state_sweep import (
    affected_by_unit, bank_rows, insertion_for, make_variants,
)


class TuStateSweepTests(unittest.TestCase):
    def test_groups_every_numeric_cur_below_hist(self):
        rows = {
            ("a", "low"): MatchRow(80, 90, 100),
            ("a", "equal"): MatchRow(100, 100, 100),
            ("b", "missing"): MatchRow(None, 80, 100),
            ("b", "low"): MatchRow(70, 70, 71),
        }
        self.assertEqual(affected_by_unit(rows), {
            "a": (("a", "low"),),
            "b": (("b", "low"),),
        })

    def test_one_insertion_precedes_earliest_affected_marker(self):
        text = ("#include <x>\n\nint before;\n\n// first evidence\n"
                "VA(0x00400100, 4)\nvoid first() {}\n\n"
                "// second evidence\nVA(0x00400200, 4)\nvoid second() {}\n")
        offset, line = insertion_for(text, (0x200, 0x100))
        self.assertTrue(text[offset:].startswith("\n// first evidence\n"))
        self.assertEqual(line, 4)

    def test_forest_sequence_is_deterministic(self):
        left = make_variants(30, 20260906)
        right = make_variants(30, 20260906)
        self.assertEqual(left, right)
        self.assertEqual(left[1].tag, "0135282a-0002-ff93bdf3")
        self.assertEqual(len(left), 30)

    def test_bank_keeps_cur_and_raises_hist_only_for_new_peak(self):
        hashed = ("u", "hashed")
        no_hash = ("u", "generated")
        rows = {
            hashed: MatchRow(70, 80, 90, 0x100, "abc"),
            no_hash: MatchRow(20, 20, 25, 0x200, None),
        }
        updated, changes = bank_rows(
            rows, {hashed: 85, no_hash: 30}, {hashed: "abc"})
        self.assertEqual(updated[hashed], MatchRow(70, 85, 90, 0x100, "abc"))
        self.assertEqual(updated[no_hash], MatchRow(20, 30, 30, 0x200, None))
        self.assertEqual(len(changes), 2)

    def test_hash_mismatch_refuses_bank(self):
        key = ("u", "fn")
        row = MatchRow(70, 80, 100, 0x100, "old")
        updated, changes = bank_rows({key: row}, {key: 100}, {key: "new"})
        self.assertEqual(updated[key], row)
        self.assertEqual(changes, [])


if __name__ == "__main__":
    unittest.main()
