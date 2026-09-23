"""EH sizes and offsets need code roots; retail data cannot prove itself."""
from dataclasses import replace
import struct
import unittest

from homm3.analysis import candidate_data, code_data_bindings, compiler_eh
from homm3.analysis.test_vendor_data import obj, put
from homm3.build.test_eh_handler_normalization import FixtureSection, _symbol, _section_aux
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.sema import data_match
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


class CompilerEhTest(unittest.TestCase):
    def fixture(self, *, states=2, magic=0x19930520, section_name='.xdata$x', map_addend=0,
                associative=False, label=False, trailing_label=False, bad_boundary=False,
                header_only=False):
        prefix = b'\x55\x8b\xec\x83\xec\x08\x33\xc0'
        if bad_boundary:
            prefix = b'\x55\x8b\xec\x83\xc0\x00\x90\x68'
        code = prefix+b'\xb8'+bytes(4)+b'\xe9'+bytes(4)
        if trailing_label:
            code += b'\x90'*4
        metadata = bytearray(88)
        struct.pack_into('<7I', metadata, 0, magic, 0 if header_only else states,
                         map_addend, 0 if header_only else 1, 0, 0, 0)
        struct.pack_into('<iIiI', metadata, 32, -1, 0, 0, 0)
        struct.pack_into('<5I', metadata, 48, 0, 1, 2, 1, 0)
        symbols = [_symbol('_func', 0, 1, 0x20, 2), _symbol('info', 0, 2, 0, 3),
                   _symbol('unwind', 32, 2, 0, 3), _symbol('trymap', 48, 2, 0, 3),
                   _symbol('catchmap', 72, 2, 0, 3), _symbol('_frame', 0, 0, 0x20, 2)]
        if label:
            symbols.append(_symbol('cleanup', 19 if trailing_label else 2, 1, 0, 6))
        if associative:
            symbols += [_symbol('.text$x', 0, 1, 0, 3, _section_aux(len(code), 2, selection=2)),
                        _symbol('.xdata$x', 0, 2, 0, 3, _section_aux(88, 6, parent=1, selection=5))]
        metadata_refs = () if header_only else ((8, 2, 6), (16, 3, 6), (36, 6 if label else 0, 6),
                                               (44, 0, 6), (64, 4, 6), (84, 0, 6))
        candidate = obj([FixtureSection('.text$x', code, ((9, 1, 6), (14, 5, 20))),
                         FixtureSection(section_name, bytes(metadata), metadata_refs)], symbols).coff
        if associative:
            payload = bytearray(candidate.data)
            struct.pack_into('<I', payload, 56, 0x1020)
            struct.pack_into('<I', payload, 96, 0x1040)
            candidate = CoffObject(bytes(payload))
        candidate.symbols[5] = replace(candidate.symbols[5], name='___CxxFrameHandler')
        retail = fixture()
        struct.pack_into('<I', retail, 0x178+36, 0x60000020)
        put(retail, 0x1000, code[:9]+struct.pack('<I', 0x402000)+b'\xe9'+struct.pack('<i', 0x1100-0x1012))
        for site, address in ((8, 0x402020), (16, 0x402030), (36, 0x401000),
                              (44, 0x401000), (64, 0x402048), (84, 0x401000)):
            if not header_only:
                struct.pack_into('<I', metadata, site, address)
        if label:
            struct.pack_into('<I', metadata, 36, 0x401013 if trailing_label else 0x401002)
        put(retail, 0x2000, metadata)
        return retail, candidate

    def bind(self, retail, candidate, *, claims=None, bindings=()):
        claims = claims if claims is not None else [
            dict(unit='a', symbol='_func', rva=0x1000, evidence='admitted'),
            dict(symbol='___CxxFrameHandler', rva=0x1100, evidence='runtime')]
        claims = [dict(c, evidence=c.get('evidence', 'test admission')) for c in claims]
        rows = candidate_data.inventory('a', candidate, 'sha')
        report = code_data_bindings.bind(Layout(retail), {'a': candidate}, rows, list(bindings),
            dict(units=[dict(unit='a', errors=[], definitions=[], storage_declarations=[])]), claims)
        strict = data_match.compare(Layout(retail), rows, report['source_bindings']+report['data_bindings'], {'a': candidate},
                                    code_claims=claims+report['code_claims'])
        return report, strict

    def test_complete_graph_uses_code_root_and_coff_offsets_without_padding(self):
        retail, candidate = self.fixture()
        report, strict = self.bind(retail, candidate)
        self.assertEqual([(r['kind'], r['size']) for r in report['compiler_records']],
            [('eh-funcinfo', 28), ('eh-unwind-map', 16), ('eh-try-map', 20), ('eh-catch-map', 16)])
        self.assertEqual(strict['summary']['matched_initialized_bytes'], 80)
        self.assertEqual(strict['summary']['enrolled_bytes'], 80)
        self.assertEqual(strict['summary']['static_exact_allocations'], 4)
        self.assertFalse(report['compiler_issues'])
        self.assertEqual({r['extent_kind'] for r in strict['enrollment']}, {'compiler-record'})

    def test_wrong_retail_map_pointer_never_supplies_its_expected_value(self):
        retail, candidate = self.fixture()
        before, _ = self.bind(retail, candidate)
        put(retail, 0x2008, struct.pack('<I', 0x402024))
        after, strict = self.bind(retail, candidate)
        self.assertEqual(before['data_bindings'], after['data_bindings'])
        self.assertEqual(strict['summary']['bytes_by_status']['pointer-mismatch'], 4)
        relocation = next(r for r in strict['relocations'] if r['site_rva'] == 0x2008)
        self.assertEqual(relocation['expected_value'], 0x402020)

    def test_changed_table_entry_remains_enrolled_and_differs(self):
        retail, candidate = self.fixture()
        before, _ = self.bind(retail, candidate)
        put(retail, 0x2028, b'\x01')
        after, strict = self.bind(retail, candidate)
        self.assertEqual(before['data_bindings'], after['data_bindings'])
        self.assertEqual(strict['summary']['bytes_by_status']['fixed-mismatch'], 1)

    def test_missing_or_wrong_runtime_identity_cannot_establish_record_kind(self):
        retail, candidate = self.fixture()
        for claims in ([dict(unit='a', symbol='_func', rva=0x1000)],
                       [dict(unit='a', symbol='_func', rva=0x1000),
                        dict(symbol='___CxxFrameHandler', rva=0x1110)]):
            report, strict = self.bind(retail, candidate, claims=claims)
            self.assertFalse(report['compiler_records'])
            self.assertEqual(strict['summary']['enrolled_bytes'], 0)

    def test_ordinary_data_section_does_not_inherit_eh_contribution_layout(self):
        retail, candidate = self.fixture(section_name='.data')
        report, strict = self.bind(retail, candidate)
        self.assertFalse(report['compiler_records'])
        self.assertEqual(strict['summary']['enrolled_bytes'], 0)

    def test_failed_code_root_cannot_propagate_map_addresses(self):
        retail, candidate = self.fixture()
        put(retail, 0x1000, b'!')
        report, strict = self.bind(retail, candidate)
        self.assertFalse(report['compiler_records'])
        self.assertEqual(strict['summary']['enrolled_bytes'], 0)

    def test_opcode_bytes_inside_another_instruction_cannot_establish_eh_root(self):
        retail, candidate = self.fixture(bad_boundary=True)
        report, strict = self.bind(retail, candidate)
        self.assertFalse(report['compiler_records'])
        self.assertEqual(strict['summary']['enrolled_bytes'], 0)

    def test_conflicting_source_root_cannot_launder_child_bindings(self):
        retail, candidate = self.fixture()
        prior = dict(id=100, name='info', candidate_ids=['a:2:0'], size=28, rva=0x2010,
                     status='bound', macro='DATA', literal_sha256='', source_identity='info')
        report, _ = self.bind(retail, candidate, bindings=[prior])
        self.assertFalse(report['compiler_records'])
        self.assertEqual(report['data_bindings'][0]['status'], 'conflicting-source-address')

    def test_bad_version_retains_header_comparison_and_explicit_issue(self):
        retail, candidate = self.fixture(magic=0x19930521)
        put(retail, 0x2000, struct.pack('<I', 0x19930520))
        report, strict = self.bind(retail, candidate)
        self.assertEqual(strict['summary']['enrolled_bytes'], 28)
        self.assertEqual(strict['summary']['bytes_by_status']['fixed-mismatch'], 1)
        self.assertTrue(report['compiler_issues'])

    def test_unsupported_schema_cannot_pass_exact_gate_when_known_prefix_matches(self):
        retail, candidate = self.fixture(magic=0x19930521, header_only=True)
        report, strict = self.bind(retail, candidate)
        self.assertEqual(strict['summary']['static_exact_allocations'], 1)
        self.assertEqual(strict['summary']['bytes_by_status']['fixed-match'], 28)
        strict['summary']['code_data_bindings'] = report['summary']
        self.assertFalse(data_match.exact(strict))

    def test_map_count_cannot_consume_neighboring_allocation(self):
        retail, candidate = self.fixture(states=3)
        report, strict = self.bind(retail, candidate)
        self.assertEqual(strict['summary']['enrolled_bytes'], 28)
        self.assertIn('typed extent', report['compiler_issues'][0]['detail'])

    def test_interior_map_pointer_cannot_create_an_allocation(self):
        retail, candidate = self.fixture(map_addend=4)
        report, strict = self.bind(retail, candidate)
        self.assertEqual(strict['summary']['enrolled_bytes'], 28)
        self.assertIn('emitted owner', report['compiler_issues'][0]['detail'])

    def test_external_or_cross_contribution_map_is_unproved(self):
        for section, storage in ((0, 2), (1, 3), (2, 2)):
            retail, candidate = self.fixture()
            candidate.symbols[2] = replace(candidate.symbols[2], section=section, storage_class=storage)
            report, _ = self.bind(retail, candidate)
            self.assertTrue(report['compiler_issues'])
            self.assertFalse(any(r['kind'] == 'eh-unwind-map' for r in report['compiler_records']))

    def test_local_cleanup_label_offset_is_anchored_only_by_checked_code(self):
        retail, candidate = self.fixture(label=True)
        _, strict = self.bind(retail, candidate)
        self.assertEqual(strict['summary']['matched_initialized_bytes'], 80)
        put(retail, 0x2024, struct.pack('<I', 0x401003))
        _, strict = self.bind(retail, candidate)
        self.assertEqual(strict['summary']['bytes_by_status']['pointer-mismatch'], 4)

    def test_label_in_excluded_code_alignment_is_not_a_checked_code_identity(self):
        retail, candidate = self.fixture(label=True, trailing_label=True)
        report, strict = self.bind(retail, candidate)
        self.assertFalse(any(c['symbol'] == 'cleanup' for c in report['code_claims']))
        self.assertEqual(strict['summary']['bytes_by_status']['pointer-unresolved'], 4)

    def test_associative_copies_share_parent_identity_but_every_copy_is_compared(self):
        retail, first = self.fixture(associative=True)
        _, second = self.fixture(associative=True)
        for mutate in (False, True):
            if mutate:
                payload = bytearray(second.data)
                payload[second.sections[1].raw_offset+40] = 1
                second = CoffObject(bytes(payload))
                second.symbols[5] = replace(second.symbols[5], name='___CxxFrameHandler')
            objects = dict(a=first, b=second)
            rows = [r for unit, obj_ in objects.items() for r in candidate_data.inventory(unit, obj_, 'sha')]
            claims = [dict(unit=u, symbol='_func', rva=0x1000, evidence='admitted') for u in objects]
            claims.append(dict(symbol='___CxxFrameHandler', rva=0x1100, evidence='runtime'))
            declared = dict(units=[dict(unit=u, errors=[], definitions=[], storage_declarations=[]) for u in objects])
            report = code_data_bindings.bind(Layout(retail), objects, rows, [], declared, claims)
            strict = data_match.compare(Layout(retail), rows, report['data_bindings'], objects, code_claims=claims)
            self.assertEqual(strict['summary']['enrolled_bytes'], 80)
            self.assertEqual(strict['summary']['bytes_by_status'].get('binding-conflict', 0), 0)
            self.assertEqual(strict['summary']['bytes_by_status'].get('fixed-mismatch', 0), int(mutate))

    def test_associative_identity_requires_a_valid_named_parent_selection(self):
        _, candidate = self.fixture(associative=True)
        self.assertTrue(compiler_eh.associative_identity(candidate, candidate.sections[1]))
        parent_header = next(s for s in candidate.symbols.values() if s.section == 1 and s.aux_count)
        for selection in (1, 5, 99):
            payload = bytearray(candidate.data)
            payload[parent_header.offset+18+14] = selection
            changed = CoffObject(bytes(payload))
            self.assertFalse(compiler_eh.associative_identity(changed, changed.sections[1]))
        candidate.symbols[0] = replace(candidate.symbols[0], value=1)
        self.assertFalse(compiler_eh.associative_identity(candidate, candidate.sections[1]))
        _, ordinary = self.fixture()
        self.assertFalse(compiler_eh.associative_identity(ordinary, ordinary.sections[1]))


if __name__ == '__main__':
    unittest.main()
