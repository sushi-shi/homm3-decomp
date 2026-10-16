"""Padding-looking operands are part of native library bodies."""
import unittest

from homm3.census.code_padding import trimmed_end
from homm3.census.libraries import _member_functions
from homm3.census.placements import _functions_of
from homm3.delink.coffx import Obj
from homm3.verify.test_library_code import _coff, _symbol, CODE, REL32


class CodePaddingTest(unittest.TestCase):
    def test_real_instruction_padding_is_trimmed(self):
        self.assertEqual(trimmed_end(b"\xc3\x90\xcc", 0, 3), 1)

    def test_short_jump_displacement_is_retained(self):
        for last in (0x90, 0xCC):
            body = bytes((0xEB, last))
            with self.subTest(last=last):
                self.assertEqual(trimmed_end(body, 0, 2), 2)
                self.assertEqual(trimmed_end(body + b"\x90\xcc", 0, 4), 2)

    def test_immediate_operand_is_retained(self):
        body = bytes.fromhex('b890cccc90')
        self.assertEqual(trimmed_end(body + b"\x90\xcc", 0, 7), 5)

    def test_relocation_floor_is_retained(self):
        body = bytes.fromhex('e9000000909090')
        self.assertEqual(trimmed_end(body, 0, 7, 5), 5)

    def test_undecodable_prefix_has_no_padding_boundary(self):
        body = b"\x0f" * 16 + b"\x90\xcc"
        self.assertEqual(trimmed_end(body, 0, len(body)), len(body))

    def test_symbol_boundaries_are_respected(self):
        body = b"\xcc\xc3\x90\xcc\xc3"
        self.assertEqual(trimmed_end(body, 1, 4), 2)
        self.assertEqual(trimmed_end(body, 0, 1), 1)

    def test_object_function_retains_displacement_but_removes_fill(self):
        coff = _coff([('.text', CODE, b'\xeb\xcc\x90\xcc', [])],
                     [_symbol('f', 0, 1, 2)])
        rows = _functions_of(Obj(coff.data))
        self.assertEqual(rows[0][3], b'\xeb\xcc')

    def test_external_assembler_extent_retains_displacement(self):
        coff = _coff([('.text', CODE, b'\x90\xeb\xcc\x90\xcc', [])],
                     [_symbol('f', 0, 1, 2), _symbol('inside', 1, 1, 3)])
        rows = _member_functions(Obj(coff.data))
        self.assertIn(b'\x90\xeb\xcc', [body for name, body, _, _ in rows if name == 'f'])


if __name__ == '__main__':
    unittest.main()
