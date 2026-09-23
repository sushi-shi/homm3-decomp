"""Wrong referents, masks, partial evidence and duplicate copies cannot pass."""
from dataclasses import replace
import struct
import unittest
import tempfile
import hashlib
from pathlib import Path

from homm3.analysis.candidate_data import inventory
from homm3.analysis.test_candidate_data import coff
from homm3.build.test_eh_handler_normalization import _symbol, _coff, _section_aux, FixtureSection
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.sema import data_match as data
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


def binding(row, rva, size, **extra):
    return dict(dict(id=row['id'], name=row['symbols'][0] if row['symbols'] else 'anonymous',
                     candidate_ids=[row['id']], status='bound', rva=rva, size=size,
                     macro='DATA', literal_sha256='', source_identity=row['symbols'][0] if row['symbols'] else ''), **extra)


class DataMatchTest(unittest.TestCase):
    def setUp(self):
        self.image = fixture()
        self.layout = Layout(self.image)

    def put(self, rva, blob):
        start = self.layout.raw(rva, len(blob))
        self.image[start:start+len(blob)] = blob
        self.layout = Layout(self.image)

    def compare(self, obj, bindings=None, **kwargs):
        rows = inventory('a', obj, 'sha')
        bindings = bindings if bindings is not None else [binding(rows[0], 0x2000, 8)]
        return data.compare(self.layout, rows, bindings, {'a': obj}, **kwargs)

    def pointers(self, addend=0, target_index=1):
        obj = coff(raw=struct.pack('<I', addend)+b'abcdEFGH',
                   symbols=[_symbol('_ptr', 0, 1, 0, 2), _symbol('_array', 4, 1, 0, 2),
                            _symbol('_other', 8, 1, 0, 2)], relocations=((0, target_index, 6),))
        rows = inventory('a', obj, 'sha')
        bindings = [binding(r, va, 4) for r, va in zip(rows, (0x2000, 0x2010, 0x2020))]
        self.put(0x2000, struct.pack('<I', 0x402010))
        self.put(0x2010, b'abcd')
        self.put(0x2020, b'EFGH')
        return obj, bindings

    def test_every_byte_has_verdict_and_changed_initializer_fails(self):
        self.put(0x2000, b'abcdefgh')
        report = self.compare(coff())
        self.assertEqual(report['matches'][0]['status'], 'static-exact')
        self.assertEqual(report['summary']['matched_initialized_bytes'], 8)
        self.assertEqual(sum(r['size'] for r in report['byte_verdicts']), report['summary']['total_bytes'])
        self.put(0x2003, b'!')
        report = self.compare(coff())
        self.assertEqual(report['matches'][0]['bytes_by_status']['fixed-mismatch'], 1)
        self.assertEqual(report['matches'][0]['status'], 'not-exact')

    def test_provenance_basename_collision_does_not_replace_selected_object(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, private = root/'source/a.obj', root/'private/a.obj'
            source.parent.mkdir()
            private.parent.mkdir()
            source.write_bytes(coff(raw=b'original').data)
            private.write_bytes(coff(raw=b'changed!').data)
            hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in (source, private)}
            result = data.load_objects(root, hashes, {'a': str(source), 'vendor': str(private)})
            self.assertEqual(result['a'].data, source.read_bytes())
            self.assertEqual(result['vendor'].data, private.read_bytes())
            with self.assertRaisesRegex(ValueError, 'ambiguous raw-object basename'):
                data.load_objects(root, hashes)
            private.write_bytes(b'tampered')
            with self.assertRaisesRegex(ValueError, 'candidate evidence changed'):
                data.load_objects(root, hashes, {'a': str(source)})
            with self.assertRaisesRegex(ValueError, 'missing from provenance'):
                data.load_objects(root, hashes, {'a': 'missing.obj'})

    def test_right_pointer_requires_independent_owner_and_addend(self):
        obj, bindings = self.pointers()
        report = self.compare(obj, bindings)
        self.assertEqual(report['relocations'][0]['status'], 'pointer-match')
        for obj, bindings in [self.pointers(addend=1), self.pointers(target_index=2)]:
            report = self.compare(obj, bindings)
            self.assertEqual(report['relocations'][0]['status'], 'pointer-mismatch')

    def test_pointer_field_cannot_anchor_its_own_unknown_destination(self):
        obj, bindings = self.pointers()
        report = self.compare(obj, bindings[:1])
        self.assertEqual(report['relocations'][0]['status'], 'pointer-unresolved')
        self.assertEqual(report['summary']['matched_initialized_bytes'], 0)

    def test_interior_and_one_past_supported_but_out_of_object_not_extrapolated(self):
        for addend, expected in [(2, 'pointer-match'), (4, 'pointer-match'), (5, 'pointer-unresolved')]:
            obj, bindings = self.pointers(addend=addend)
            # Drop the physically adjacent allocation: adjacency cannot provide
            # an accidental anchor for an out-of-object pointer.
            self.put(0x2000, struct.pack('<I', 0x402010+addend))
            report = self.compare(obj, bindings[:2])
            self.assertEqual(report['relocations'][0]['status'], expected)

    def test_code_entries_do_not_prove_nonzero_interior_offsets(self):
        for addend, verdict in [(0, 'pointer-match'), (1, 'pointer-unresolved')]:
            obj = coff(raw=struct.pack('<II', addend, 0),
                       symbols=[_symbol('_ptr', 0, 1, 0, 2), _symbol('_fn', 0, 0, 0x20, 2)],
                       relocations=((0, 1, 6),))
            self.put(0x2000, struct.pack('<II', 0x401000+addend, 0))
            report = self.compare(obj, code_claims=[dict(symbol='_fn', rva=0x1000, evidence='admitted')])
            self.assertEqual(report['relocations'][0]['status'], verdict)

    def test_local_code_identity_cannot_resolve_another_units_external_pointer(self):
        local = CoffObject(_coff((FixtureSection('.text', b'abcdefgh', ()),),
                                (_symbol('_fn', 0, 1, 0x20, 3),)))
        user = coff(raw=bytes(8), symbols=[_symbol('_ptr', 0, 1, 0, 2),
                    _symbol('_fn', 0, 0, 0x20, 2)], relocations=((0, 1, 6),))
        rows = inventory('user', user, 'sha')
        claims = [dict(unit='local', symbol='_fn', rva=0x1000, evidence='local annotation', linkage='INTERNAL')]
        identities = data.Identities(rows, [], {'local': local, 'user': user}, claims)
        self.assertFalse(identities.resolve('user', user.symbols[1], 0))
        self.assertEqual(identities.resolve('local', local.symbols[0], 0)[0]['target_rva'], 0x1000)

    def test_checked_local_labels_keep_their_exact_symbol_index(self):
        obj = CoffObject(_coff((FixtureSection('.text$a', b'abcdefgh', ()),
                               FixtureSection('.text$b', b'ijklmnop', ())),
                              (_symbol('label', 2, 1, 0, 6), _symbol('label', 2, 2, 0, 6))))
        claims = [dict(unit='a', symbol='label', symbol_index=0, rva=0x1002,
                       evidence='checked code offset', linkage='INTERNAL')]
        identities = data.Identities([], [], {'a': obj}, claims)
        self.assertEqual(identities.resolve('a', obj.symbols[0], 0)[0]['target_rva'], 0x1002)
        self.assertFalse(identities.resolve('a', obj.symbols[1], 0))
        self.assertFalse(identities.resolve('a', obj.symbols[0], 1))

    def test_missing_candidate_relocation_cannot_pass_equal_numeric_word(self):
        raw = struct.pack('<II', 0x402010, 0)
        self.put(0x2000, raw)
        report = self.compare(coff(raw=raw), retail_pointers=[0x2000])
        self.assertEqual(report['matches'][0]['bytes_by_status'], {'missing-relocation': 4, 'fixed-match': 4})

    def test_unresolved_and_unsupported_relocations_are_not_masks(self):
        obj, bindings = self.pointers()
        obj.relocations = (replace(obj.relocations[0], typ=0x14),)
        report = self.compare(obj, bindings)
        self.assertEqual(report['relocations'][0]['status'], 'unsupported-relocation')
        self.assertEqual(report['matches'][0]['status'], 'not-exact')

    def test_partial_relocation_across_symbol_boundary_marks_both_parts(self):
        obj = coff(raw=bytes(8), symbols=[_symbol('_a', 0, 1, 0, 2),
                   _symbol('_b', 2, 1, 0, 2), _symbol('_dest', 0, 0, 0, 2)], relocations=((0, 2, 6),))
        rows = inventory('a', obj, 'sha')
        bindings = [binding(rows[0], 0x2000, 2), binding(rows[1], 0x2002, 6)]
        report = self.compare(obj, bindings)
        self.assertEqual(report['matches'][0]['bytes_by_status'], {'unsupported-relocation': 2})
        self.assertEqual(report['matches'][1]['bytes_by_status']['unsupported-relocation'], 2)

    def test_overlapping_candidate_relocations_withhold_entire_fields(self):
        obj = coff(raw=bytes(8), symbols=[_symbol('_a', 0, 1, 0, 2), _symbol('_fn', 0, 0, 0x20, 2)],
                   relocations=((0, 1, 6), (2, 1, 6)))
        report = self.compare(obj, code_claims=[dict(symbol='_fn', rva=0x1000, evidence='admitted')])
        self.assertEqual(report['matches'][0]['bytes_by_status']['unsupported-relocation'], 6)

    def test_zero_fill_agreement_is_separate_from_initialized_data_matching(self):
        obj = coff(raw=bytes(8), section='.bss', flags=0xC0300080)
        report = self.compare(obj, [binding(inventory('a', obj, 'sha')[0], 0x3000, 8)])
        self.assertEqual(report['summary']['zero_fill_agreement_bytes'], 8)
        self.assertEqual(report['summary']['matched_initialized_bytes'], 0)
        self.assertEqual(report['matches'][0]['initialization'], 'not-verified')

    def test_conflicting_projected_storage_cannot_credit_overlapping_bytes(self):
        obj, bindings = self.pointers()
        bindings[1]['rva'] = 0x2002
        report = self.compare(obj, bindings)
        self.assertEqual(report['summary']['bytes_by_status']['binding-conflict'], 6)

    def test_good_folded_copy_cannot_hide_a_bad_copy(self):
        self.put(0x2000, b'abcdefgh')
        a, b = coff(), coff(raw=b'abc!efgh')
        rows = inventory('a', a, 'sha')+inventory('b', b, 'sha')
        report = data.compare(self.layout, rows, [binding(r, 0x2000, 8) for r in rows], {'a': a, 'b': b})
        self.assertEqual(report['summary']['bytes_by_status']['fixed-mismatch'], 1)
        self.assertEqual(report['summary']['matched_initialized_bytes'], 7)
        self.assertEqual(report['summary']['enrolled_bytes'], 8)

    def test_folded_literal_identity_requires_same_payload_and_admitted_comdat_selection(self):
        def object_(payload, selection):
            raw = bytearray(_coff((FixtureSection('.data', bytes(4), ((0, 3, 6),)),
                                  FixtureSection('.data', payload, ())),
                                 (_symbol('_ptr', 0, 1, 0, 2),
                                  _symbol('.data', 0, 2, 0, 3, _section_aux(4, 0, selection=selection)),
                                  _symbol('_literal', 0, 2, 0, 2))))
            struct.pack_into('<I', raw, 56, 0xC0300040)
            struct.pack_into('<I', raw, 96, 0xC0301040)
            obj = CoffObject(bytes(raw))
            obj.symbols[3] = replace(obj.symbols[3], name='??_C@_0synthetic_literal')
            return obj
        self.put(0x2000, struct.pack('<I', 0x402020))
        self.put(0x2020, b'abc\0')
        for payload, selection, expected in [(b'abc\0', 2, 'pointer-match'),
                                              (b'bad\0', 2, 'pointer-unresolved'),
                                              (b'abc\0', 1, 'pointer-unresolved')]:
            user, owner = object_(payload, selection), object_(b'abc\0', 2)
            user_rows, owner_rows = inventory('user', user, 'sha'), inventory('owner', owner, 'sha')
            bindings = [binding(user_rows[0], 0x2000, 4), binding(owner_rows[1], 0x2020, 4)]
            report = data.compare(self.layout, user_rows+owner_rows, bindings, {'user': user, 'owner': owner})
            self.assertEqual(report['relocations'][0]['status'], expected)

    def test_accounting_overlay_preserves_denominators_and_point_metadata(self):
        from homm3.sema.image_coverage import partition, audit_partition
        self.put(0x2000, b'abc!efgh')
        report = self.compare(coff())
        rows = partition(self.layout, [], {0x2000: [dict(name='table', source='source')]}, [])
        for row in rows:
            row['data_accounting_status'] = 'unresolved'
        result, summary = data.overlay(self.layout, rows, report)
        for domain, total in [('image', self.layout.image_size), ('file', len(self.image))]:
            audit_partition(result, domain, total)
        image = [r for r in result if r['domain'] == 'image']
        self.assertEqual(sum(r['size'] for r in image if r['data_match_status'] == 'fixed-match'), 7)
        self.assertEqual(sum(bool(r['labels']) for r in image), 1)
        self.assertEqual(summary['additional_accounted_bytes'], 7)
        for row in result:
            if row['rva'] == 0x2004:
                self.assertEqual(row['preview_hex'], b'efgh'.hex())


if __name__ == '__main__':
    unittest.main()
