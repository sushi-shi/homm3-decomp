"""Every PE file byte and image address must belong to exactly one region."""
import struct
import unittest

from homm3.sema.retail_layout import Layout
from homm3.sema.test_coverage import pe_fixture


def fixture():
    data = pe_fixture()
    struct.pack_into('<I', data, 0x98 + 28, 0x400000)
    struct.pack_into('<II', data, 0x98 + 56, 0x4000, 0x200)
    return data


class LayoutTest(unittest.TestCase):
    def test_complete_partitions_include_holes_overlay_and_bss(self):
        layout = Layout(fixture())
        for regions, size in [(layout.file_regions, 0x900), (layout.image_regions, 0x4000)]:
            self.assertEqual(regions[0].start, 0)
            self.assertEqual(regions[-1].end, size)
            self.assertEqual(sum(r.size for r in regions), size)
            self.assertTrue(all(a.end == b.start for a, b in zip(regions, regions[1:])))
        self.assertEqual(layout.file_regions[-1].storage, 'overlay')
        self.assertTrue(any(r.section == '.bss' and r.storage == 'zero-fill'
                            for r in layout.image_regions))
        self.assertTrue(any(r.storage == 'raw-tail' for r in layout.file_regions))
        self.assertEqual(layout.raw(0x2010, 4), 0x610)
        for rva, size in [(0x2200, 4), (0x3000, 4), (0x21ff, 4), (0x2000, -1)]:
            with self.assertRaises(ValueError):
                layout.raw(rva, size)

    def test_overlapping_file_mappings_fail_closed(self):
        data = fixture()
        struct.pack_into('<I', data, 0x178 + 40 + 20, 0x500)
        with self.assertRaisesRegex(ValueError, 'overlapping'):
            Layout(data)

    def test_overlapping_image_mappings_fail_closed(self):
        data = fixture()
        struct.pack_into('<I', data, 0x178 + 40 + 12, 0x1100)
        with self.assertRaisesRegex(ValueError, 'overlapping'):
            Layout(data)

    def test_truncated_headers_and_directory_overflow(self):
        for mutation in ['truncated', 'directories', 'header-size']:
            data = fixture()
            if mutation == 'truncated':
                data = data[:0x100]
            elif mutation == 'directories':
                struct.pack_into('<I', data, 0x98 + 92, 17)
            else:
                struct.pack_into('<I', data, 0x98 + 60, 0x180)
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                Layout(data)
