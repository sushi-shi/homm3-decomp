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

    def test_multiple_header_instances_select_their_own_carriers(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        from homm3.core import common
        from homm3.match.source_ownership import Definition
        from homm3.retail_labels.headers import project
        definition = Definition('include/bits.h', 8, 0, 100, 'Bits::get',
                                'int () const', 0, True, True, 0x401000,
                                'bits144', instance='Bits<144>::get',
                                additional_instances=((0x402000, 'Bits<145>::get', 'bits145'),))
        claims = [{'channel': 'src-VA', 'rva': rva, 'size': 3, 'kind': 'func'}
                  for rva in (0x1000, 0x2000)]
        with patch('homm3.match.source_ownership.collect', return_value=([definition], [], [])), \
             patch('homm3.match.status.load_baseline', return_value={}), \
             patch('homm3.retail_labels.source._base_authority_names',
                   side_effect=lambda unit: {unit: [(('bits144' if unit == 'a' else 'bits145'), 3)]}), \
             patch('homm3.retail_labels.source.scan_file', return_value=claims):
            rows, problems = {'a': [], 'b': []}, []
            project([common.HOMM3_DIR / 'include/bits.h'], {0x1000, 0x2000},
                    {'a': {}, 'b': {}}, rows, problems)
        self.assertEqual(problems, [])
        self.assertEqual([(r['rva'], r['joined']) for r in rows['a']], [(0x1000, 'bits144')])
        self.assertEqual([(r['rva'], r['joined']) for r in rows['b']], [(0x2000, 'bits145')])

    def test_annotation_run_reaches_one_generic_body(self):
        import tempfile
        from pathlib import Path
        from homm3.retail_labels import source
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'bits.h'
            path.write_text('template<int N>\n' + ''.join(
                f'// VA instance: Bits<{n}>::operator*\nVA(0x{va:08x}, 3)\n'
                for n, va in ((144, 0x401000), (145, 0x402000), (146, 0x403000))) +
                'int Bits<N>::operator*() const { return N; }\n')
            rows = source.scan_file(path, {0x1000, 0x2000, 0x3000})
        self.assertEqual([r['rva'] for r in rows], [0x1000, 0x2000, 0x3000])
        self.assertEqual(len({r['name'] for r in rows}), 1)

    def test_written_iterator_cannot_use_a_generated_enrollment(self):
        import contextlib
        import io
        import tempfile
        from pathlib import Path
        from homm3.retail_labels import source
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'iterator.cpp'
            path.write_text('VA_COMPGEN(0x00401000, 0x14, BITSET_ITERATOR_DEREF, Bitset144)\n')
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr), self.assertRaises(SystemExit):
                source.scan_file(path, {0x1000})
            self.assertIn('generic header definition', stderr.getvalue())

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


class LocalInlineIdentityTest(unittest.TestCase):
    def test_only_active_banked_inline_in_owning_source_gets_fallback(self):
        from dataclasses import replace
        from unittest.mock import patch
        from homm3.core import common
        from homm3.match.source_ownership import Definition
        from homm3.retail_labels.source import banked_inline_names, ir_bind
        d = Definition('src/example.cpp', 8, 0, 100, 'Example::run',
                       'void ()', 0, True, True, 0x401000,
                       '?run@Example@@QAEXXZ')
        path = common.HOMM3_DIR / d.file
        for definitions, banked, accepted in (
                ([d], {('example', 0x1000)}, True),
                ([], {('example', 0x1000)}, False),
                ([replace(d, inline=False)], {('example', 0x1000)}, False),
                ([replace(d, file='src/other.cpp')], {('example', 0x1000)}, False),
                ([d], {('other', 0x1000)}, False),
                ([d], {('example', 0x2000)}, False),
                ([d], set(), False)):
            with self.subTest(definitions=definitions, banked=banked), \
                 patch('homm3.retail_labels.source._base_authority_names',
                       return_value={'other': [('?other@@YIXXZ', 8)]}):
                fallback = banked_inline_names(path, definitions, banked)
                rows, problems = [dict(channel='src-VA', rva=0x1000, size=8)], []
                taken = ir_bind('example', rows, {0x1000: d.mangled}, problems, fallback)
                if accepted:
                    self.assertEqual(rows[0]['joined'], d.mangled)
                    self.assertEqual(taken, {d.mangled})
                    self.assertTrue(any('missing body' in p for p in problems))
                else:
                    self.assertNotIn('joined', rows[0])
                    self.assertTrue(rows[0]['ir_unconfirmed'])
                    self.assertEqual(taken, set())


class AnonymousNamespaceIdentityTest(unittest.TestCase):
    clang = '??0t_initializer@?A0xCC6F802@@QAE@PAX0@Z'
    vc6 = r'??0t_initializer@?%Z:\tmp\lane\src\forcefeedback.cpp2829932257@@QAE@PAX0@Z'

    def test_namespace_encoding_requires_unique_module_and_exact_signature(self):
        from homm3.retail_labels.source import vc6_function_name
        self.assertEqual(vc6_function_name(self.clang, [self.vc6], 'forcefeedback'), self.vc6)
        header = self.vc6.replace('forcefeedback.cpp', 'forcefeedback.h')
        self.assertEqual(vc6_function_name(self.clang, [header], 'forcefeedback'), header)
        self.assertEqual(vc6_function_name('exact', ['exact'], 'example'), 'exact')
        for candidates in (
                [self.vc6.replace('forcefeedback.cpp', 'other.cpp')],
                [header.replace('forcefeedback.h', 'other.h')],
                [self.vc6.replace('PAX0', 'HH')],
                [self.vc6.replace('t_initializer', 'other')],
                [self.vc6.replace('cpp2829932257', 'cpp')],
                [self.vc6, self.vc6.replace('2829932257', '123')]):
            with self.subTest(candidates=candidates):
                self.assertIsNone(vc6_function_name(self.clang, candidates, 'forcefeedback'))
        self.assertIsNone(vc6_function_name('ordinary', [self.vc6], 'forcefeedback'))

    def test_claim_and_object_join_share_the_same_resolution(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        from homm3.match.source_ownership import claim_identity
        from homm3.retail_labels.source import ir_bind
        d = SimpleNamespace(file='src/forcefeedback.cpp', va=0x401000,
                            line=1, name='t_initializer::t_initializer',
                            mangled=self.clang, additional_instances=())
        claim = SimpleNamespace(kind='func', rva=0x1000, name=self.vc6)
        self.assertEqual(claim_identity([d], [claim]), [])
        claim.name = self.vc6.replace('forcefeedback.cpp', 'other.cpp')
        self.assertTrue(claim_identity([d], [claim]))
        rows, problems = [dict(channel='src-VA', rva=0x1000, size=8)], []
        with patch('homm3.retail_labels.source._base_authority_names',
                   return_value={'ctor': [(self.vc6, 8)]}):
            self.assertEqual(ir_bind('forcefeedback', rows, {0x1000: self.clang}, problems),
                             {self.vc6})
        self.assertEqual(rows[0]['joined'], self.vc6)
        self.assertEqual(problems, [])


class LabelGateTest(unittest.TestCase):
    def test_selftest(self):
        from homm3.retail_labels.source import selftest
        self.assertEqual(selftest(), [])

    def test_selftest_failure_is_reported_and_fails_build(self):
        import contextlib
        import io
        from unittest.mock import patch
        from homm3.retail_labels.source import main
        stderr = io.StringIO()
        with patch('homm3.retail_labels.source.run', return_value=([], [], [])), \
             patch('homm3.retail_labels.source.selftest', return_value=['broken control']), \
             contextlib.redirect_stderr(stderr):
            self.assertEqual(main(['--all']), 1)
        self.assertIn('SELFTEST BROKEN: broken control', stderr.getvalue())

    def test_unit_extraction_does_not_silence_fatal_problems(self):
        import contextlib
        import io
        from unittest.mock import patch
        from homm3.retail_labels.source import main
        with patch('homm3.retail_labels.source.run', return_value=([], [], ['(FATAL) conflict'])), \
             contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(main(['--unit', 'example']), 1)


if __name__ == '__main__':
    unittest.main()
