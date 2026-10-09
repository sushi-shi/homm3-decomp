"""placements.fold_label: which name labels an address several bodies reach."""
import unittest

from homm3.census.placements import fold_label


class FoldLabelTests(unittest.TestCase):
    def setUp(self):
        self.definers = {name: ['textresource'] for name in ('dtorPAD', 'dtorH', 'dtorPAV', 'a', 'b')}

    def test_the_sole_single_address_name_labels_a_mixed_address(self):
        # ~vector<char*> and ~vector<int> also reach the editor's copy
        # elsewhere; only ~vector<vector<char*>*> reaches just this one
        names = {'dtorPAD': {0x10, 0x20}, 'dtorH': {0x10, 0x20}, 'dtorPAV': {0x10}}
        self.assertEqual(fold_label({'dtorPAD', 'dtorH', 'dtorPAV'}, names, {}, self.definers,
                                    set()), 'dtorPAV')

    def test_a_lone_name_is_no_fold(self):
        self.assertIsNone(fold_label({'dtorPAV'}, {'dtorPAV': {0x10}}, {}, self.definers, set()))

    def test_identical_bodies_fold_and_different_ones_do_not(self):
        names = {'a': {0x10}, 'b': {0x10}}
        same = {'a': (b'\x8b\x01\xc3', {}), 'b': (b'\x8b\x01\xc3', {})}
        self.assertEqual(fold_label({'a', 'b'}, names, same, self.definers, set()), 'a')
        other = {'a': (b'\x8b\x01\xc3', {}), 'b': (b'\x8b\x02\xc3', {})}
        self.assertIsNone(fold_label({'a', 'b'}, names, other, self.definers, set()))


if __name__ == '__main__':
    unittest.main()
