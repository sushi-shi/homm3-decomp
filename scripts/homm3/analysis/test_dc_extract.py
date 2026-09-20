"""Raw procedure regeneration must not inherit stale text-dump module headings."""
import csv
from pathlib import Path
import tempfile
import unittest

from homm3.analysis import dc_extract
from homm3.core import nb11
from homm3.core.test_nb11 import fixture


class ProcedureRosterTest(unittest.TestCase):
    def test_crt_module_and_source_boundaries_come_from_own_compiland(self):
        symbols = nb11.Symbols(
            procedures={0x100: nb11.Procedure("_cinit", 12, module="crt0dat.obj"),
                        0x200: nb11.Procedure("local", 8, module="onexit.obj",
                                              linkage="static")},
            source_lines={"adler32.obj": [("adler32.c", 17, 0x100)],
                          "crt0dat.obj": [("crt0dat.c", 40, 0x100),
                                          ("crt0dat.c", 41, 0x100)]})
        rows = dc_extract.function_rows(symbols)
        self.assertEqual(rows[0][3:7], ["_cinit", "crt0dat.obj", "crt0dat.c", 40])
        self.assertEqual(rows[1][2:7], ["static", "local", "onexit.obj", "", ""])

    def test_embedded_symbols_reproduce_counts_without_old_csv(self):
        symbols = nb11.parse(fixture())
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp)
            dc_extract.write_functions(symbols, output)
            first = (output / "functions.csv").read_bytes()
            dc_extract.write_functions(symbols, output)
            self.assertEqual(first, (output / "functions.csv").read_bytes())
            rows = list(csv.DictReader(line for line in first.decode().splitlines()
                                       if not line.startswith("#")))
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["module"], "unit.obj")
            self.assertEqual(rows[0]["kind"], "global")
            self.assertEqual(rows[0]["line"], "10")
            self.assertEqual(rows[0]["params"], "1")
            self.assertEqual(rows[0]["locals"], "0")
            variables = list(csv.DictReader(line for line in
                (output / 'variables.csv').read_text().splitlines()
                if not line.startswith('#')))
            self.assertEqual(variables[0], {
                'proc': 'Function', 'module': 'unit.obj', 'kind': 'param',
                'sp_offset': 'sp+0xc', 'type': 'T_INT4(0074)', 'name': 'value'})


if __name__ == "__main__":
    unittest.main()
