"""Concurrent score lanes preserve earned rows and all-time peaks."""
import unittest

from homm3.match import merge_baseline, status


def ledger(*rows):
    return status.BASELINE_HEADER + "\n".join(rows) + "\n"


class MergeBaselineTest(unittest.TestCase):
    def test_takes_only_earned_lane_rows_ignoring_cur_drift(self):
        base = ledger("unit\tone\t80.0000\t90.0000\t95.0000\t0x1\told",
                      "unit\ttwo\t80.0000\t90.0000\t95.0000\t0x2\told")
        main = ledger("unit\tone\t85.0000\t95.0000\t95.0000\t0x1\tmain",
                      "unit\ttwo\t82.0000\t90.0000\t95.0000\t0x2\told")
        lane = ledger("unit\tone\t70.0000\t90.0000\t95.0000\t0x1\told",
                      "# lane evidence",
                      "unit\ttwo\t100.0000\t100.0000\t100.0000\t0x2\tlane")
        merged, taken = merge_baseline.merge(base, main, lane)
        rows = status.parse_baseline(merged)
        self.assertEqual(taken, 1)
        self.assertEqual(rows[("unit", "one")].max, 95)
        self.assertEqual(rows[("unit", "one")].src_hash, "main")
        self.assertEqual(rows[("unit", "two")].max, 100)
        self.assertIn("# lane evidence\nunit\ttwo", merged)

    def test_both_earned_keep_higher_max_and_hist(self):
        base = ledger("unit\tf\t80.0000\t80.0000\t80.0000\t0x1\told")
        main = ledger("unit\tf\t90.0000\t95.0000\t99.0000\t0x1\tmain")
        lane = ledger("unit\tf\t85.0000\t96.0000\t96.0000\t0x1\tlane")
        merged, _ = merge_baseline.merge(base, main, lane)
        row = status.parse_baseline(merged)[("unit", "f")]
        self.assertEqual((row.cur, row.max, row.hist, row.src_hash),
                         (85, 96, 99, "lane"))

    def test_conflicting_retail_body_needs_review(self):
        base = ledger("unit\tf\t80.0000\t80.0000\t80.0000\t0x1\told")
        main = ledger("unit\tf\t90.0000\t90.0000\t90.0000\t0x1\tmain")
        lane = ledger("unit\tf\t95.0000\t95.0000\t95.0000\t0x2\tlane")
        with self.assertRaisesRegex(ValueError, "conflicting retail RVA"):
            merge_baseline.merge(base, main, lane)


if __name__ == "__main__":
    unittest.main()
