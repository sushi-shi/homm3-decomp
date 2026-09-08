import unittest

from homm3.retail_labels.headers import choose_carrier


class HeaderCarrierTest(unittest.TestCase):
    def test_emitter_preferred_over_missing_banked_carrier(self):
        self.assertEqual(choose_carrier(20, {'a', 'b'}, {20: 'b'}, []), 'b')
        self.assertEqual(choose_carrier(20, {'a'}, {20: 'b'}, []), 'a')

    def test_missing_body_preserves_existing_comparison_but_cannot_admit_claim(self):
        self.assertEqual(choose_carrier(20, set(), {20: 'a'}, []), 'a')
        self.assertIsNone(choose_carrier(20, set(), {}, [(10, 'a'), (30, 'a')]))

    def test_existing_carrier_survives_ambiguous_replacement_emitters(self):
        self.assertEqual(choose_carrier(20, {'b', 'c'}, {20: 'a'},
                                        [(10, 'a'), (30, 'a')]), 'a')
        self.assertIsNone(choose_carrier(20, {'b', 'c'}, {},
                                         [(10, 'a'), (30, 'a')]))

    def test_unique_retail_neighbour_selects_new_header_claim(self):
        self.assertEqual(choose_carrier(20, {'a', 'b'}, {}, [(10, 'b'), (30, 'b')]), 'b')

    def test_ambiguous_boundary_is_not_guessed(self):
        self.assertIsNone(choose_carrier(20, {'a', 'b'}, {}, [(10, 'a'), (30, 'b')]))


class InlineSourceAnnotationTest(unittest.TestCase):
    def test_banked_projection_requires_active_source_identity(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        from homm3.core import common
        from homm3.retail_labels.headers import project

        path = common.HOMM3_DIR / 'include/example.h'
        definition = SimpleNamespace(file='include/example.h', va=0x401000,
                                     mangled='?renamed@Example@@QAEXXZ')
        for definitions in ([definition], []):
            with self.subTest(active=bool(definitions)), \
                 patch('homm3.match.source_ownership.collect', return_value=(definitions, [], [])), \
                 patch('homm3.match.status.load_baseline', return_value={
                     ('example', 'oldName'): SimpleNamespace(rva=0x1000)}), \
                 patch('homm3.retail_labels.source._base_authority_names', return_value={}), \
                 patch('homm3.retail_labels.source.scan_file', return_value=[{
                     'channel': 'src-VA', 'rva': 0x1000, 'size': 16, 'kind': 'func'}]):
                rows, problems = {'example': []}, []
                project([path], {0x1000}, {'example': {}}, rows, problems)
                if definitions:
                    self.assertEqual(rows['example'][0]['joined'], definition.mangled)
                    self.assertEqual(rows['example'][0]['unit'], 'example')
                    self.assertFalse(any('(FATAL)' in p for p in problems))
                    self.assertTrue(any('missing body' in p for p in problems))
                else:
                    self.assertEqual(rows['example'], [])
                    self.assertTrue(any('(FATAL)' in p for p in problems))

    def test_missing_ir_annotation_uses_ast_and_conflict_is_fatal(self):
        from types import SimpleNamespace
        from homm3.core import common
        from homm3.retail_labels.source import ast_names
        definition = SimpleNamespace(file='src/lobby.cpp', va=0x401000,
                                     mangled='??0Reply@@QAE@HH@Z')
        problems = []
        names = ast_names(common.HOMM3_DIR / 'src/lobby.cpp', [definition], {}, problems)
        self.assertEqual(names, {0x1000: definition.mangled})
        self.assertEqual(problems, [])
        ast_names(common.HOMM3_DIR / 'src/lobby.cpp', [definition], {0x1000: 'wrong'}, problems)
        self.assertTrue(any('(FATAL)' in p for p in problems))


if __name__ == '__main__':
    unittest.main()
