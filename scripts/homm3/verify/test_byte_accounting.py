import unittest

from homm3.verify.byte_accounting import Range, partition


class ByteAccountingTests(unittest.TestCase):
    def test_frontiers_and_holes_are_not_skipped(self):
        rows = partition(20, [Range(4, 8, 'game', 'a'), Range(10, 15, 'game', 'b')])
        self.assertEqual([(r['start'], r['end']) for r in rows if r['category']=='missing'],
                         [(0, 4), (8, 10), (15, 20)])
        self.assertEqual(sum(r['size'] for r in rows), 20)

    def test_nested_overlaps_are_not_hidden_by_short_neighbor(self):
        rows = partition(20, [Range(1, 19, 'game', 'long'),
                              Range(3, 5, 'game', 'short'),
                              Range(7, 9, 'game', 'later')])
        self.assertEqual([(r['start'], r['end']) for r in rows if r['category']=='overlap'],
                         [(3, 5), (7, 9)])

    def test_folded_copies_count_once(self):
        r = Range(2, 6, 'game', 'fold')
        self.assertEqual(partition(8, [r, r]), partition(8, [r]))

    def test_section_container_is_not_an_overlap_with_its_datum(self):
        rows = partition(12, [Range(2, 10, 'section', '.rdata', 1),
                              Range(4, 8, 'game', 'vtable', 2)])
        self.assertNotIn('overlap', [r['category'] for r in rows])
        self.assertEqual(sum(r['size'] for r in rows if r['category']=='section'), 4)

    def test_outside_domain_is_an_error(self):
        with self.assertRaises(ValueError):
            partition(10, [Range(2, 11, 'game', 'bad')])

    def test_vendor_data_manifest_is_the_same_owner_as_source_map(self):
        from homm3.model import Binding, Model
        from homm3.verify.byte_accounting import model_ranges
        model = Model([], [Binding(16, 16, '', 'data', '_table', 'crc32',
                                   'data_zlib', ())], [])
        rows = model_ranges(model, [dict(rva='0x10', size='16', name='_table',
                                        provenance='zlib-source-sizeof')], [])
        self.assertEqual(rows, [Range(16, 32, 'library-vendor', '_table', 2)])



class InitializerTests(unittest.TestCase):
    def compare(self, payload, retail, *, relocation=None, target=None, manifest_ordinal='1',
                name='datum', emitted_names=('datum',)):
        from pathlib import Path
        from tempfile import TemporaryDirectory
        from types import SimpleNamespace
        from unittest.mock import patch
        import struct
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.verify.byte_accounting import compare_initializers
        from homm3.model import Binding, Model
        image = SimpleNamespace(relocs_in=lambda lo, hi: [0x100] if relocation else [])
        pe = SimpleNamespace(image_base=0x400000, read=lambda start, size: retail[:size])
        # Section 1 is code. Data is section 2, despite target manifest ordinal 1.
        symbols = tuple(_symbol(f'datum{i}', 0, 2, 0, 2)
                        for i, _ in enumerate(emitted_names)) + (
            _symbol('referent', 0, 0, 0, 2),)
        relocs = ((0, len(emitted_names), 6),) if relocation else ()
        obj = bytearray(_coff((FixtureSection('.text', b'\xc3', ()),
                              FixtureSection('.data', payload, relocs)), symbols))
        symptr = struct.unpack_from('<I', obj, 8)[0]
        strings = bytearray(4)
        for i, emitted in enumerate(emitted_names):
            struct.pack_into('<II', obj, symptr + i * 18, 0, len(strings))
            strings.extend(emitted.encode('ascii') + b'\0')
        struct.pack_into('<I', strings, 0, len(strings))
        obj[-4:] = strings
        model = Model([], [] if target is None else [
            Binding(target, 4, '', 'data', 'referent', 'test', 'src', ())], [])
        row = dict(object='test.c', name=name, rva='0x100', size=hex(len(retail)),
                   storage='data', section_ordinal=manifest_ordinal, section_offset='0x0')
        with TemporaryDirectory() as tmp, patch('homm3.delink.image.Image', return_value=image):
            Path(tmp, 'test.obj').write_bytes(obj)
            return compare_initializers(model, [row], pe, Path(tmp))[0]

    def test_candidate_name_selects_section_not_target_ordinal(self):
        self.assertEqual(self.compare(b'abc\0', b'abc\0')['verdict'], 'exact')

    def test_incomplete_initializer_is_visible(self):
        result = self.compare(b'a\0\0\0', b'ab\0\0')
        self.assertEqual((result['verdict'], result['different']), ('mismatch', 1))

    def test_pointer_addend_is_compared(self):
        import struct
        result = self.compare(struct.pack('<I', 4), struct.pack('<I', 0x402008),
                              relocation=True, target=0x2000)
        self.assertEqual(result['verdict'], 'mismatch')
        exact = self.compare(struct.pack('<I', 8), struct.pack('<I', 0x402008),
                             relocation=True, target=0x2000)
        self.assertEqual(exact['verdict'], 'exact')

    def test_unknown_pointer_is_not_exact(self):
        result = self.compare(bytes(4), bytes(4), relocation=True)
        self.assertEqual((result['verdict'], result['unresolved']), ('unresolved', 4))

    def test_anonymous_namespace_data_requires_unique_source_identity(self):
        clang = '?values@?A0xABCD@@3PAHA'
        emitted = r'?values@?%Z:\project\src\test.cpp123@@3PAHA'
        self.assertEqual(self.compare(b'abcd', b'abcd', name=clang,
                                      emitted_names=(emitted,))['verdict'], 'exact')
        self.assertEqual(self.compare(b'abcd', b'abce', name=clang,
                                      emitted_names=(emitted,))['verdict'], 'mismatch')
        for candidates in ((emitted.replace('test.cpp', 'other.cpp'),),
                           (emitted.replace('PAHA', 'PANA'),),
                           (emitted, emitted.replace('123', '456'))):
            with self.subTest(candidates=candidates):
                self.assertEqual(self.compare(b'abcd', b'abcd', name=clang,
                                              emitted_names=candidates)['verdict'],
                                 'unavailable')


class CompilerOwnershipTests(unittest.TestCase):
    def ranges(self, *, reviewed=((0x200, 0x100), (0x400, 0x100)),
               sizes=((0x200, 3), (0x400, 5)), claimed=True, embedded_push=False):
        import struct
        from types import SimpleNamespace
        from unittest.mock import patch
        from homm3.model import Binding, Model
        from homm3.verify.byte_accounting import compiler_ranges
        from homm3.delink.eh_band import Group
        push = b'\x68' + struct.pack('<I', 0x400500)
        body = (b'\xb8' + push if embedded_push else push) + b'\xc3'
        pe = SimpleNamespace(image_base=0x400000, path='unused',
                             section=lambda _: {'va': 0x100, 'vsize': 0x500},
                             read=lambda _a, _n: body)
        parent = Binding(0x100, len(body), '', 'code', 'owner', 'unit', 'src', ())
        model = Model([parent] if claimed else [], [], [])
        group = Group(0x100, 'owner', 'unit', (0x200, 0x400), 0x500)
        with patch('homm3.delink.eh_band.groups', return_value=[group]) as decode, \
             patch('homm3.retail_labels.censuses.functions',
                   return_value=[{'rva': a, 'size': n} for a, n in sizes]), \
             patch('homm3.verify.byte_accounting.read', return_value=([], [], [
                 {'rva': hex(a), 'parent_rva': hex(p)} for a, p in reviewed])):
            result = compiler_ranges(pe, model)
        self.assertFalse(decode.call_args.kwargs['contiguous'])
        return result

    def test_discontiguous_cleanup_uses_individual_reviewed_extents(self):
        self.assertEqual([(r.start, r.end) for r in self.ranges()],
                         [(0x500, 0x50a), (0x200, 0x203), (0x400, 0x405)])

    def test_missing_size_or_disagreeing_parent_keeps_cleanup_unclaimed(self):
        result = self.ranges(reviewed=((0x200, 0x99), (0x400, 0x100)),
                             sizes=((0x200, 3),))
        self.assertEqual([(r.start, r.end) for r in result], [(0x500, 0x50a)])

    def test_unclaimed_parent_cannot_own_cleanup(self):
        self.assertFalse(self.ranges(claimed=False))

    def test_push_bytes_inside_another_instruction_are_not_an_owner(self):
        self.assertFalse(self.ranges(embedded_push=True))

    def test_explicit_body_claim_takes_priority_over_generated_attribution(self):
        ranges = self.ranges()
        ranges.append(Range(0x200, 0x203, 'game', 'explicit source body', 2))
        rows = partition(0x600, ranges)
        self.assertFalse(any(r['category'] == 'overlap' for r in rows))
        self.assertEqual(next(r for r in rows if r['start'] == 0x200)['category'], 'game')


if __name__ == "__main__":
    unittest.main()
