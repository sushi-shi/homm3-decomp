"""Code-derived identities must respect individual owners and typed extents."""
import struct
import unittest

from homm3.analysis import candidate_data, code_data_bindings as code
from homm3.analysis import test_vendor_bindings as vendor_fixtures
from homm3.analysis.test_vendor_data import obj, put
from homm3.build.test_eh_handler_normalization import FixtureSection, _symbol, _section_aux
from homm3.sema import data_match
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


def fact(name, size, **extra):
    return dict(dict(name=name, symbol='_'+name, size=size, usr=name, path='src/a.cpp', line=1,
                     linkage='EXTERNAL', local=False, unit='a'), **extra)


class CodeDataBindingsTest(unittest.TestCase):
    def setup_objects(self, *, section_symbol=False, addend=0):
        raw = b'abcdefgh'+struct.pack('<ii', addend, 0)+b'\xc3'
        symbols = [_symbol('_func', 0, 1, 0x20, 2), _symbol('_one', 0, 2, 0, 2),
                   _symbol('_two', 8, 2, 0, 2)]
        refs = ((8, 1, 6), (12, 2, 6))
        if section_symbol:
            symbols += [_symbol('.data', 0, 2, 0, 3, _section_aux(16, 0))]
            refs = ((8, 3, 6),)
            raw = b'abcdefgh'+struct.pack('<ii', 8, 0)+b'\xc3'
        candidate = obj([FixtureSection('.text', raw, refs),
                         FixtureSection('.data', b'firstPADsecond!!', ())], symbols).coff
        retail = fixture()
        struct.pack_into('<I', retail, 0x178+36, 0x60000020)
        linked = b'abcdefgh'+struct.pack('<II', 0x402050 if section_symbol else 0x402010+addend,
                                        0 if section_symbol else 0x402050)+b'\xc3'
        put(retail, 0x1000, linked)
        put(retail, 0x2010, b'first')
        put(retail, 0x2050, b'second!!')
        return retail, candidate

    def bind(self, retail, candidate, *, definitions=None, bindings=(), declarations=()):
        rows = candidate_data.inventory('a', candidate, 'sha')
        definitions = [fact('one', 5), fact('two', 8)] if definitions is None else definitions
        declared = dict(units=[dict(unit='a', errors=[], definitions=definitions,
                                   storage_declarations=list(declarations))])
        report = code.bind(Layout(retail), {'a': candidate}, rows, list(bindings), declared,
                           [dict(unit='a', symbol='_func', rva=0x1000)])
        report['rows'] = rows
        return report

    def compare(self, retail, candidate, report):
        return data_match.compare(Layout(retail), report['rows'],
            report['source_bindings']+report['data_bindings'], {'a': candidate})

    def test_individual_allocations_do_not_inherit_candidate_section_spacing(self):
        retail, candidate = self.setup_objects()
        report = self.bind(retail, candidate)
        self.assertEqual([(b['rva'], b['size'], b['status']) for b in report['data_bindings']],
                         [(0x2010, 5, 'bound'), (0x2050, 8, 'bound')])
        self.assertEqual(self.compare(retail, candidate, report)['summary']['matched_initialized_bytes'], 13)
        put(retail, 0x2011, b'!')
        changed = self.bind(retail, candidate)
        self.assertEqual(report['data_bindings'], changed['data_bindings'])
        self.assertEqual(self.compare(retail, candidate, changed)['summary']['bytes_by_status']['fixed-mismatch'], 1)

    def test_section_symbol_addend_identifies_only_the_referenced_owner(self):
        retail, candidate = self.setup_objects(section_symbol=True)
        report = self.bind(retail, candidate)
        self.assertEqual([(b['name'], b['rva'], b['size']) for b in report['data_bindings']],
                         [('_two', 0x2050, 8)])

    def test_unknown_extent_does_not_claim_padding_or_neighbor_bytes(self):
        retail, candidate = self.setup_objects()
        report = self.bind(retail, candidate, definitions=[])
        self.assertEqual({b['status'] for b in report['data_bindings']}, {'source-extent-unproved'})
        self.assertEqual(self.compare(retail, candidate, report)['summary']['enrolled_bytes'], 0)

    def test_complete_scalar_type_suffix_sizes_skipped_local_static_without_padding(self):
        from dataclasses import replace
        retail, candidate = self.setup_objects()
        candidate.symbols[1] = replace(candidate.symbols[1], name='_?$S3@?1??f@@YAXXZ@4EA')
        report = self.bind(retail, candidate, definitions=[])
        row = report['data_bindings'][0]
        self.assertEqual((row['size'], row['extent_kind'], row['status']), (1, 'compiler-type', 'bound'))
        candidate.symbols[1] = replace(candidate.symbols[1], name='_?array@?1??f@@YAXXZ@4PAY03HA')
        self.assertEqual(self.bind(retail, candidate, definitions=[])['data_bindings'][0]['status'],
                         'source-extent-unproved')

    def test_out_of_extent_code_addend_remains_unproved(self):
        retail, candidate = self.setup_objects(addend=6)
        report = self.bind(retail, candidate)
        self.assertEqual(report['data_bindings'][0]['status'], 'code-reference-outside-source-extent')
        self.assertEqual(report['data_bindings'][1]['status'], 'bound')

    def test_unannotated_reader_declaration_cannot_disagree_with_writer_extent(self):
        retail, candidate = self.setup_objects()
        report = self.bind(retail, candidate, declarations=[fact('one', 9)])
        self.assertEqual(report['data_bindings'][0]['status'], 'conflicting-source-size')

    def test_equal_total_bytes_do_not_hide_unannotated_shape_conflict(self):
        retail, candidate = self.setup_objects()
        shape = dict(dimensions=[2, 4], strides_bytes=[4, 1], element_type='char',
                     element_kind='CHAR_S', element_bytes=1, pointee_bytes=None)
        report = self.bind(retail, candidate, definitions=[fact('two', 8, shape=shape)],
                           declarations=[fact('two', 8, shape=dict(shape, dimensions=[4, 2], strides_bytes=[2, 1]))])
        self.assertEqual(report['data_bindings'][1]['status'], 'conflicting-source-shape')

    def test_code_contradiction_preserves_independent_source_anchor_for_consumer_checks(self):
        retail, candidate = self.setup_objects()
        prior = dict(id=100, name='one', candidate_ids=['a:2:0'], size=5, rva=0x2020,
                     status='bound', macro='DATA', literal_sha256='', source_identity='one')
        report = self.bind(retail, candidate, bindings=[prior])
        self.assertEqual(report['data_bindings'][0]['status'], 'conflicting-source-address')
        self.assertEqual(report['source_bindings'][0], prior)
        self.assertEqual(self.compare(retail, candidate, report)['summary']['enrolled_bytes'], 13)

    def test_different_references_to_one_owner_do_not_pick_a_convenient_address(self):
        retail, candidate = self.setup_objects()
        from dataclasses import replace
        candidate.relocations = tuple(replace(r, symbol_index=1) for r in candidate.relocations)
        report = self.bind(retail, candidate)
        self.assertEqual({b['status'] for b in report['data_bindings']}, {'conflicting-code-placement'})
        self.assertEqual(self.compare(retail, candidate, report)['summary']['enrolled_bytes'], 0)

    def test_complete_comdat_keeps_physical_extent_separate_from_source_type(self):
        candidate = vendor_fixtures.VendorBindingsTest().pooled_object(b'word', 'a.obj').coff
        retail = fixture()
        struct.pack_into('<I', retail, 0x178+36, 0x60000020)
        put(retail, 0x1000, b'abcdefgh'+struct.pack('<I', 0x402010)+b'\xc3')
        put(retail, 0x2010, b'word')
        report = self.bind(retail, candidate, definitions=[])
        self.assertEqual(report['data_bindings'][0]['extent_kind'], 'coff-contribution')
        strict = self.compare(retail, candidate, report)
        self.assertEqual(strict['summary']['matched_initialized_bytes'], 4)
        from homm3.analysis.data_accesses import Storage
        from homm3.analysis.access_expressions import Expression
        storage = Storage(0x400000, strict['enrollment'])
        span = storage.extent(storage.normalize(Expression.constant(0x402010)), 4)
        self.assertEqual(span['status'], 'extent-unproved')
        self.assertTrue(span['within_all_emitted_spans'])


if __name__ == '__main__':
    unittest.main()
