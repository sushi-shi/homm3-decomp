"""Tests for symbol resolution by demangled spelling."""
from __future__ import annotations

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

from homm3.core import undname
from homm3.sema import context

CSV = """# generated
rva,name,unit,size,kind,provenance
0x1000,?GetTeam@game@@QBEHH@Z,game,0x20,func,dc
0x1100,?OnSameTeam@game@@QBE_NHH@Z,game,0x20,func,dc
0x1200,?Open@Widget@@QAEXXZ,widget,0x10,func,dc
0x1300,?Open@Other@@QAEXXZ,other,0x10,func,dc
0x2000,?gpGeneralText@@3PAVTTextResource@@A,game,0x4,data,dc
"""


@unittest.skipUnless(undname.available(), "llvm-undname not on PATH")
class DemangledResolveTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        path = Path(self.dir.name) / "symbol_names.csv"
        path.write_text(CSV)
        self._saved = context.SYMCSV
        context.SYMCSV = path
        self.db = context.SymbolDb()

    def tearDown(self):
        context.SYMCSV = self._saved
        self.dir.cleanup()

    def test_qualified_bare_and_pasted_spellings(self):
        with contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(self.db.resolve("game::GetTeam"), 0x1000)
            self.assertEqual(self.db.resolve("GetTeam"), 0x1000)
            self.assertEqual(self.db.resolve("game::GetTeam(int) const"), 0x1000)
            self.assertEqual(self.db.resolve("gpGeneralText"), 0x2000)
            self.assertEqual(self.db.resolve("?GetTeam@game@@QBEHH@Z"), 0x1000)
        self.assertIn("['game::GetTeam' -> ?GetTeam@game@@QBEHH@Z]", out.getvalue())

    def test_ambiguous_bare_name_lists_candidates_instead_of_guessing(self):
        with contextlib.redirect_stderr(io.StringIO()) as err:
            with self.assertRaises(SystemExit) as stop:
                self.db.resolve("Open")
        self.assertEqual(stop.exception.code, 2)
        self.assertIn("?Open@Widget@@QAEXXZ", err.getvalue())
        self.assertIn("?Open@Other@@QAEXXZ", err.getvalue())
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(self.db.resolve("Widget::Open"), 0x1200)

    def test_unknown_spelling_still_dies(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                self.db.resolve("game::NoSuchThing")


if __name__ == "__main__":
    unittest.main()
