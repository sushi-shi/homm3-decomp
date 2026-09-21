"""Link diagnostics must retain identities and vendor imports must be exact."""
import struct
import unittest
from homm3.build.link import unresolved_symbols, link_succeeded, run_wine, main
from homm3.build.import_libraries import named_imports, exact_import_names


class LinkDiagnosticsTest(unittest.TestCase):
    def test_strict_requires_completed_clean_link(self):
        self.assertTrue(link_succeeded('', 0, True, True))
        for output, rc, exists in [
            ('', 1, True), ('', 124, True), ('', 0, False),
            ('a.obj : error LNK2001: unresolved external symbol missing', 0, True),
            ('a.obj : error LNK2005: duplicate', 0, True),
            ('a.obj : warning LNK4006: duplicate', 0, True),
            ('warning LNK4088: image being generated due to /FORCE', 0, True),
        ]:
            with self.subTest(output=output, rc=rc, exists=exists):
                self.assertFalse(link_succeeded(output, rc, exists, True))

    def test_diagnostic_mode_allows_partial_image_but_not_timeout(self):
        self.assertTrue(link_succeeded('LNK2001: unresolved external symbol missing', 1, True, False))
        self.assertFalse(link_succeeded('', 124, True, False))

    def test_timeout_does_not_accept_an_existing_executable(self):
        import subprocess
        import tempfile
        from pathlib import Path
        from unittest.mock import Mock, patch
        process = Mock(pid=12345)
        process.wait.side_effect = [subprocess.TimeoutExpired('wine', 1), 0]
        with tempfile.TemporaryDirectory() as directory:
            produced = Path(directory) / 'candidate.exe'
            produced.write_bytes(b'partial executable')
            with patch('homm3.build.link.subprocess.Popen', return_value=process), \
                 patch('homm3.build.link.os.getpgid', return_value=12345), \
                 patch('homm3.build.link.os.killpg') as kill:
                output, rc = run_wine(['wine'], directory, produced)
            self.assertEqual(rc, 124)
            self.assertEqual(output, '')
            kill.assert_called_once()

    def test_strict_rejects_force_override_before_starting_wine(self):
        from contextlib import redirect_stderr
        from io import StringIO
        for flag in ['/FORCE', '/force:unresolved', '-FORCE:MULTIPLE']:
            with redirect_stderr(StringIO()), self.assertRaises(SystemExit) as error:
                main(['--strict', '--', flag])
            self.assertEqual(error.exception.code, 2)

    def test_decorated_and_c_symbols(self):
        output = '\n'.join([
            'a.obj : error LNK2001: unresolved external symbol "int g_value" (?g_value@@3HA)',
            'b.obj : error LNK2001: unresolved external symbol "int g_value" (?g_value@@3HA)',
            'b.obj : error LNK2001: unresolved external symbol __imp__GetTickCount@0',
        ])
        self.assertEqual(unresolved_symbols(output), ['?g_value@@3HA', '__imp__GetTickCount@0'])


def import_image():
    data = bytearray(0x600)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HH', data, 0x84, 0x14c, 1)
    struct.pack_into('<H', data, 0x94, 0xe0)
    struct.pack_into('<H', data, 0x98, 0x10b)
    struct.pack_into('<I', data, 0x98 + 28, 0x400000)
    struct.pack_into('<II', data, 0x98 + 104, 0x1000, 40)
    struct.pack_into('<4I', data, 0x178 + 8, 0x400, 0x1000, 0x400, 0x200)
    struct.pack_into('<5I', data, 0x200, 0x1040, 0, 0, 0x1080, 0x1060)
    struct.pack_into('<3I', data, 0x240, 0x10a0, 0x8000000b, 0)
    data[0x280:0x288] = b'TEST.dll'
    data[0x2a2:0x2ac] = b'_Export@4\0'
    return data


class ImportLibraryTest(unittest.TestCase):
    def test_short_import_preserves_exact_stdcall_export(self):
        strings = b'_Export@4\0TEST.dll\0'
        member = struct.pack('<HHHHIIHH', 0, 0xffff, 0, 0x14c, 0,
                             len(strings), 0, 2 << 2) + strings
        header = (b'export/         ' + b'0           ' + b'0     ' +
                  b'0     ' + b'0       ' + f'{len(member):<10}'.encode() + b'`\n')
        archive = b'!<arch>\n' + header + member + (b'\n' if len(member) & 1 else b'')
        result = exact_import_names(archive, 'TEST.dll', ['_Export@4'])
        self.assertEqual(result[:86], archive[:86])
        self.assertEqual(struct.unpack_from('<H', result, 86)[0], 1 << 2)
        self.assertEqual(result[88:], archive[88:])
        with self.assertRaisesRegex(ValueError, 'differ from retail'):
            exact_import_names(archive, 'TEST.dll', ['Export@4'])

    def test_preserves_decoration_and_ordinals(self):
        self.assertEqual(named_imports(import_image()), {'TEST.dll': ['_Export@4', 11]})

    def test_missing_directory_terminator(self):
        data = import_image()
        struct.pack_into('<I', data, 0x98 + 108, 20)
        with self.assertRaisesRegex(ValueError, 'unterminated import directory'):
            named_imports(data)

    def test_unbacked_lookup(self):
        data = import_image()
        struct.pack_into('<I', data, 0x200, 0x3000)
        with self.assertRaisesRegex(ValueError, 'not file backed'):
            named_imports(data)


if __name__ == '__main__':
    unittest.main()
