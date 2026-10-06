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

    def test_label_renamed_on_one_side_is_not_resurrected(self):
        # 2026-10-05: main renamed completeDraw (E -> _N mangling) while a lane
        # kept the base row untouched. The merge kept both labels for RVA
        # 0xf3f0, and the README renderer refuses a ledger that names one
        # retail body twice.
        old = "advmgr\t?completeDraw@@QAEXHHHEE@Z\t100.0000\t100.0000\t100.0000\t0xf3f0\told"
        new = "advmgr\t?completeDraw@@QAEXHHH_N0@Z\t100.0000\t100.0000\t100.0000\t0xf3f0\tnew"
        other = "unit\tf\t80.0000\t80.0000\t80.0000\t0x1\tsame"
        base = ledger(old, other)
        renamed = ledger(new, other)
        untouched = ledger(old, other.replace("80.0000\t80.0000\t80.0000", "80.0000\t90.0000\t90.0000"))
        for main, lane in ((renamed, untouched), (untouched, renamed)):
            with self.subTest(renamed_side="main" if main is renamed else "lane"):
                merged, _ = merge_baseline.merge(base, main, lane)
                rows = status.parse_baseline(merged)
                self.assertEqual([key for key, row in rows.items() if row.rva == 0xf3f0],
                                 [("advmgr", "?completeDraw@@QAEXHHH_N0@Z")])
                self.assertEqual(rows[("unit", "f")].max, 90)

    def test_rename_and_change_keep_the_new_label_and_peak(self):
        base = ledger("u\told\t80.0000\t80.0000\t80.0000\t0x5\th0")
        main = ledger("u\tnew\t80.0000\t80.0000\t80.0000\t0x5\th0")
        lane = ledger("u\told\t90.0000\t95.0000\t95.0000\t0x5\th1")
        merged, _ = merge_baseline.merge(base, main, lane)
        rows = status.parse_baseline(merged)
        self.assertEqual(list(rows), [("u", "new")])
        self.assertEqual(rows[("u", "new")].hist, 95)

    def test_two_renames_of_one_body_warn_and_keep_both(self):
        base = ledger("u\told\t80.0000\t80.0000\t80.0000\t0x5\th0")
        main = ledger("u\tmain_name\t80.0000\t80.0000\t80.0000\t0x5\th0")
        lane = ledger("u\tlane_name\t80.0000\t80.0000\t80.0000\t0x5\th0")
        warnings = []
        merged, _ = merge_baseline.merge(base, main, lane, warnings=warnings)
        self.assertEqual(set(status.parse_baseline(merged)),
                         {("u", "main_name"), ("u", "lane_name")})
        self.assertEqual(len(warnings), 1)
        self.assertIn("0x5", warnings[0])

    def test_recursive_merge_base_with_nested_conflict_markers(self):
        # Git's recursive strategy writes a virtual base whose conflicts with
        # its own merge bases use lengthened markers. `_parts` used to reject
        # them as "malformed match baseline row: '<<<<<<<<< Temporary ...'".
        base = (status.BASELINE_HEADER
                + "unit\tone\t80.0000\t80.0000\t80.0000\t0x1\tsame\n"
                + "<<<<<<<<< Temporary merge branch 1\n"
                + "unit\ttwo\t80.0000\t90.0000\t90.0000\t0x2\tfirst\n"
                + "<<<<<<<<<<< Temporary merge branch 1\n"
                + "unit\tthree\t10.0000\t10.0000\t10.0000\t0x3\tfirst\n"
                + "===========\n"
                + "unit\tthree\t20.0000\t20.0000\t20.0000\t0x3\tsecond\n"
                + ">>>>>>>>>>> Temporary merge branch 2\n"
                + "||||||||| merged common ancestors\n"
                + "unit\ttwo\t70.0000\t70.0000\t70.0000\t0x2\tancestor\n"
                + "=========\n"
                + "unit\ttwo\t85.0000\t95.0000\t95.0000\t0x2\tsecond\n"
                + ">>>>>>>>> Temporary merge branch 2\n")
        variants = [status.parse_baseline(text)
                    for text in merge_baseline.base_variants(base)]
        self.assertEqual([v[("unit", "two")].src_hash for v in variants],
                         ["first", "second"])
        # The nested hunk sits inside branch 1 and resolves to its own side.
        self.assertEqual(variants[0][("unit", "three")].src_hash, "first")
        self.assertNotIn(("unit", "three"), variants[1])
        # main kept virtual-base rows, so the lane's earned rows win; the
        # lane left `one` alone, so main's change survives.
        main = ledger("unit\tone\t85.0000\t85.0000\t85.0000\t0x1\tmain",
                      "unit\ttwo\t85.0000\t95.0000\t95.0000\t0x2\tsecond",
                      "unit\tthree\t10.0000\t10.0000\t10.0000\t0x3\tfirst")
        lane = ledger("unit\tone\t80.0000\t80.0000\t80.0000\t0x1\tsame",
                      "unit\ttwo\t99.0000\t99.0000\t99.0000\t0x2\tlane",
                      "unit\tthree\t30.0000\t30.0000\t30.0000\t0x3\tlane")
        merged, taken = merge_baseline.merge(base, main, lane)
        rows = status.parse_baseline(merged)
        self.assertEqual(taken, 2)
        self.assertEqual(rows[("unit", "one")].src_hash, "main")
        self.assertEqual(rows[("unit", "two")].src_hash, "lane")
        self.assertEqual(rows[("unit", "three")].src_hash, "lane")

    def test_unterminated_conflict_hunk_is_reported(self):
        with self.assertRaisesRegex(ValueError, "unterminated conflict"):
            merge_baseline.base_variants("<<<<<<< ours\nrow\n=======\n")


if __name__ == "__main__":
    unittest.main()
