"""Tests for the reference-site listing of `homm3 sema xref --to`."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from homm3.sema import _asm, context, xref


class RowCoveringTest(unittest.TestCase):
    ROWS = _asm.reloc_rows(
        "00401000 <f>:\n"
        "  401000: 8b 0d 88 92 69 00\tmov\tecx, dword ptr [0x699288] <data_299288>\n"
        "  401006: c3\tret\n")

    def test_site_inside_an_instruction_finds_its_row(self):
        self.assertEqual(xref._row_covering(self.ROWS, 0x401002)[2],
                         "mov ecx, dword ptr [0x699288] <data_299288>")
        self.assertEqual(xref._row_covering(self.ROWS, 0x401006)[2], "ret")

    def test_site_past_the_decode_end_is_none(self):
        self.assertIsNone(xref._row_covering(self.ROWS, 0x401007))
        self.assertIsNone(xref._row_covering(self.ROWS, 0x400fff))


class DataLabelTest(unittest.TestCase):
    def test_symbol_label_names_data_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "symbol_names.csv"
            path.write_text("rva,name,unit,size,kind,provenance\n"
                            "0x1000,?f@@YAXXZ,u,0x10,func,dc\n"
                            "0x2000,?gpGeneralText@@3PAVTTextResource@@A,u,0x4,data,dc\n")
            saved = context.SYMCSV
            context.SYMCSV = path
            try:
                db = context.SymbolDb()
            finally:
                context.SYMCSV = saved
        self.assertEqual(db.label(0x2000),
                         "0x00002000 ?gpGeneralText@@3PAVTTextResource@@A (data)")
        self.assertEqual(db.label(0x1000), "0x00001000 ?f@@YAXXZ [u]")
        self.assertEqual(db.label(0x3000), "0x00003000 ?")


if __name__ == "__main__":
    unittest.main()
