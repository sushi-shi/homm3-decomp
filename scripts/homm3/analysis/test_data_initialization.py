"""CRT bounds and generic initializer defects need independent evidence."""
from dataclasses import replace
import struct
import unittest

from homm3.analysis import data_initialization as init
from homm3.analysis.data_effects import integer
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


def crt_fixture():
    image = fixture()
    image[0x178:0x180] = b'.text\0\0\0'
    struct.pack_into('<I', image, 0x178+36, 0x60000020)
    layout = Layout(image)
    raw = (b'\x68'+bytes(4)+b'\x68'+bytes(4)+b'\xe8'+bytes(4))*2+bytes.fromhex('83c410c3')
    helper = bytes.fromhex('568b7424083b74240c730d8b0685c07402ffd083c604ebed5ec3')
    symbols = [_symbol('cinit', 0, 1, 0x20, 2), _symbol('walker', 0, 2, 0x20, 3)]
    symbols += [_symbol(n, 0, 0, 0, 2) for n in ('xi_z', 'xi_a', 'xc_z', 'xc_a')]
    refs = ((1, 2, 6), (6, 3, 6), (11, 1, 20), (16, 4, 6), (21, 5, 6), (26, 1, 20))
    obj = CoffObject(_coff((FixtureSection('.text', raw, refs), FixtureSection('.text', helper, ())), tuple(symbols)))
    for index, name in enumerate(('__cinit', '__initterm', '___xi_z', '___xi_a', '___xc_z', '___xc_a')):
        obj.symbols[index] = replace(obj.symbols[index], name=name)
    retail = bytearray(raw)
    for offset, value in [(1, 0x402010), (6, 0x402000), (16, 0x402030), (21, 0x402020)]:
        struct.pack_into('<I', retail, offset, value)
    for offset in (11, 26):
        struct.pack_into('<i', retail, offset, 0x1080-(0x1000+offset+4))
    def put(rva, data):
        offset = layout.raw(rva, len(data))
        image[offset:offset+len(data)] = data
    put(0x1000, retail)
    put(0x1080, helper)
    put(0x1100, b'\xc3')
    for start in (0x2000, 0x2020):
        put(start, struct.pack('<5I', 0, 0x401100, 0, 0, 0))
    return image, obj, {'__cinit': {0x1000}}, {0x1000: len(raw), 0x1080: len(helper), 0x1100: 1}


class InitializationTest(unittest.TestCase):
    def test_bounds_require_crt_anchor_code_helper_and_sentinels(self):
        image, obj, runtime, functions = crt_fixture()
        tables, issues = init.retail_tables(Layout(image), obj, runtime, functions)
        self.assertFalse(issues)
        self.assertEqual([t['size'] for t in tables], [20, 20])
        self.assertEqual(tables[1]['slots'][1]['target_rva'], 0x1100)
        for rva, value in [(0x1000, 0x90), (0x1080, 0x90), (0x2000, 1), (0x2010, 1)]:
            changed = bytearray(image)
            changed[Layout(image).raw(rva, 1)] = value
            rows, errors = init.retail_tables(Layout(changed), obj, runtime, functions)
            self.assertFalse(rows)
            self.assertTrue(errors)
        self.assertFalse(init.retail_tables(Layout(image), obj, {}, functions)[0])

    def test_unknown_code_extent_is_explicit_but_noncode_slot_invalidates_table(self):
        image, obj, runtime, functions = crt_fixture()
        tables, errors = init.retail_tables(Layout(image), obj, runtime, {k: v for k, v in functions.items() if k != 0x1100})
        self.assertFalse(errors)
        self.assertEqual(tables[0]['slots'][1]['status'], 'registered-unbounded')
        struct.pack_into('<I', image, Layout(image).raw(0x2004, 4), 0x402100)
        self.assertFalse(init.retail_tables(Layout(image), obj, runtime, functions)[0])

    def test_nonzero_addend_and_wrong_helper_destination_cannot_define_bounds(self):
        image, obj, runtime, functions = crt_fixture()
        changed = bytearray(image)
        struct.pack_into('<I', changed, Layout(image).raw(0x100b, 4), 0)
        self.assertFalse(init.retail_tables(Layout(changed), obj, runtime, functions)[0])
        raw = bytearray(obj.data)
        raw[obj.sections[0].raw_offset+1] = 1
        mutated = CoffObject(bytes(raw))
        mutated.symbols = obj.symbols
        self.assertFalse(init.retail_tables(Layout(image), mutated, runtime, functions)[0])

    def test_registration_requires_real_body_and_aligned_dir32(self):
        symbols = (_symbol('init', 0, 2, 0x20, 2), _symbol('missing', 0, 0, 0x20, 2))
        raw = _coff((FixtureSection('.CRT$XCU', bytes(8), ((0, 0, 6), (4, 1, 6))),
                     FixtureSection('.text', b'\xc3', ())), symbols)
        rows = init.candidate_slots({'a': CoffObject(raw)})
        self.assertEqual([r['status'] for r in rows], ['registered', 'missing-body'])
        self.assertEqual(rows[0]['table_kind'], 'cpp-initializers')
        changed = CoffObject(raw)
        changed.relocations = (replace(changed.relocations[0], site=1), changed.relocations[1])
        self.assertEqual(init.candidate_slots({'a': changed})[0]['status'], 'invalid-slot')

    def profile(self, writes, **extra):
        return dict(dict(complete=True, writes=[dict(rva=rva, width=4, value=integer(value))
                    for rva, value in writes], owners=sorted({rva for rva, _ in writes}),
                    anchors=[], reads=[]), **extra)

    def test_missing_wrong_destination_and_changed_writes_fail_generically(self):
        retail = self.profile([(0x3000, 1), (0x3004, 2)])
        for candidate in [self.profile([(0x3000, 1)]), self.profile([(0x3000, 1), (0x3008, 2)]),
                          self.profile([(0x3000, 1), (0x3004, 3)])]:
            self.assertEqual(init.compare_profiles(retail, candidate)['status'], 'effects-differ')
        self.assertEqual(init.compare_profiles(retail, retail)['matched_effect_bytes'], 8)
        partial = self.profile([(0x3000, 1)], complete=False)
        result = init.compare_profiles(retail, partial)
        self.assertEqual(result['status'], 'effects-unproved')
        self.assertFalse(result['missing_writes_proven'])

    def test_wrong_missing_and_unknown_callback_are_separate(self):
        empty = self.profile([])
        for callbacks in [[], [0x1200]]:
            self.assertEqual(init.compare_profiles(empty, empty, [0x1100], callbacks)['registration_status'], 'different')
        self.assertEqual(init.compare_profiles(empty, empty, [0x1100], [None])['status'], 'effects-unproved')
        partial = self.profile([], complete=False, registration_complete=False)
        self.assertEqual(init.compare_profiles(empty, partial, [0x1100], [])['status'], 'effects-unproved')

    def test_storage_pairing_does_not_use_slot_order_or_accept_ambiguity(self):
        retail = [self.profile([(0x3000, 1)], root=0x1100), self.profile([(0x4000, 2)], root=0x1200)]
        candidate = [self.profile([(0x4000, 2)]), self.profile([(0x3000, 1)])]
        pairs, _ = init.pair_initializers(retail, candidate)
        self.assertEqual(pairs, {0: 1, 1: 0})
        candidate.append(self.profile([(0x4000, 3)]))
        self.assertEqual(init.pair_initializers(retail, candidate)[0], {1: 0})

    def test_inverted_proven_order_reports_dependency_bytes(self):
        retail = [dict(root=0x1100, table_id=0, ordinal=1), dict(root=0x1200, table_id=0, ordinal=2)]
        candidate = [self.profile([(0x3000, 1)], unit='a', section='.CRT$XCU', section_ordinal=1, offset=i*4)
                     for i in range(2)]
        issues = init.order_issues(retail, candidate, {0: 1, 1: 0})
        self.assertEqual(issues[0]['dependency_bytes'], list(range(0x3000, 0x3004)))
        candidate[1]['unit'] = 'b'
        self.assertFalse(init.order_issues(retail, candidate, {0: 1, 1: 0}))


if __name__ == '__main__':
    unittest.main()
