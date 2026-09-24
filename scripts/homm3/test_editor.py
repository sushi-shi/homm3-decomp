"""Editor discovery admission and byte-comparison contracts, without game data."""
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.core.image import Image
from homm3.core.project import Project
from homm3.core.test_project_flow import fixture
from homm3.editor import compile_unit, inventory, scan


def object_bytes(code, reloc=False):
    relocations = struct.pack('<IIH', 1, 0, 0x14) if reloc else b''
    symbol = b'probe\0\0\0' + struct.pack('<IhHBB', 0, 1, 0x20, 2, 0)
    return (struct.pack('<HHIIIHH', 0x14c, 1, 0, 60 + len(code) + len(relocations), 1, 0, 0)
            + struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, len(code), 60,
                          60 + len(code), 0, int(reloc), 0, 0x60000020)
            + code + relocations + symbol + struct.pack('<I', 4))


class EditorTest(unittest.TestCase):
    def image(self, code):
        section = SimpleNamespace(rva=0x1000, executable=True)
        return SimpleNamespace(image_base=0x400000, sections=[section], blob=lambda _: code)

    def test_exactness_requires_full_admitted_extent_and_preserves_duplicate_hits(self):
        code = b'\x33\xc0\xc3'
        coff = CoffObject(object_bytes(code))
        image = self.image(code + b'\xcc' + code)
        row, = scan(coff, image, {0x1000: 4, 0x1004: 3})
        self.assertEqual(row['status'], 'exact-bytes')
        self.assertEqual([h['whole_function'] for h in row['hits']], [False, True])
        row, = scan(coff, image, {})
        self.assertEqual(row['status'], 'byte-hit')
        row, = scan(coff, self.image(b'\xc3'), {})
        self.assertEqual(row['status'], 'different')

    def test_relocations_are_not_masked_into_false_exactness(self):
        code = b'\xe8\0\0\0\0\xc3'
        row, = scan(CoffObject(object_bytes(code, reloc=True)), self.image(code), {0x1000: 6})
        self.assertEqual(row['status'], 'needs-relocation-comparison')
        self.assertEqual(row['hits'], [])

    def test_inventory_rejects_overlaps_duplicates_noncode_and_unbacked_spans(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = bytearray(fixture())
            struct.pack_into('<I', data, 0x178 + 36, 0x60000020)
            (root / 'image.exe').write_bytes(data)
            image = Image(root / 'image.exe')
            path = root / 'functions.tsv'
            for contents in ('0x1000\t4\n0x1000\t4\n',
                             '0x1000\t4\n0x1002\t4\n', '0x11ff\t4\n'):
                path.write_text('rva\tsize\n' + contents)
                with self.subTest(contents=contents), self.assertRaises(ValueError):
                    inventory(path, image)
            path.write_text('# seed\nrva\tsize\n0x1000\t4\n')
            self.assertEqual(inventory(path, image), {0x1000: 4})
            data[0x178 + 36:0x178 + 40] = bytes(4)
            (root / 'image.exe').write_bytes(data)
            with self.assertRaisesRegex(ValueError, 'non-code'):
                inventory(path, Image(root / 'image.exe'))

    def test_compile_isolated_and_failure_removes_stale_report_and_object(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            project = Project(root)
            project.__dict__['manifest'] = {
                'unit': [dict(unit='shared', source='src/shared.cpp')],
                'flags': {'small': ['/O1']},
            }
            path = root / 'build/editor/h3maped/small/shared.obj'
            path.parent.mkdir(parents=True)
            path.write_bytes(b'old object')
            path.with_suffix('.json').write_text('old report')
            with patch('homm3.editor.subprocess.run', return_value=SimpleNamespace(returncode=1)) as run:
                with self.assertRaisesRegex(ValueError, 'compilation failed'):
                    compile_unit(project, 'h3maped', 'shared', 'small')
            self.assertFalse(path.exists())
            self.assertFalse(path.with_suffix('.json').exists())
            self.assertIn(str(path), run.call_args.args[0])
            self.assertEqual(run.call_args.kwargs['env']['HOMM3_DIR'], str(root))
            self.assertEqual(run.call_args.args[0][-1], '/O1')
            self.assertFalse((root / 'build/objdiff').exists())
