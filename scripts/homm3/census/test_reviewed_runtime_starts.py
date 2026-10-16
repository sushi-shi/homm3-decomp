"""Verified whole-library anchors can recover otherwise missed native entries."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.census import functions
from homm3.census.libraries import reviewed_runtime_anchors
from homm3.census.test_initializer_tails import Image
from homm3.census.test_reviewed_runtime_exports import fixture


class ReviewedRuntimeAnchorsTest(unittest.TestCase):
    def anchors(self, **changes):
        obj, image, _mapped, row = fixture()
        row.update(changes)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'library.lib'
            path.touch()
            with patch('homm3.verify.library_code.archive', return_value=({'asm.obj': obj}, {})):
                return reviewed_runtime_anchors(image, [row], {'LIBCMT': path})

    def test_only_the_declared_zero_offset_function_is_an_anchor(self):
        self.assertEqual(self.anchors(), {0x1000: 'LIBCMT:asm.obj#1:_first'})

    def test_alias_partial_legacy_and_non_anchor_rows_supply_no_start(self):
        for changes in ({'kind': 'alias'}, {'ownership': '-'}, {'size': '3'},
                        {'symbol': '_second'}, {'section': '2', 'symbol': 'datum'}):
            with self.subTest(changes=changes):
                self.assertEqual(self.anchors(**changes), {})


class ReviewedCensusStartsTest(unittest.TestCase):
    def run_census(self, starts=None):
        # One entry zeroes EAX, another independently verified entry shares
        # its return. No native pointer or call identifies the second entry.
        return functions.run(Image(bytes.fromhex('31c0c3')), log=lambda _: None,
                             reviewed_starts=starts)

    def test_without_reviewed_evidence_the_existing_descent_is_unchanged(self):
        census = self.run_census()
        self.assertEqual(set(census.starts), {0x1000})
        self.assertEqual({r: n for r, n, _ in functions.partition(census)}, {0x1000: 3})

    def test_reviewed_entry_refreshes_its_predecessors_extent(self):
        census = self.run_census({0x1002: 'LIB:member#section:entry'})
        self.assertEqual(set(census.starts), {0x1000, 0x1002})
        self.assertEqual(census.starts[0x1002], 'reviewed-library:LIB:member#section:entry')
        self.assertEqual({r: n for r, n, _ in functions.partition(census)},
                         {0x1000: 2, 0x1002: 1})
        self.assertNotIn(0x1002, census.reached[0x1000])

    def test_existing_entry_keeps_its_original_evidence(self):
        census = self.run_census({0x1000: 'already known'})
        self.assertEqual(census.starts, {0x1000: 'entry'})

    def test_outside_text_or_undecodable_entries_are_not_forced(self):
        census = self.run_census({0x2000: 'outside', 0x1001: 'truncated instruction'})
        self.assertEqual(set(census.starts), {0x1000})


if __name__ == '__main__':
    unittest.main()
