"""Emission identity must survive layout, pooling, linkage and freshness defects."""
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest

from homm3.analysis import candidate_data as data
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
from homm3.core import tsv


def coff(raw=b'abcdefgh', symbols=None, section='.data', flags=0xC0300040, relocations=()):
    payload = bytearray(_coff((FixtureSection(section, raw, relocations),),
                             tuple(symbols or [_symbol('_table', 0, 1, 0, 2)])))
    struct.pack_into('<I', payload, 56, flags)
    return CoffObject(bytes(payload))


def declaration(**kwargs):
    return dict(dict(id='decl', name='table', symbols=['_table'], definition_units=['a'],
                     units=['a'], linkage='EXTERNAL', local=False, usr='table', rva=0x1000,
                     size=8, status='sized', source='src/a.cpp:2'), **kwargs)


def report(declarations=(), sites=(), units=()):
    return dict(declarations=list(declarations), sites=list(sites), units=list(units))


class CandidateDataTest(unittest.TestCase):
    def bind(self, obj, declarations):
        return data.bind(report(declarations), data.inventory('a', obj, 'sha'), {'a': obj})

    def test_partition_keeps_padding_aliases_and_unnamed_bytes_without_logical_extent(self):
        obj = coff(symbols=[_symbol('_table', 2, 1, 0, 2), _symbol('_alias', 2, 1, 0, 2),
                            _symbol('_next', 6, 1, 0, 2)])
        rows = data.inventory('a', obj, 'sha')
        self.assertEqual([r['physical_size'] for r in rows], [2, 4, 2])
        self.assertEqual(rows[0]['symbols'], [])
        self.assertEqual(rows[1]['symbols'], ['_table', '_alias'])
        self.assertEqual(sum(r['physical_size'] for r in rows), 8)
        self.assertEqual(rows[1]['alignment'], 4)

    def test_debug_and_non_linkable_sections_are_excluded(self):
        for section, flags in [('.debug$F', 0x40), ('.drectve', 0x40), ('.data', 0x840),
                               ('.data', 0x240), ('.text', 0x60000020)]:
            self.assertFalse(data.inventory('a', coff(section=section, flags=flags), 'sha'))

    def test_truncated_table_rejected_and_padding_not_counted_as_logical_size(self):
        obj = coff()
        good = self.bind(obj, [declaration(size=5)])[0]
        self.assertEqual((good['status'], good['size']), ('bound', 5))
        self.assertEqual(good['candidate_match'], 'not-compared')
        self.assertEqual(self.bind(obj, [declaration(size=9)])[0]['status'], 'truncated-storage')

    def test_changed_symbol_and_wrong_unit_cannot_bind(self):
        for obj, decl in [(coff(symbols=[_symbol('_other', 0, 1, 0, 2)]), declaration()),
                          (coff(), declaration(definition_units=['b']))]:
            self.assertEqual(self.bind(obj, [decl])[0]['status'], 'missing-emission')

    def test_internal_global_spelling_cannot_capture_local_or_external(self):
        for linkage, local, expected in [('INTERNAL', False, 'bound'),
                                         ('INTERNAL', True, 'missing-emission'),
                                         ('EXTERNAL', False, 'missing-emission')]:
            decl = declaration(symbols=['?table@@3HA'], linkage=linkage, local=local)
            self.assertEqual(self.bind(coff(), [decl])[0]['status'], expected)

    def test_contradictory_definitions_preserve_both_emitted_symbols(self):
        obj = coff(symbols=[_symbol('_small', 0, 1, 0, 2), _symbol('_large', 4, 1, 0, 2)])
        decl = declaration(symbols=['_small'], definitions=[{'symbol': '_large'}],
                           size=None, status='conflicting-size')
        result = self.bind(obj, [decl])[0]
        self.assertEqual(result['status'], 'conflicting-size')
        self.assertEqual(len(result['candidate_ids']), 2)

    def test_duplicate_owners_and_common_requests_not_merged_without_link_proof(self):
        obj = coff()
        rows = data.inventory('a', obj, 'sha') + data.inventory('b', obj, 'sha')
        decl = declaration(definition_units=['a', 'b'])
        self.assertEqual(data.bind(report([decl]), rows, {'a': obj, 'b': obj})[0]['status'],
                         'multiple-emitted-owners')
        common = coff(symbols=[_symbol('_table', 12, 0, 0, 2)])
        common_row = data.inventory('a', common, 'sha')[-1]
        self.assertEqual(common_row['allocation'], 'common-request')
        self.assertEqual(common_row['bytes_sha256'], '')
        self.assertEqual(self.bind(common, [declaration()])[0]['status'], 'common-size-conflict')

    def test_reference_cells_retain_relocation_target_and_addend(self):
        obj = coff(raw=struct.pack('<II', 4, 99), symbols=[_symbol('_table', 0, 1, 0, 2),
                                                       _symbol('_dest', 0, 0, 0, 2)],
                   relocations=((0, 1, 6),))
        row = data.inventory('a', obj, 'sha')[0]
        self.assertEqual(row['relocations'][0]['target'], '_dest')
        self.assertEqual(row['relocations'][0]['addend'], 4)
        self.assertEqual(self.bind(obj, [declaration(size=4)])[0]['candidate_match'], 'not-compared')

    def test_c_string_semantics_and_unsupported_expressions(self):
        self.assertEqual(data.byte_literal(r'"a\000b" "\xff" "\101\n"'), b'a\0b\xffA\n\0')
        self.assertEqual(data.byte_literal('""'), b'\0')
        for expression in ['L"wide"', 'TEXT("macro")', '1.5', r'"\x123"', r'"\q"', '"é"']:
            self.assertIsNone(data.byte_literal(expression), expression)

    def pooled(self, obj, *, active=True, rvas=(0x1000,)):
        sites = [dict(path='src/a.cpp', line=2, offset=i, macro='DATA_COMPGEN', name='message',
                      expression='"abc"', rva=rva) for i, rva in enumerate(rvas)]
        units = [dict(unit='a', errors=[], active_macros=sites)] if active else []
        return data.bind(report(sites=sites, units=units), data.inventory('a', obj, 'sha'), {'a': obj})

    def test_pool_identity_requires_active_annotation_and_unique_allocation(self):
        obj = coff(raw=b'abc\0abc\0', symbols=[_symbol('$SG1', 0, 1, 0, 3),
                                             _symbol('$SG2', 4, 1, 0, 3)], flags=0x40300040)
        self.assertEqual(self.pooled(obj)[0]['status'], 'ambiguous-pool')
        obj = coff(raw=b'abc\0', symbols=[_symbol('$SG1', 0, 1, 0, 3)], flags=0x40300040)
        self.assertEqual(self.pooled(obj)[0]['status'], 'bound')
        self.assertEqual(self.pooled(obj, active=False)[0]['status'], 'inactive-or-unparsed-site')
        self.assertEqual({b['status'] for b in self.pooled(obj, rvas=(0x1000, 0x1004))},
                         {'conflicting-retail-address'})
        self.assertEqual({b['status'] for b in self.pooled(obj, rvas=(0x1000, 0x1000))}, {'bound'})

    def test_unrelated_zeros_or_global_array_cannot_satisfy_literal(self):
        self.assertEqual(self.pooled(coff(raw=b'abc\0', flags=0x40300040))[0]['status'],
                         'missing-pool-emission')

    def test_literal_prefix_does_not_bind_a_different_embedded_nul_literal(self):
        obj = coff(raw=b'abc\0xyz\0', symbols=[_symbol('$SG1', 0, 1, 0, 3)], flags=0x40300040)
        self.assertEqual(self.pooled(obj)[0]['status'], 'missing-pool-emission')

    def test_guard_needs_typed_symbol_shared_scope_and_both_function_references(self):
        function = '?f@@YAXXZ'
        owner = SimpleNamespace(index=1, section=1, name='_?table@?1?' + function + '@4HA', typ=0)
        guard = SimpleNamespace(index=2, section=1, name='_?$S7@?1?' + function + '@4EA', typ=0)
        func = SimpleNamespace(index=3, section=2, name=function, typ=0x20, value=0)
        obj = SimpleNamespace(symbols={1: owner, 2: guard, 3: func},
                              relocations=[SimpleNamespace(symbol_index=i, section=2, site=i) for i in (1, 2)],
                              sections=[None, SimpleNamespace(raw_size=8)])
        rows = [dict(symbol_indices=[2], physical_size=4)]
        site = dict(expression='table')
        self.assertEqual(data.guard_candidates(site, function, rows, obj), rows)
        func.name = '?caller@@YAXXZ'
        self.assertEqual(data.guard_candidates(site, function, rows, obj), rows)
        obj.relocations.pop()
        self.assertFalse(data.guard_candidates(site, function, rows, obj))
        obj.relocations.append(SimpleNamespace(symbol_index=2, section=2, site=2))
        guard.name = guard.name.replace('@4EA', '@4HA')
        self.assertFalse(data.guard_candidates(site, function, rows, obj))

    def test_export_keeps_one_row_per_site_and_literal_tabs_roundtrip(self):
        obj = coff()
        bindings = self.bind(obj, [declaration(source='file\tname:2')])
        with tempfile.TemporaryDirectory() as directory:
            result = dict(candidate_data=data.inventory('a', obj, 'sha'), data_bindings=bindings,
                          candidate_issues=[], summary={'matched_bytes': 0})
            data.export(result, Path(directory))
            rows = tsv.read(Path(directory)/'data-bindings.tsv')[2]
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]['size'], '8')
            self.assertEqual(rows[0]['source'], '"file\\tname:2"')


if __name__ == '__main__':
    unittest.main()
