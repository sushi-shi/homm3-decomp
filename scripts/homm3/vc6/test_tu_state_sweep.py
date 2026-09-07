from __future__ import annotations

import unittest

from homm3.match.status import MatchRow
from homm3.vc6.tu_state_sweep import (
    _initial_include_insertion, _project_header_pool, affected_by_unit,
    bank_rows, insertion_for, insert_variant, make_variants,
)


class TuStateSweepTests(unittest.TestCase):
    def test_groups_every_numeric_max_below_hist(self):
        rows = {
            ("a", "low"): MatchRow(80, 90, 100),
            ("a", "already-banked"): MatchRow(80, 100, 100),
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

    def test_trial_inserts_one_include_block_for_the_whole_tu(self):
        text = ("#include <x>\n#include \"already.h\"\n\nint before;\n\n// first evidence\n"
                "VA(0x00400100, 4)\nvoid first() {}\n\n"
                "// second evidence\nVA(0x00400200, 4)\nvoid second() {}\n")
        offset, line = _initial_include_insertion(text)
        variant = make_variants(
            1, 20260906, "test", tuple(f"new{i}.h" for i in range(12)))[0]
        candidate = insert_variant(text, ((offset, line, 0),), variant)
        self.assertEqual(candidate.count("#line"), 1)
        self.assertEqual(sum(candidate.count(f'#include "new{i}.h"')
                             for i in range(12)),
                         len(variant.body.splitlines()))
        self.assertLess(candidate.index(variant.body), candidate.index("int before"))
        self.assertGreater(candidate.index(variant.body),
                           candidate.index('#include "already.h"'))

    def test_include_sequence_is_deterministic_and_has_five_to_ten_headers(self):
        headers = tuple(f"header{i}.h" for i in range(20))
        left = make_variants(30, 20260906, "unit", headers)
        right = make_variants(30, 20260906, "unit", headers)
        self.assertEqual(left, right)
        self.assertEqual(len(left), 30)
        self.assertTrue(all(5 <= len(item.body.splitlines()) <= 10
                            for item in left))
        self.assertTrue(all(len(set(item.body.splitlines())) ==
                            len(item.body.splitlines()) for item in left))

    def test_tu_specific_invalid_header_pairings_are_excluded(self):
        self.assertNotIn(
            "kbwin.h", _project_header_pool('#include "diff.h"\n', "diff"))
        self.assertNotIn(
            "resourcemanager_cache_result.h",
            _project_header_pool(
                '#include "singleselectionwindow_priv.h"\n',
                "singleselectionwindow"))
        self.assertNotIn(
            "autostrptr.h",
            _project_header_pool('#include "spells.h"\n', "spells"))

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
