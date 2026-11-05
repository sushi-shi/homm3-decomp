"""A reviewed COFF data section separates short text from real pointers."""
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
import struct
import tempfile
import unittest

from homm3.census.libraries import reviewed_fixed_data_words
from homm3.census.functions import data_code_pointers
from homm3.census.find_relocs import data_sites
from homm3.verify.test_library_code import _coff, _symbol, DATA


class FixedLibraryDataTests(unittest.TestCase):
    def fixed(self, damage=False, **changes):
        obj = _coff([('.data', DATA, b'\0' * 4 + b'ENC\0', [(0, 1, 6)])],
                    [_symbol('_table', 0, 1, 2), _symbol('_text', 0, 0, 2)])
        payload = struct.pack('<I', 0x402000) + (b'ENU\0' if damage else b'ENC\0')
        sec = SimpleNamespace(rva=0x30000, size=8, raw_offset=0, executable=False,
                              name='.data')
        image = SimpleNamespace(data=payload, image_base=0x400000,
                                section_of=lambda _rva: sec, sections=[sec],
                                blob=lambda _sec: payload)
        row = dict(rva='0x30000', size='8', kind='data', library='LIBCMT.LIB',
                   member='table.obj', section='1', symbol='_table', ownership='whole-section')
        row.update(changes)
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / 'LIBCMT.LIB'
            archive.touch()
            with patch('homm3.verify.library_code.archive',
                       return_value=({'table.obj': obj}, {})):
                fixed = reviewed_fixed_data_words(image, [row], {'LIBCMT': archive})
        return image, fixed

    def test_fixed_code_is_excluded_and_relocated_pointer_remains(self):
        image, fixed = self.fixed()
        self.assertEqual(fixed, {0x30004})
        census = SimpleNamespace(image=image, nonpointer_data=fixed, literal_masks={},
                                 in_text=lambda rva: 0x1000 <= rva < 0x40000)
        self.assertEqual(data_code_pointers(census), [(0x2000, 0x30000)])
        image.in_image = lambda value: 0x401000 <= value < 0x440000
        self.assertEqual(set(data_sites(image, image.sections[0], nonpointer_data=fixed)),
                         {0x30000})

    def test_unverified_or_mismatching_data_does_not_filter(self):
        for changes in ({'ownership': '-'}, {'kind': 'alias'}, {'size': '4'}, {'damage': True}):
            with self.subTest(changes=changes):
                self.assertEqual(self.fixed(**changes)[1], set())


if __name__ == '__main__':
    unittest.main()
