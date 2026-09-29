import struct
from types import SimpleNamespace
import unittest

from homm3.verify.source_initializers import Owner, Pattern, match_body, verified_roots


class SourceInitializerTests(unittest.TestCase):
    owner = Owner('include/example.h', 7, 'mask', '_mask', 4, 'bitset<10>')
    pattern = Pattern(owner, b'\xe8'+bytes(4)+b'\xa3'+bytes(4)+b'\xc3',
                      ((1, 'callee', 20), (6, '_mask', 6)))

    def match(self, *, callee=0x200, destination=0x300, sites=(0x106,),
              payload=bytes(4), change=None):
        body = bytearray(self.pattern.body)
        struct.pack_into('<i', body, 1, callee-0x105)
        struct.pack_into('<I', body, 6, 0x400000+destination)
        if change is not None:
            body[change] ^= 1
        image = SimpleNamespace(image_base=0x400000,
            relocs_in=lambda lo, hi: sites, payload=lambda r, n: payload,
            pe=SimpleNamespace(sections=[dict(name='.data', va=0x300, vsize=16)]))
        return match_body(self.pattern, 0x100, bytes(body), image, {'callee': {0x200}})

    def test_all_instructions_and_named_call_are_compared(self):
        self.assertEqual(self.match(), 0x300)
        self.assertIsNone(self.match(callee=0x204))
        self.assertIsNone(self.match(change=10))

    def test_missing_extra_relocation_and_bad_destination_rejected(self):
        for kwargs in (dict(sites=()), dict(sites=(0x101, 0x106)),
                       dict(destination=0x301), dict(destination=0x400),
                       dict(payload=b'abcd')):
            with self.subTest(kwargs=kwargs):
                self.assertIsNone(self.match(**kwargs))

    def test_every_crt_slot_verified_and_table_unique(self):
        rows = [dict(slot=str(i), rva=hex(0x100+i*16)) for i in range(4)]
        table = b''.join(struct.pack('<I', 0x400100+i*16) for i in range(4))
        def roots(data):
            return verified_roots(SimpleNamespace(data=data, image_base=0x400000,
                sections=[dict(name='.rdata', rptr=0, rsize=len(data))]), rows)
        self.assertEqual(roots(table), [0x100, 0x110, 0x120, 0x130])
        rows.append(dict(slot='-', rva='0x200'))  # reviewed atexit cleanup
        self.assertEqual(roots(table), [0x100, 0x110, 0x120, 0x130])
        self.assertFalse(roots(table+table))
        self.assertFalse(roots(table[:-1]+b'\x01'))

    def test_verified_store_exposes_conflicting_source_extent(self):
        from dataclasses import asdict
        from homm3.verify.byte_accounting import Range, initializer_ranges, partition
        row = dict(rva=0x100, size=11, destination=0x300, owner=asdict(self.owner))
        ranges = initializer_ranges({'matches': [row]})
        ranges.append(Range(0x2f8, 0x308, 'game', 'oversized buffer', 2))
        result = partition(0x400, ranges)
        conflicts = [r for r in result if r['category'] == 'overlap']
        self.assertEqual([(r['start'], r['size']) for r in conflicts], [(0x300, 4)])
        self.assertEqual(sum(r['size'] for r in result
                             if r['category'] == 'source-initializer-exact'), 11)

    def test_candidate_requires_real_crt_root_store_and_zero_addends(self):
        from dataclasses import replace
        from pathlib import Path
        from tempfile import TemporaryDirectory
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.delink.coffx import Obj
        from homm3.verify.source_initializers import candidate_patterns
        def patterns(*, body=self.pattern.body, rel=((1, 2, 20), (6, 1, 6)),
                     root_kind=6, store_size=4):
            raw = bytearray(_coff((
                FixtureSection('.text', body+b'\x90'*5, rel),
                FixtureSection('.bss', bytes(store_size), ()),
                FixtureSection('.CRT$XCU', bytes(4), ((0, 0, root_kind),))),
                (_symbol('init', 0, 1, 0x20, 2),
                 _symbol('_mask', 0, 2, 0, 3),
                 _symbol('callee', 0, 0, 0, 2))))
            struct.pack_into('<I', raw, 20+40+36, 0x80)
            with TemporaryDirectory() as tmp:
                path = Path(tmp, 'test.obj'); path.write_bytes(raw)
                return candidate_patterns(Obj(path), {'_mask': self.owner})
        self.assertEqual(patterns(), {replace(self.pattern, padding=b'\x90'*5)})
        self.assertFalse(patterns(root_kind=20))
        self.assertFalse(patterns(store_size=2))
        self.assertFalse(patterns(body=self.pattern.body[:6]+b'\x01'+self.pattern.body[7:]))
        self.assertFalse(patterns(rel=((1, 2, 6), (6, 1, 6))))
        # Loading a datum is not its initializer's defining store.
        self.assertFalse(patterns(body=self.pattern.body[:5]+b'\xa1'+self.pattern.body[6:]))

    def test_padding_needs_compiler_bytes_and_next_boundary(self):
        from dataclasses import replace
        from homm3.verify.source_initializers import matched_padding
        pattern = replace(self.pattern, padding=b'\x90'*5, alignment=16)
        image = SimpleNamespace(relocs_in=lambda a, b: [],
                                pe=SimpleNamespace(read=lambda a, n: b'\x90'*n))
        self.assertEqual(matched_padding(pattern, 0x100, image, {0x110}), 5)
        self.assertFalse(matched_padding(pattern, 0x100, image, {0x111}))
        self.assertFalse(matched_padding(pattern, 0x100, image, {0x10d, 0x110}))
        image.pe.read = lambda a, n: bytes(n)
        self.assertFalse(matched_padding(pattern, 0x100, image, {0x110}))


if __name__ == '__main__':
    unittest.main()
