"""Template vptr operands must distinguish narrow and wide library bodies."""
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from homm3.census import libraries
from homm3.census.test_tail_joins import _Image


class TemplateVtableIdentityTests(unittest.TestCase):
    def test_template_vptr_refutes_the_wrong_destructor(self):
        narrow = '?$basic_streambuf@DU?$char_traits@D@std@@@std'
        wide = '?$basic_streambuf@GU?$char_traits@G@std@@@std'
        callee = b'\x90' * 11 + b'\xc3'
        body = bytes.fromhex('c7010000000083c134e900000000')
        native = bytearray(body)
        struct.pack_into('<I', native, 2, 0x402000)
        struct.pack_into('<i', native, 10, 0x1000 - 0x102e)
        image = _Image(callee + b'\xcc' * 20 + native)
        candidates = [('base.obj', '_release', callee, {}, 2)]
        for cls in (narrow, wide):
            candidates.append((cls + '.obj', '??1' + cls + '@@UAE@XZ', body,
                               {2: ('??_7' + cls + '@@6B@', 6),
                                10: ('_release', 20)}, 2))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'LIBCMT.LIB'
            path.touch()
            with patch.object(libraries, 'archive_functions', return_value=candidates):
                found = libraries.derive(image, {0x1000: 12, 0x1020: 14},
                                         {'LIBCMT': path}, log=lambda _: None,
                                         vtables={wide: 0x2000, narrow: 0x2100})
        self.assertEqual([name for rva, name, _lib, _member in found if rva == 0x1020],
                         ['??1' + wide + '@@UAE@XZ'])


if __name__ == '__main__':
    unittest.main()
