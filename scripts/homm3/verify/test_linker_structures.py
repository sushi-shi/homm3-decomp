"""Import tables, .CRT$XCU slots and data section tails."""

import struct
import unittest

from homm3.verify import linker_structures as linker
from homm3.verify.library_code import common_alignment


class FakePe:
    """Identity-mapped image: RVA == file offset."""
    image_base = 0x400000

    def __init__(self, size=0x3000, sections=()):
        self.data = bytearray(size)
        struct.pack_into('<I', self.data, 0x3c, 0x40)
        self.sections = list(sections)

    def read(self, rva, size):
        chunk = bytes(self.data[rva:rva + size])
        return chunk if len(chunk) == size else None


def _image(hint=7, name=b'Beep', dll=b'KERNEL32.dll', stamp=0):
    pe = FakePe()
    header = 0x40 + 24 + 96
    struct.pack_into('<II', pe.data, header + 8 * 1, 0x1000, 40)     # import directory
    struct.pack_into('<II', pe.data, header + 8 * 12, 0x800, 8)      # IAT directory
    # descriptor + null descriptor
    struct.pack_into('<IIIII', pe.data, 0x1000, 0x1028, stamp, 0, 0x1038, 0x800)
    struct.pack_into('<II', pe.data, 0x1028, 0x1030, 0)              # lookup table
    struct.pack_into('<II', pe.data, 0x800, 0x1030, 0)               # IAT
    record = struct.pack('<H', hint) + name + b'\0'
    pe.data[0x1030:0x1030 + len(record)] = record
    pe.data[0x1038:0x1038 + len(dll) + 1] = dll + b'\0'
    return pe


PINNED = {('kernel32.dll', 'Beep'): ('KERNEL32.dll', 7, 'KERNEL32.LIB')}


class ImportTests(unittest.TestCase):
    def test_exact_tables_tile_the_region(self):
        records = linker.import_records(_image(), PINNED, referenced={0x800})
        self.assertTrue(all(r.category == 'linker-import' for r in records), records)
        self.assertEqual({r.kind for r in records},
                         {'descriptor', 'null-descriptor', 'lookup', 'iat', 'hint-name',
                          'dll-name', 'pad'})
        pad = next(r for r in records if r.kind == 'pad')
        self.assertEqual((pad.start, pad.size), (0x1037, 1))

    def test_every_field_the_library_cannot_reproduce_is_reported(self):
        cases = [(_image(hint=8), 'hint-name'),
                 (_image(dll=b'KeRNeL32.dll'), 'dll-name'),
                 (_image(stamp=0xad2b0000), 'descriptor')]
        for pe, kind in cases:
            records = linker.import_records(pe, PINNED, referenced={0x800})
            bad = [r for r in records if r.reasons]
            self.assertEqual([r.kind for r in bad], [kind])
        records = linker.import_records(_image(), PINNED, referenced=set())
        self.assertEqual([r.kind for r in records if r.reasons], ['iat'])
        records = linker.import_records(_image(), {}, referenced={0x800})
        self.assertEqual(sorted(r.kind for r in records if r.reasons), ['dll-name', 'hint-name'])

    def test_bound_iat_is_reported(self):
        pe = _image()
        struct.pack_into('<I', pe.data, 0x800, 0x77001234)
        records = linker.import_records(pe, PINNED, referenced={0x800})
        self.assertIn('iat', [r.kind for r in records if r.reasons])


class CrtSlotTests(unittest.TestCase):
    def test_slots_between_markers_need_an_exact_body_and_block_order(self):
        pe = FakePe()
        base = pe.image_base
        # ___xc_a at 0x100, three game slots, one library slot, ___xc_z at 0x114
        words = [0x2000, 0x2100, 0x2200, 0x2300]
        for i, w in enumerate(words):
            struct.pack_into('<I', pe.data, 0x104 + 4 * i, base + w)
        symbols = {0x100: {'___xc_a'}, 0x114: {'___xc_z'}}
        library = [(0x100, 0x104), (0x110, 0x114), (0x114, 0x118)]
        bodies = {0x2000: ('h:a', 'a', None, None), 0x2100: ('h:b', 'b', None, None),
                  0x2200: ('src:x', None, None, None)}
        rows, findings = linker.crt_slots(pe, bodies, symbols, library)
        self.assertEqual(sorted(r[0] for r in rows), [0x104, 0x108, 0x10c])
        self.assertEqual(findings, [])
        bodies.pop(0x2200)
        rows, findings = linker.crt_slots(pe, bodies, symbols, library)
        self.assertEqual(sorted(r[0] for r in rows), [0x104, 0x108])
        self.assertEqual(len(findings), 1)


class TailTests(unittest.TestCase):
    def test_zero_tails_up_to_file_and_section_alignment(self):
        pe = FakePe(size=0x4000, sections=[
            dict(name='.rdata', va=0x1000, vsize=0x900, rsize=0x1000, rptr=0x1000),
            dict(name='.data', va=0x2000, vsize=0x1800, rsize=0x1000, rptr=0x2000)])
        struct.pack_into('<II', pe.data, 0x40 + 24 + 32, 0x1000, 0x1000)
        struct.pack_into('<I', pe.data, 0x40 + 24 + 56, 0x4000)
        files, images = linker.data_alignment_tails(pe)
        self.assertEqual([r[:2] for r in files], [(0x1900, 0x2000)])
        self.assertEqual([r[:2] for r in images], [(0x1900, 0x2000), (0x3800, 0x4000)])
        pe.data[0x1a00] = 1
        files, images = linker.data_alignment_tails(pe)
        self.assertEqual([r[:2] for r in images], [(0x3800, 0x4000)])


class CommonTests(unittest.TestCase):
    def test_fill_before_a_source_datum_in_the_common_zone(self):
        pe = FakePe()
        commons = [(0x100, 0x101)]
        self.assertEqual([r[:2] for r in linker.common_zone_fill(pe, commons, [(0x104, 0x108)])],
                         [(0x101, 0x104)])
        self.assertEqual(linker.common_zone_fill(pe, commons, [(0x0f0, 0x0f4)]), [])
        pe.data[0x102] = 1
        self.assertEqual(linker.common_zone_fill(pe, commons, [(0x104, 0x108)]), [])

    def test_common_alignment_caps_at_32(self):
        self.assertEqual([common_alignment(n) for n in (1, 3, 4, 12, 16, 64, 257, 4096)],
                         [1, 4, 4, 16, 16, 32, 32, 32])


if __name__ == '__main__':
    unittest.main()
