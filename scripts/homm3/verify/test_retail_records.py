import struct
import unittest
from types import SimpleNamespace

from homm3.verify import retail_records


class FakePe:
    def __init__(self, data, base=0):
        self.data, self.image_base = bytes(data), base
        self.sections = [{'name': '.data', 'va': 0, 'vsize': len(data),
                          'rsize': len(data), 'rptr': 0}]

    def read(self, rva, size):
        return self.data[rva:rva + size] if rva + size <= len(self.data) else None


class RetailRecordTests(unittest.TestCase):
    def test_real_names_follow_vc6_extended_spelling(self):
        self.assertEqual(retail_records._extended80(struct.pack('<d', 1.0)),
                         '3fff8000000000000000')
        self.assertEqual(retail_records._extended80(struct.pack('<d', 0.1)),
                         '3ffbccccccccccccd000')
        self.assertEqual(retail_records._extended80(struct.pack('<f', 0.1)),
                         '3ffbcccccd0000000000')
        self.assertEqual(retail_records._extended80(struct.pack('<d', -2.0)),
                         'c0008000000000000000')

    def test_c_string_decodes_escapes(self):
        self.assertEqual(retail_records._c_string(r'a\n\x41\101\\'), b'a\nAA\\')
        self.assertIsNone(retail_records._c_string('bad\\'))

    def test_padding_needs_whole_contributions_and_exact_alignment(self):
        pe = FakePe(bytes(64))
        records = retail_records.Records()
        records.ends.add(0x13)
        records.starts[0x14] = 4
        records.starts[0x20] = 8
        rows = [{'category': 'missing', 'start': 0x13, 'end': 0x14},
                {'category': 'missing', 'start': 0x19, 'end': 0x20},
                {'category': 'missing', 'start': 0x16, 'end': 0x18}]
        pads = retail_records.alignment_padding(pe, rows, records)
        # Only the gap after a whole contribution that the next one's
        # alignment exactly explains is padding.
        self.assertEqual([(p.start, p.end) for p in pads], [(0x13, 0x14)])

    def test_padding_rejects_nonzero_bytes(self):
        data = bytearray(64)
        data[0x13] = 1
        records = retail_records.Records()
        records.ends.add(0x13)
        records.starts[0x14] = 4
        rows = [{'category': 'missing', 'start': 0x13, 'end': 0x14}]
        self.assertEqual(retail_records.alignment_padding(FakePe(data), rows, records), [])


if __name__ == '__main__':
    unittest.main()
