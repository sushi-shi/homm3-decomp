"""Vendor addresses must be independent of the data bytes being compared."""
import struct
import unittest
from dataclasses import replace

from homm3.analysis import vendor_bindings as bindings
from homm3.analysis.test_vendor_data import obj, put
from homm3.analysis import test_vendor_data as fixtures
from homm3.build.test_eh_handler_normalization import FixtureSection, _symbol, _coff, _section_aux
from homm3.analysis import vendor_data
from homm3.sema import data_match
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


class VendorBindingsTest(unittest.TestCase):
    def bind(self, retail, objects, seeds=None, **kwargs):
        # The generic PE layout fixture omits permission flags. Supply a real
        # executable text section for code-root proof tests.
        retail = bytearray(retail)
        struct.pack_into('<I', retail, 0x178+36, 0x60000020)
        return bindings.bind(Layout(retail), objects,
                             seeds if seeds is not None else {('', '_func'): {0x1000}}, **kwargs)

    def compare(self, retail, report):
        return data_match.compare(Layout(retail), report['candidate_data'], report['data_bindings'],
                                  {unit: obj.coff for unit, obj in report['objects'].items()})

    def test_initializer_differences_do_not_remove_enrollment(self):
        retail, candidate = fixtures.VendorMatchTest().setup_object(addend=-2)
        original = self.bind(retail, [candidate])
        put(retail, 0x2018, b'!')
        changed = self.bind(retail, [candidate])
        self.assertEqual(original['data_bindings'], changed['data_bindings'])
        self.assertEqual(changed['placements'][0]['rva'], 0x2010)
        self.assertEqual(changed['placements'][0]['status'], 'bound')
        result = self.compare(retail, changed)
        self.assertEqual(result['summary']['bytes_by_status']['fixed-mismatch'], 1)

    def pointer_fixture(self, independent=False):
        raw = b'abcdefgh'+bytes(8)+b'\xc3'
        refs = ((8, 1, 6), (12, 2, 6)) if independent else ((8, 1, 6),)
        candidate = obj([FixtureSection('.text', raw, refs),
                         FixtureSection('.data', bytes(4), ((0, 2, 6),)),
                         FixtureSection('.data', b'target!!', ())],
                        [_symbol('_func', 0, 1, 0x20, 2),
                         _symbol('_pointer', 0, 2, 0, 2), _symbol('_target', 0, 3, 0, 2)])
        retail = fixture()
        linked = raw[:8]+struct.pack('<I', 0x402010)
        linked += struct.pack('<I', 0x402030) if independent else bytes(4)
        put(retail, 0x1000, linked+raw[16:])
        put(retail, 0x2010, struct.pack('<I', 0x402030))
        put(retail, 0x2030, b'target!!')
        return retail, candidate

    def test_data_edge_cannot_bootstrap_its_own_destination(self):
        retail, candidate = self.pointer_fixture()
        report = self.bind(retail, [candidate])
        self.assertEqual(len(report['placements']), 1)
        self.assertEqual(self.compare(retail, report)['relocations'][0]['status'], 'pointer-unresolved')

    def test_independent_code_reference_detects_wrong_data_pointer(self):
        retail, candidate = self.pointer_fixture(independent=True)
        report = self.bind(retail, [candidate])
        self.assertEqual(self.compare(retail, report)['relocations'][0]['status'], 'pointer-match')
        put(retail, 0x2010, struct.pack('<I', 0x402034))
        self.assertEqual(report['data_bindings'], self.bind(retail, [candidate])['data_bindings'])
        self.assertEqual(self.compare(retail, report)['relocations'][0]['status'], 'pointer-mismatch')

    def test_conflicting_roots_do_not_select_a_convenient_copy(self):
        retail, candidate = fixtures.VendorMatchTest().setup_object()
        put(retail, 0x1080, bytes(retail[0x400:0x40d]))
        report = self.bind(retail, [candidate], {('', '_func'): {0x1000, 0x1080}})
        self.assertTrue(report['data_bindings'])
        self.assertTrue(all(b['status'] == 'unproved-code-path' for b in report['data_bindings']))
        self.assertEqual(self.compare(retail, report)['summary']['enrolled_bytes'], 0)

    def test_missing_or_changed_code_cannot_bind_data(self):
        retail, candidate = fixtures.VendorMatchTest().setup_object()
        self.assertFalse(self.bind(retail, [candidate], {})['data_bindings'])
        put(retail, 0x1000, b'!')
        report = self.bind(retail, [candidate])
        self.assertFalse(report['data_bindings'])
        self.assertEqual(report['code'][0]['status'], 'unproved')

    def test_data_bytes_cannot_masquerade_as_an_executable_code_anchor(self):
        retail, candidate = fixtures.VendorMatchTest().setup_object()
        put(retail, 0x2050, bytes(retail[0x400:0x40d]))
        report = self.bind(retail, [candidate], {('', '_func'): {0x2050}})
        self.assertFalse(report['data_bindings'])
        self.assertIn('outside executable', report['code'][0]['detail'])

    def test_conflicting_admitted_target_invalidates_code_path(self):
        retail, candidate = fixtures.VendorMatchTest().setup_object()
        report = self.bind(retail, [candidate], {('', '_func'): {0x1000}, ('', '_data'): {0x2040}})
        self.assertTrue(all(b['status'] == 'unproved-code-path' for b in report['data_bindings']))
        self.assertEqual(self.compare(retail, report)['summary']['enrolled_bytes'], 0)

    def test_archive_alternatives_cannot_invent_nonoverlapping_storage(self):
        raw = b'abcdefgh'+bytes(4)+b'\xc3'
        caller = obj([FixtureSection('.text', raw, ((8, 1, 6),))],
                     [_symbol('_func', 0, 1, 0x20, 2), _symbol('_array', 0, 0, 0, 2)])
        first = obj([FixtureSection('.data', bytes(16), ())], [_symbol('_array', 0, 1, 0, 2)])
        second = obj([FixtureSection('.data', bytes(32), ())], [_symbol('_array', 24, 1, 0, 2)])
        first.member, second.member = 'first.obj', 'second.obj'
        retail = fixture()
        put(retail, 0x1000, raw[:8]+struct.pack('<I', 0x402030)+raw[12:])
        report = self.bind(retail, [caller, first, second])
        self.assertTrue(report['data_bindings'])
        self.assertEqual({b['status'] for b in report['data_bindings']}, {'ambiguous-archive-definition'})
        self.assertEqual(self.compare(retail, report)['summary']['enrolled_bytes'], 0)

    def pooled_object(self, payload, member, selection=2, symbol='_pool'):
        raw = b'abcdefgh'+bytes(4)+b'\xc3'
        data = bytearray(_coff((FixtureSection('.text', raw, ((8, 3, 6),)),
                               FixtureSection('.data', payload, ())),
                              (_symbol('_func', 0, 1, 0x20, 2),
                               _symbol('.data', 0, 2, 0, 3, _section_aux(len(payload), 0, selection=selection)),
                               _symbol(symbol, 0, 2, 0, 2))))
        struct.pack_into('<I', data, 56, 0x60000020)
        struct.pack_into('<I', data, 96, 0xC0301040)
        return vendor_data.load_object('test.lib', member, 'test.lib', bytes(data))

    def test_explicit_comdat_identity_keeps_bad_copy_visible(self):
        retail = fixture()
        put(retail, 0x1000, b'abcdefgh'+struct.pack('<I', 0x402010)+b'\xc3')
        put(retail, 0x2010, b'good')
        first = self.pooled_object(b'good', 'a.obj')
        second = self.pooled_object(b'bad!', 'b.obj')
        report = self.bind(retail, [first, second])
        result = self.compare(retail, report)
        self.assertEqual(result['summary']['enrolled_bytes'], 4)
        self.assertEqual(result['summary']['bytes_by_status']['fixed-mismatch'], 4)
        self.assertNotIn('binding-conflict', result['summary']['bytes_by_status'])
        self.assertEqual({r['status'] for r in result['matches']}, {'static-exact', 'not-exact'})

    def test_equal_bytes_without_linker_identity_cannot_merge(self):
        retail = fixture()
        put(retail, 0x1000, b'abcdefgh'+struct.pack('<I', 0x402010)+b'\xc3')
        put(retail, 0x2010, bytes(4))
        for second in (self.pooled_object(bytes(4), 'b.obj', selection=1),
                       self.pooled_object(bytes(4), 'b.obj', symbol='_other')):
            report = self.bind(retail, [self.pooled_object(bytes(4), 'a.obj'), second])
            self.assertEqual(self.compare(retail, report)['summary']['bytes_by_status']['binding-conflict'], 4)

    def test_largest_comdat_public_tail_can_share_any_identity_without_claiming_prefix(self):
        raw = bytearray(_coff((FixtureSection('.data', b'HEADbody', ()),),
                             (_symbol('.data', 0, 1, 0, 3, _section_aux(8, 0, selection=6)),
                              _symbol('_pool', 4, 1, 0, 2))))
        struct.pack_into('<I', raw, 56, 0x40301040)
        largest = vendor_data.load_object('test.lib', 'large.obj', 'test.lib', bytes(raw))
        from homm3.analysis.candidate_data import inventory
        rows = inventory('large', largest.coff, largest.digest)
        self.assertFalse(bindings.linker_identity(largest.coff, rows[0]))
        any_copy = self.pooled_object(b'body', 'any.obj')
        any_row = inventory('any', any_copy.coff, any_copy.digest)[0]
        self.assertEqual(bindings.linker_identity(largest.coff, rows[1]),
                         bindings.linker_identity(any_copy.coff, any_row))
        self.assertTrue(bindings.linker_identity(largest.coff, rows[1]))
        self.assertFalse(bindings.linker_identity(largest.coff, dict(rows[1], physical_size=3)))

    def test_common_copies_require_same_name_size_and_location(self):
        raw = b'abcdefgh'+bytes(4)+b'\xc3'
        first = obj([FixtureSection('.text', raw, ((8, 1, 6),))],
                    [_symbol('_func', 0, 1, 0x20, 2), _symbol('_buffer', 32, 0, 0, 2)])
        second = replace(first, member='second.obj')
        retail = fixture()
        put(retail, 0x1000, raw[:8]+struct.pack('<I', 0x403040)+raw[12:])
        report = self.bind(retail, [first, second])
        result = self.compare(retail, report)
        self.assertEqual(result['summary']['enrolled_bytes'], 32)
        self.assertEqual(result['summary']['zero_fill_agreement_bytes'], 32)
        report['data_bindings'][1]['size'] = 16
        self.assertEqual(self.compare(retail, report)['summary']['bytes_by_status']['binding-conflict'], 32)

    def test_common_request_does_not_override_an_unselected_strong_initializer(self):
        raw = b'abcdefgh'+bytes(4)+b'\xc3'
        caller = obj([FixtureSection('.text', raw, ((8, 1, 6),))],
                     [_symbol('_func', 0, 1, 0x20, 2), _symbol('_buffer', 4, 0, 0, 2)])
        strong = obj([FixtureSection('.data', b'init', ())], [_symbol('_buffer', 0, 1, 0, 2)])
        strong.member = 'strong.obj'
        retail = fixture()
        put(retail, 0x1000, raw[:8]+struct.pack('<I', 0x402010)+raw[12:])
        for payload in (bytes(4), b'init', b'bad!'):
            put(retail, 0x2010, payload)
            report = self.bind(retail, [caller, strong])
            self.assertEqual(report['data_bindings'][0]['status'], 'common-initializer-unproved')
            self.assertEqual(report['data_bindings'][0]['possible_strong_definitions'],
                             [dict(node=[1, 1], symbol='_buffer', offset=0)])
            self.assertEqual(self.compare(retail, report)['summary']['enrolled_bytes'], 0)
        report = self.bind(retail, [caller], source_definitions={'_buffer': [
            dict(unit='game', symbol='_buffer', section=1, offset=0)]})
        self.assertEqual(report['data_bindings'][0]['status'], 'common-initializer-unproved')


if __name__ == '__main__':
    unittest.main()
