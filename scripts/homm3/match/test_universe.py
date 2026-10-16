import unittest
from types import SimpleNamespace
from unittest.mock import patch

from homm3.match import universe, status


class FunctionUniverseTest(unittest.TestCase):
    def test_reviewed_cleanup_is_excluded_from_game_denominator(self):
        rows = {universe.FUNCTIONS: [['0x100', '10']],
                universe.FUNCLETS: [], universe.RUNTIME_MAP: [],
                universe.ZLIB_MAP: [], universe.INIT_THUNKS: [['0x100', '-']]}
        image = SimpleNamespace(data=bytes(256), image_base=0x400000,
                                sections=[SimpleNamespace(name='.text', rva=0x100)],
                                blob=lambda _: bytes(10))
        with patch.object(universe, '_rows', side_effect=lambda path: rows[path]):
            categories, sizes = universe.classify(image)
        self.assertEqual(categories, {0x100: 'init-thunk'})
        self.assertEqual(sizes, {0x100: 10})

    def test_zlib_data_does_not_enter_function_denominator(self):
        for suffix in ([], ['func']):
            with self.subTest(schema=suffix):
                rows = {universe.FUNCTIONS: [['0x100', '1']],
                        universe.FUNCLETS: [], universe.RUNTIME_MAP: [],
                        universe.INIT_THUNKS: [],
                        universe.ZLIB_MAP: [['0x100', '1', '_crc32', 'crc32', *suffix],
                                           ['0x200', '1024', '_crc_table', 'crc32', 'data']]}
                image = SimpleNamespace(data=bytes(256), image_base=0x400000,
                                        sections=[SimpleNamespace(name='.text', rva=0x100)],
                                        blob=lambda _: b'\xc3')
                with patch.object(universe, '_rows', side_effect=lambda path: rows[path]):
                    categories, sizes = universe.classify(image)
                self.assertEqual(categories, {0x100: 'zlib'})
                self.assertEqual(sizes, {0x100: 1})


def contribution(**updates):
    row = dict(rva='0x100', size='100', kind='code', library='LIBCMT.LIB',
               member='asm.obj', section='1', symbol='_entry', ownership='whole-section')
    row.update(updates)
    return row


class RuntimeContributionOwnershipTest(unittest.TestCase):
    def owned(self, functions, *rows):
        return universe.contribution_runtime_bodies(functions, rows)

    def test_complete_internal_body_needs_no_invented_symbol(self):
        self.assertEqual(self.owned({0x100: 20, 0x114: 80}, contribution()),
                         {0x100, 0x114})

    def test_partial_overlap_and_adjacent_pieces_do_not_prove_ownership(self):
        self.assertEqual(self.owned({0xff: 2, 0x163: 2, 0x164: 1}, contribution()), set())
        self.assertEqual(self.owned({0x100: 10}, contribution(size='5'),
                                   contribution(rva='0x105', size='5', section='2')), set())

    def test_only_pinned_code_sections_are_owners(self):
        for change in ({'kind': 'alias'}, {'kind': 'data'}, {'kind': 'bss'},
                       {'kind': 'common'}, {'kind': 'thunk'}, {'library': 'zlib'},
                       {'library': 'project.lib'}, {'section': '0'}, {'size': '0'},
                       {'section': '-1'}, {'member': ''}, {'symbol': ''},
                       {'member': '-'}, {'symbol': '-'},
                       {'ownership': '-'}, {'ownership': 'prefix'}):
            with self.subTest(change=change):
                self.assertEqual(self.owned({0x110: 20}, contribution(**change)), set())

    def test_legacy_prefix_rows_supply_no_interior_ownership(self):
        legacy = contribution()
        del legacy['ownership']
        self.assertEqual(self.owned({0x110: 20}, legacy), set())

    def test_conflicting_overlapping_owners_are_not_guessed(self):
        self.assertEqual(self.owned({0x110: 20}, contribution(),
                                   contribution(rva='0x120', member='different.obj')), set())
        self.assertEqual(self.owned({0x110: 20}, contribution(),
                                   contribution(member='different.obj')), set())
        self.assertEqual(self.owned({0x110: 20}, contribution(), contribution()), {0x110})

    def test_other_categories_keep_their_specific_ownership(self):
        rows = {universe.FUNCTIONS: [['0x100', '10'], ['0x110', '10'],
                                    ['0x120', '10'], ['0x130', '10']],
                universe.FUNCLETS: [['0x100']], universe.RUNTIME_MAP: [],
                universe.ZLIB_MAP: [['0x110', '10', '_z', 'zlib', 'func']],
                universe.INIT_THUNKS: [['0x120', '-']]}
        image = SimpleNamespace(data=bytes(256), image_base=0x400000,
                                sections=[SimpleNamespace(name='.text', rva=0x100)],
                                blob=lambda _: bytes(100))
        with patch.object(universe, '_rows', side_effect=lambda path: rows[path]), \
                patch.object(universe, '_runtime_contributions', return_value=[contribution()]):
            categories, _ = universe.classify(image)
        self.assertEqual(categories, {0x100: 'eh-funclet', 0x110: 'zlib',
                                      0x120: 'init-thunk', 0x130: 'runtime'})

    def test_missing_reviewed_inventory_supplies_no_coverage(self):
        with patch.object(universe, 'RUNTIME_CONTRIBUTIONS',
                          SimpleNamespace(is_file=lambda: False)):
            self.assertEqual(universe._runtime_contributions(), [])

    def test_status_targets_use_the_same_runtime_classification(self):
        categories = {1: 'runtime', 2: 'target', 3: 'eh-funclet',
                      4: 'zlib', 5: 'init-thunk', 6: 'import-thunk'}
        with patch.object(status, 'CENSUS_FUNCTIONS', SimpleNamespace(is_file=lambda: True)), \
                patch.object(universe, 'classify', return_value=(categories, {})):
            self.assertEqual(status.census_targets(), {2, 3, 4, 5, 6})


if __name__ == '__main__':
    unittest.main()
