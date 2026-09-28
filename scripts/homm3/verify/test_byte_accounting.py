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



class InitializerTests(unittest.TestCase):
    def compare(self, payload, retail, *, relocation=None, target=None, manifest_ordinal='1'):
        from pathlib import Path
        from tempfile import TemporaryDirectory
        from types import SimpleNamespace
        from unittest.mock import patch
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.verify.byte_accounting import compare_initializers
        from homm3.model import Binding, Model
        image = SimpleNamespace(relocs_in=lambda lo, hi: [0x100] if relocation else [])
        pe = SimpleNamespace(image_base=0x400000, read=lambda start, size: retail[:size])
        # Section 1 is code. Data is section 2, despite target manifest ordinal 1.
        symbols = (_symbol('datum', 0, 2, 0, 2), _symbol('referent', 0, 0, 0, 2))
        relocs = ((0, 1, 6),) if relocation else ()
        obj = _coff((FixtureSection('.text', b'\xc3', ()),
                     FixtureSection('.data', payload, relocs)), symbols)
        model = Model([], [] if target is None else [
            Binding(target, 4, '', 'data', 'referent', 'test', 'src', ())], [])
        row = dict(object='test.c', name='datum', rva='0x100', size=hex(len(retail)),
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

if __name__ == "__main__":
    unittest.main()
