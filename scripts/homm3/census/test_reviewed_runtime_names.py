"""Reviewed folds retain identities without excluding game-owned bodies."""
import unittest
from homm3.census.libraries import reviewed_runtime_names


def row(name, kind='code', rva='0x100', **updates):
    value = dict(rva=rva, size='20', kind=kind, library='LIBCPMT.LIB',
                 member=name + '.obj', section='1', symbol=name)
    value.update(updates)
    return value


def label(name, rva=0x100):
    return (rva, name, 'LIBCPMT', name + '.obj')


class ReviewedRuntimeNamesTest(unittest.TestCase):
    def test_reviewed_owner_and_existing_code_fold_behavior(self):
        self.assertEqual(reviewed_runtime_names({0x100}, [], [row('narrow'), row('wide')]),
                         ([label('narrow')], [label('wide')]))

    def test_alias_before_owner_is_order_independent(self):
        owner, alias = row('narrow'), row('wide', 'alias')
        for rows in ([alias, owner], [owner, alias]):
            self.assertEqual(reviewed_runtime_names({0x100}, [], rows),
                             ([label('narrow')], [label('wide')]))

    def test_reviewed_iterator_retains_both_passes(self):
        rows = iter([row('wide', 'alias'), row('narrow')])
        self.assertEqual(reviewed_runtime_names({0x100}, [], rows),
                         ([label('narrow')], [label('wide')]))

    def test_alias_cannot_create_runtime_ownership(self):
        self.assertEqual(reviewed_runtime_names({0x100}, [], [row('wide', 'alias')]), ([], []))

    def test_alias_into_game_code_is_not_a_runtime_name(self):
        existing = [label('runtime', 0x200)]
        self.assertEqual(reviewed_runtime_names({0x100, 0x200}, existing,
                                                [row('wide', 'alias')]), (existing, []))

    def test_census_entry_is_required(self):
        self.assertEqual(reviewed_runtime_names(set(), [], [row('narrow'), row('wide', 'alias')]),
                         ([], []))

    def test_existing_primary_and_alias_are_preserved_without_duplicates(self):
        self.assertEqual(reviewed_runtime_names({0x100}, [label('narrow')],
                                                [row('wide', 'alias'), row('wide', 'alias')],
                                                [label('wide')]),
                         ([label('narrow')], [label('wide')]))

    def test_equal_primary_name_is_not_its_own_alias(self):
        self.assertEqual(reviewed_runtime_names({0x100}, [label('narrow')],
                                                [row('narrow', 'alias')]),
                         ([label('narrow')], []))

    def test_noncode_and_invalid_alias_sections_supply_no_name(self):
        for entry in (row('other', 'data'), row('other', 'bss'),
                      row('other', 'alias', size='0'), row('other', 'alias', section='0')):
            with self.subTest(row=entry):
                self.assertEqual(reviewed_runtime_names({0x100}, [label('narrow')], [entry]),
                                 ([label('narrow')], []))


if __name__ == '__main__':
    unittest.main()
