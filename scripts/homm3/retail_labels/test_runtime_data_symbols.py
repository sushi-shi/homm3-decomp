"""Library data placements name game-referenced objects, never local statics."""
import tempfile
import unittest
from pathlib import Path

from homm3.retail_labels import providers

HEADER = "rva\tsize\tkind\tlibrary\tmember\tsection\tsymbol\tevidence\n"


class RuntimeDataSymbolsTest(unittest.TestCase):
    def claims(self, rows):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "runtime-contributions.tsv"
            path.write_text(HEADER + "".join("\t".join(r) + "\n" for r in rows))
            return {c.rva: c.name for c in providers.runtime_data_symbols(path)}

    def test_external_library_data_is_named(self):
        got = self.claims([
            ("0x243da8", "16", "data", "DXGUID.LIB", "dxguid.obj", "129",
             "_DPAID_ServiceProvider", "-"),
            ("0x2455f0", "8", "data", "LIBCPMT.LIB", "ios.obj", "3",
             "?_Fpz@std@@3_JB", "placed-by-layout"),
        ])
        self.assertEqual(got, {0x243da8: "_DPAID_ServiceProvider",
                               0x2455f0: "?_Fpz@std@@3_JB"})

    def test_code_zlib_statics_and_repeats_are_skipped(self):
        got = self.claims([
            ("0x217e4f", "13", "code", "LIBCMT.LIB", "rand.obj", "1",
             "_srand", "-"),
            ("0x244500", "1024", "data", "zlib", "crc32.obj", "2",
             "_crc_table", "-"),
            ("0x25ba30", "4", "data", "LIBCMT.LIB", "a.obj", "2",
             "$T16509", "-"),
            ("0x25ba40", "4", "data", "LIBCMT.LIB", "b.obj", "2",
             "_twice", "-"),
            ("0x25ba50", "4", "bss", "LIBCMT.LIB", "c.obj", "3",
             "_twice", "-"),
        ])
        self.assertEqual(got, {})


if __name__ == "__main__":
    unittest.main()
