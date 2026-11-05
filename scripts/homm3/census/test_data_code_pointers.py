"""String payloads must not create native function boundaries."""
import struct
import unittest
from types import SimpleNamespace

from homm3.census.functions import data_code_pointers


class DataCodePointerTests(unittest.TestCase):
    def pointers(self, data, **kwargs):
        section = SimpleNamespace(name=".rdata", rva=0x30000, executable=False)
        image = SimpleNamespace(image_base=0x400000, sections=[section],
                                blob=lambda _section: data)
        census = SimpleNamespace(image=image, in_text=lambda r: 0x1000 <= r < 0x20000,
                                 nonpointer_data=frozenset(), literal_masks={})
        return data_code_pointers(census, **kwargs)

    def test_import_name_tail_is_not_a_function_pointer(self):
        # The native campaign import string's last aligned word is 0x41676e.
        data = b"\xeb\x00GetClassLongA\x00" + struct.pack("<I", 0x401000)
        self.assertEqual(self.pointers(data), [(0x1000, 0x30010)])

    def test_wide_literal_is_not_a_pointer_array(self):
        data = "DrawTextA\0".encode("utf-16le") + b"\0\0"
        self.assertEqual(self.pointers(data), [])

    def test_start_discovery_keeps_low_pointer_runs_that_read_as_utf16(self):
        # 0x00410041 and 0x00410042 read as UTF-16 "AABA", ended by the NUL
        # word. Function seeds use narrow literals only.
        data = struct.pack("<III", 0x410041, 0x410042, 0)
        self.assertEqual(self.pointers(data), [])
        self.assertEqual(self.pointers(data, wide=False),
                         [(0x10041, 0x30000), (0x10042, 0x30004)])
        # The narrow import-name rule still applies to start discovery.
        data = b"\xeb\x00GetClassLongA\x00" + struct.pack("<I", 0x401000)
        self.assertEqual(self.pointers(data, wide=False), [(0x1000, 0x30010)])

    def test_short_printable_pointer_bytes_remain_admissible(self):
        # Three printable bytes can be a real low-based code pointer.
        data = struct.pack("<II", 0x414141, 0x401000)
        self.assertEqual(self.pointers(data), [(0x14141, 0x30000), (0x1000, 0x30004)])


if __name__ == "__main__":
    unittest.main()
