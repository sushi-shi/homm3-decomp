"""Spelling bridges must preserve TU, source origin, function and complete type."""
import unittest

from homm3.analysis import candidate_data, code_data_bindings, data_symbols


def fact(**changes):
    return dict(dict(name='table', symbol='?table@?A0x123ABC@@3PAHA', unit='a',
        anonymous_namespace_files=['src/a.cpp'], linkage='INTERNAL', local=False,
        usr='table', size=12, path='src/a.cpp', line=1), **changes)


ANON = r'?table@?%Z:\checkout\src\a.cpp123456@@3PAHA'
PARENT = '?function@@YAPAHH@Z'
LOCAL = '?table@?1?'+PARENT+'@4PAHA'


def local(**changes):
    return fact(**dict(dict(symbol=LOCAL, local=True, storage='STATIC', parent_symbol=PARENT), **changes))


class DataSymbolsTest(unittest.TestCase):
    def test_anonymous_scope_requires_observed_origin_in_owning_tu(self):
        proof = data_symbols.spelling(fact(), ANON, 'a')
        self.assertEqual(proof['rules'], ['anonymous-namespace-origin'])
        self.assertEqual(proof['anonymous_origins'][0]['source_path'], 'src/a.cpp')
        for f, name, unit in [(fact(), ANON, 'b'),
                (fact(anonymous_namespace_files=[]), ANON, 'a'),
                (fact(), ANON.replace('src', 'include'), 'a'),
                (fact(), ANON.replace('a.cpp', 'b.cpp'), 'a'),
                (fact(), ANON.replace('123456', 'crc'), 'a')]:
            self.assertIsNone(data_symbols.spelling(f, name, unit))

    def test_header_origin_and_windows_path_case_are_supported(self):
        f = fact(anonymous_namespace_files=['include/shared.h'])
        name = ANON.replace(r'src\a.cpp', r'INCLUDE\SHARED.H')
        self.assertIsNotNone(data_symbols.spelling(f, name, 'a'))

    def test_complete_named_scope_and_type_remain_distinct(self):
        for name in [ANON.replace('@@3', '@Other@@3'), ANON.replace('PAHA', 'PAMA'),
                     ANON.replace('?table@', '?other@')]:
            self.assertIsNone(data_symbols.spelling(fact(), name, 'a'))

    def test_unsupported_anonymous_back_reference_is_not_demangled_away(self):
        f = fact(symbol='?table@?A0x123ABC@@3PAUItem@?A0x123ABC@@A')
        name = ANON.replace('PAHA', 'PAUItem@1@A')
        self.assertIsNone(data_symbols.spelling(f, name, 'a'))

    def test_local_discriminator_encoding_and_parent_signature(self):
        for ordinal in ['1', '4', '9', 'BC@']:
            name = '_'+LOCAL.replace('@?1?', '@?'+ordinal+'?')
            self.assertIsNotNone(data_symbols.spelling(local(), name, 'a'))
        for name in ['_'+LOCAL.replace('YAPAHH', 'YAPAHM'),
                     '_'+LOCAL.replace('@4PAHA', '@4PAMA'),
                     '_'+LOCAL.replace('@?1?', '@?Z?'),
                     '_'+LOCAL.replace('?function', '?other')]:
            self.assertIsNone(data_symbols.spelling(local(), name, 'a'))
        self.assertIsNone(data_symbols.spelling(local(parent_symbol=''), '_'+LOCAL.replace('@?1?', '@?4?'), 'a'))
        self.assertIsNone(data_symbols.spelling(local(), '_'+LOCAL, 'b'))

    def test_local_inside_anonymous_function_composes_without_losing_scope(self):
        parent = '?function@?A0x123ABC@@YAPAHH@Z'
        f = local(symbol=LOCAL.replace(PARENT, parent), parent_symbol=parent)
        emitted = '_'+f['symbol'].replace('@?1?', '@?4?').replace('@?A0x123ABC@', r'@?%Z:\src\a.cpp77@')
        proof = data_symbols.spelling(f, emitted, 'a')
        self.assertEqual(proof['rules'], ['vc6-local-prefix', 'anonymous-namespace-origin',
                                          'local-static-discriminator'])

    def test_const_array_reference_preserves_dimensions_and_element_qualifiers(self):
        f = fact(symbol='?reference@@3AAY02$$CBHB', name='reference', linkage='EXTERNAL',
                 reference_cell=True, const_array_reference=True)
        emitted = '?reference@@3AAY02$$CBHA'
        self.assertEqual(data_symbols.spelling(f, emitted, 'a')['rules'], ['const-array-reference-cell'])
        for name in [emitted.replace('Y02', 'Y03'), emitted.replace('$$CB', '$$CC'),
                     emitted.replace('HA', 'MA')]:
            self.assertIsNone(data_symbols.spelling(f, name, 'a'))
        for field in ['reference_cell', 'const_array_reference']:
            self.assertIsNone(data_symbols.spelling(dict(f, **{field: False}), emitted, 'a'))
        self.assertIsNone(data_symbols.spelling(f, emitted, 'b'))

    def test_scalar_reference_and_pointer_qualifiers_are_never_erased(self):
        for symbol in ['?reference@@3ABHB', '?reference@@3PBHB']:
            f = fact(symbol=symbol, reference_cell=True, const_array_reference=False)
            self.assertIsNone(data_symbols.spelling(f, symbol[:-1]+'A', 'a'))
            self.assertEqual(data_symbols.spelling(f, symbol, 'a')['rules'], ['exact'])

    def test_internal_c_spelling_requires_internal_linkage_and_owning_tu(self):
        self.assertIsNotNone(data_symbols.spelling(fact(symbol='?table@@3PAHA'), '_table', 'a'))
        for symbol in ['?table@?A0x123ABC@@3PAHA', '?table@Other@@3PAHA']:
            self.assertIsNotNone(data_symbols.spelling(fact(symbol=symbol), '_table', 'a'))
            self.assertIsNone(data_symbols.spelling(fact(symbol=symbol, linkage='EXTERNAL'), '_table', 'a'))
            self.assertIsNone(data_symbols.spelling(fact(symbol=symbol), '_table', 'b'))

    def test_exact_local_spelling_does_not_suppress_alternative_scope(self):
        rows = [dict(id=str(i), unit='a', symbols=[name], physical_size=12)
                for i, name in enumerate([LOCAL, '_'+LOCAL.replace('@?1?', '@?4?')])]
        f = local()
        declaration = dict(f, symbols=[f['symbol']], units=['a'], definition_units=['a'])
        self.assertEqual(len(candidate_data.named_candidates(declaration, rows)), 2)
        declared = dict(units=[dict(unit='a', errors=[], definitions=[f], storage_declarations=[f])])
        matches = code_data_bindings.definitions(declared, rows)
        for row in rows:
            self.assertEqual(code_data_bindings.extent(row, matches[row['id']], [], None)[2],
                             'ambiguous-source-definition')

    def test_distinct_local_source_entities_with_one_emission_remain_ambiguous(self):
        a, b = local(usr='first'), local(usr='second')
        rows = [dict(id='only', unit='a', symbols=['_'+LOCAL], physical_size=12)]
        declared = dict(units=[dict(unit='a', errors=[], definitions=[a,b], storage_declarations=[a,b])])
        matches = code_data_bindings.definitions(declared, rows)
        self.assertEqual(code_data_bindings.extent(rows[0], matches['only'], [], None)[2],
                         'ambiguous-source-definition')
        declaration = dict(a, id='decl', rva=0x1000, symbols=[a['symbol']], units=['a'],
                           definition_units=['a'], source='src/a.cpp:1', status='sized')
        declared.update(declarations=[declaration], sites=[])
        self.assertEqual(candidate_data.bind(declared, rows, {})[0]['status'], 'ambiguous-source-definition')

    def test_c_spelling_does_not_merge_distinct_namespace_entities(self):
        a, b = fact(symbol='?table@First@@3PAHA', usr='first'), fact(symbol='?table@Second@@3PAHA', usr='second')
        rows = [dict(id='only', unit='a', symbols=['_table'], physical_size=12)]
        declared = dict(units=[dict(unit='a', errors=[], definitions=[a,b], storage_declarations=[a,b])])
        matches = code_data_bindings.definitions(declared, rows)
        self.assertEqual(code_data_bindings.extent(rows[0], matches['only'], [], None)[2],
                         'ambiguous-source-definition')
        declaration = dict(a, id='decl', rva=0x1000, symbols=[a['symbol']], units=['a'],
                           definition_units=['a'], source='src/a.cpp:1', status='sized')
        declared.update(declarations=[declaration], sites=[])
        self.assertEqual(candidate_data.bind(declared, rows, {})[0]['status'], 'ambiguous-source-definition')


if __name__ == '__main__':
    unittest.main()
