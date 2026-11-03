"""placements.fold_label: which name labels an address several bodies reach."""
import unittest

from homm3.census.placements import fold_label, foreign_to_claim, relocation_identity, sole_choice


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

    def test_an_own_comdat_labels_before_an_own_ordinary_function(self):
        # TQuestDefeatMonster::accept (Quest.cpp) folds onto the inline
        # TVCCaptureTown::accept; only the inline one can be a table row
        names = {'?accept@TQuestDefeatMonster': {0x10}, '?accept@TVCCaptureTown': {0x10}}
        body = (b'\x8b\x44\x24\x04\xc3', {})
        bodies = dict.fromkeys(names, body)
        definers = {'?accept@TQuestDefeatMonster': ['Quest'], '?accept@TVCCaptureTown': ['GameMap']}
        owned = {'Quest', 'GameMap'}
        self.assertEqual(fold_label(set(names), names, bodies, definers, owned),
                         '?accept@TQuestDefeatMonster')
        self.assertEqual(fold_label(set(names), names, bodies, definers, owned,
                                    lambda name: name == '?accept@TVCCaptureTown'),
                         '?accept@TVCCaptureTown')

    def test_same_array_different_fields_are_not_an_icf_fold(self):
        names = {'a': {0x10}, 'b': {0x10}}
        relocs = {1: ('contexts', 6)}
        bodies = {'a': (b'\xa1\x00\x00\x00\x00\xc3', relocs),
                  'b': (b'\xa1\x08\x00\x00\x00\xc3', relocs)}
        self.assertNotEqual(relocation_identity(*bodies['a']),
                            relocation_identity(*bodies['b']))
        self.assertIsNone(fold_label(set(names), names, bodies, self.definers, set()))
        self.assertTrue(foreign_to_claim('a', {'b'}, bodies))

    def test_different_referents_make_masked_admission_ambiguous(self):
        body = b'\xa1\x00\x00\x00\x00\xc3'
        self.assertNotEqual(relocation_identity(body, {1: ('sprites', 6)}),
                            relocation_identity(body, {1: ('bitmaps', 6)}))

    def test_equal_referents_and_addends_can_fold(self):
        names = {'a': {0x10}, 'b': {0x10}}
        body = (b'\xa1\x08\x00\x00\x00\xc3', {1: ('contexts', 6)})
        bodies = dict.fromkeys(names, body)
        self.assertEqual(fold_label(set(names), names, bodies, self.definers, set()), 'a')

    def test_calls_to_proven_folded_destructors_can_fold(self):
        names = {'a': {0x10}, 'b': {0x10}, 'dtorA': {0x20}, 'dtorB': {0x20}}
        body = b'\xe8\x00\x00\x00\x00\xc3'
        bodies = {'a': (body, {1: ('dtorA', 20)}),
                  'b': (body, {1: ('dtorB', 20)})}
        self.assertEqual(fold_label({'a', 'b'}, names, bodies, self.definers, set()), 'a')


class SoleChoiceTests(unittest.TestCase):
    def test_one_name_from_several_units_is_one_choice(self):
        choices = [('?erase@x', 'masked prefix (Quest)'), ('?erase@x', 'masked prefix (QuestLocation)')]
        self.assertEqual(sole_choice(choices), choices[0])

    def test_two_names_at_one_address_are_no_choice(self):
        self.assertIsNone(sole_choice([('?a@x', 'one'), ('?b@x', 'two')]))


if __name__ == '__main__':
    unittest.main()
