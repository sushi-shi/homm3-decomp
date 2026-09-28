"""Relocation-aware verification of statically linked library sections."""

import struct
import tempfile
import unittest
from pathlib import Path

from homm3.compare.canonicalize import CoffObject
from homm3.verify import library_code
from homm3.verify.library_code import Contribution, Library

CODE = 0x60101020          # code, align 1, execute/read
CODE16 = 0x60501020        # code, align 16
DATA = 0xC0300040          # initialized data, align 4, read/write
DIR32, REL32 = 6, 20
BASE = 0x400000


def _symbol(name, value, section, storage, aux=b''):
    return (name.encode().ljust(8, b'\0') +
            struct.pack('<IhHBB', value, section, 0, storage, len(aux) // 18) + aux)


def _coff(sections, symbols):
    """sections: [(name, flags, raw, [(site, symbol index, type)])]."""
    data = bytearray(20 + 40 * len(sections))
    layout = []
    for _name, _flags, raw, relocs in sections:
        raw_offset = len(data)
        data += raw
        reloc_offset = len(data) if relocs else 0
        for site, sym, typ in relocs:
            data += struct.pack('<IIH', site, sym, typ)
        layout.append((raw_offset, reloc_offset))
    table = b''.join(symbols)
    symbol_offset = len(data)
    data += table + struct.pack('<I', 4)
    struct.pack_into('<HHIIIHH', data, 0, 0x14C, len(sections), 0, symbol_offset,
                     len(table) // 18, 0, 0)
    for i, ((name, flags, raw, relocs), (raw_offset, reloc_offset)) in enumerate(zip(sections, layout)):
        at = 20 + 40 * i
        data[at:at + 8] = name.encode().ljust(8, b'\0')
        struct.pack_into('<IIIIIIHHI', data, at + 8, 0, 0, len(raw), raw_offset,
                         reloc_offset, 0, len(relocs), 0, flags)
    return CoffObject(bytes(data))


class FakeImage:
    base = BASE

    def __init__(self, memory, slots=None):
        self.memory = memory
        self.slots = slots or {}

    def read(self, rva, size):
        chunk = bytes(self.memory[rva:rva + size])
        return chunk if len(chunk) == size else None

    def section_of(self, rva, size=1):
        if 0x1000 <= rva and rva + size <= 0x2000:
            return '.text'
        if 0x2000 <= rva and rva + size <= 0x3000:
            return '.data'
        return None

    def import_slot(self, name, dll=''):
        return self.slots.get(name)


class LibraryCodeTests(unittest.TestCase):
    def setUp(self):
        caller = bytes.fromhex('e800000000' '6800000000' 'ff1500000000' 'c3')
        helper = bytes.fromhex('33c0c3')
        datum = b'LIB\0'
        self.obj = _coff(
            [('.text', CODE, caller, [(1, 1, REL32), (6, 2, DIR32), (12, 3, DIR32)]),
             ('.text', CODE16, helper, []),
             ('.data', DATA, datum, [])],
            [_symbol('_caller', 0, 1, 2),          # 0
             _symbol('_helper', 0, 2, 3),          # 1 static helper
             _symbol('$SG1', 0, 3, 3),             # 2 datum
             _symbol('__imp_Sl', 0, 0, 2)])        # 3 import
        memory = bytearray(0x3000)
        # caller at 0x1000 (17 bytes), 0xCC fill, helper at 0x1020, datum at 0x2000
        body = bytearray(caller)
        struct.pack_into('<i', body, 1, 0x1020 - (0x1000 + 5))
        struct.pack_into('<I', body, 6, BASE + 0x2000)
        struct.pack_into('<I', body, 12, BASE + 0x2800)
        memory[0x1000:0x1000 + len(body)] = body
        memory[0x1000 + len(body):0x1020] = b'\xcc' * (0x1020 - 0x1000 - len(body))
        memory[0x1020:0x1023] = helper
        memory[0x2000:0x2004] = datum
        self.memory = memory
        self.image = FakeImage(memory, {'__imp_Sl': 0x2800})
        self.library = Library({'LIBX.LIB': {'a.obj': self.obj}}, {}, {})
        self.rows = [Contribution(0x1000, len(caller), 'LIBX.LIB', 'a.obj', 1, '_caller'),
                     Contribution(0x1020, 3, 'LIBX.LIB', 'a.obj', 2, '_helper')]

    def run_verify(self, rows=None):
        return library_code.verify(None, self.rows if rows is None else rows,
                                   library=self.library, zlib_rows=[], image=self.image)

    def test_exact_sections_relocations_and_data(self):
        verdicts, data = self.run_verify()
        self.assertEqual([v.verdict for v in verdicts], ['exact', 'exact'], verdicts)
        self.assertEqual([(key[0], address, fits) for key, address, fits, _n in data],
                         [('section', 0x2000, True)])

    def test_fill_needs_alignment_and_cc(self):
        verdicts, _ = self.run_verify()
        fill = [r for r in library_code.ranges(verdicts, self.image)
                if r[3].startswith('link fill')]
        self.assertEqual(fill, [(0x1011, 0x1020, 'library-runtime',
                                 'link fill before LIBX.LIB:a.obj#2:_helper')])
        self.memory[0x1015] = 0x90
        fill = [r for r in library_code.ranges(verdicts, self.image)
                if r[3].startswith('link fill')]
        self.assertEqual(fill, [(0x1016, 0x1020, 'library-runtime',
                                 'link fill before LIBX.LIB:a.obj#2:_helper')])

    def test_empty_section_alignment_keeps_its_fill(self):
        empty = _coff([('.text', 0x60300020, b'', [])], [])
        self.library = Library({'LIBX.LIB': {'a.obj': self.obj, 'e.obj': empty}}, {}, {})
        rows = self.rows + [Contribution(0x1014, 0, 'LIBX.LIB', 'e.obj', 1, '-')]
        verdicts, _ = self.run_verify(rows)
        self.assertEqual([v.verdict for v in verdicts], ['exact'] * 3)
        ranges = library_code.ranges(verdicts, self.image)
        self.assertIn((0x1011, 0x1014, 'library-runtime',
                       'link fill before LIBX.LIB:e.obj#1:-'), ranges)
        self.assertIn((0x1014, 0x1020, 'library-runtime',
                       'link fill before LIBX.LIB:a.obj#2:_helper'), ranges)
        self.assertFalse([r for r in ranges if r[0] == r[1]])

    def test_byte_difference_is_a_mismatch(self):
        self.memory[0x1020] = 0x31
        verdicts, _ = self.run_verify()
        self.assertEqual(verdicts[1].verdict, 'mismatch')

    def test_wrong_call_target_is_unresolved(self):
        struct.pack_into('<i', self.memory, 0x1001, 0x1021 - 0x1005)
        verdicts, _ = self.run_verify()
        self.assertEqual(verdicts[0].verdict, 'unresolved')
        self.assertIn('expected 0x1020', verdicts[0].reasons[0])

    def test_datum_bytes_must_match(self):
        self.memory[0x2000] = ord('X')
        verdicts, _ = self.run_verify()
        self.assertEqual(verdicts[0].verdict, 'unresolved')

    def test_wrong_import_slot_is_unresolved(self):
        self.image.slots['__imp_Sl'] = 0x2804
        verdicts, _ = self.run_verify()
        self.assertEqual(verdicts[0].verdict, 'unresolved')

    def test_unplaced_static_target_is_unresolved(self):
        verdicts, _ = self.run_verify(self.rows[:1])
        self.assertEqual(verdicts[0].verdict, 'unresolved')
        self.assertIn('not placed', verdicts[0].reasons[0])


class ArchiveTests(unittest.TestCase):
    def test_nul_terminated_long_names(self):
        def member(name, body):
            header = name.encode().ljust(16) + b'0'.ljust(12) + b'0'.ljust(6) * 2 + \
                b'0'.ljust(8) + str(len(body)).encode().ljust(10) + b'`\n'
            return header + body + (b'\n' if len(body) & 1 else b'')
        longnames = b'build\\intel\\mt_obj\\first.obj\0build\\intel\\mt_obj\\second.obj\0'
        data = b'!<arch>\n' + member('/', b'') + member('//', longnames) + \
            member('/0', b'one') + member('/30', b'two')
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'X.LIB'
            path.write_bytes(data)
            self.assertEqual(list(library_code.archive_members(path)),
                             [('first.obj', b'one'), ('second.obj', b'two')])


class CoverageTests(unittest.TestCase):
    def test_runtime_label_yields_only_inside_verified_union(self):
        from homm3.verify.byte_accounting import _covered
        verified = [(0x10, 0x20), (0x20, 0x28)]
        self.assertTrue(_covered(0x12, 0x26, verified))
        self.assertFalse(_covered(0x12, 0x29, verified))
        self.assertFalse(_covered(0x08, 0x12, verified))


if __name__ == '__main__':
    unittest.main()
