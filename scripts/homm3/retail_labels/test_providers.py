"""Unit tests for hand-owned retail-label provider semantics."""

from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from homm3.retail_labels import providers


class ZlibMapTest(unittest.TestCase):
    def claims(self, body):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / 'zlib.tsv'
            path.write_text(body)
            return providers.zlib_map(path)

    def test_legacy_function_schema(self):
        claims = self.claims('rva\tsize\tname\tunit\n0x100\t16\t_crc32\tcrc32\n')
        self.assertEqual((claims[0].kind, claims[0].channel), ('func', 'zlib-map'))

    def test_typed_rows_keep_data_out_of_function_channel(self):
        claims = self.claims('rva\tsize\tname\tunit\tkind\n'
                             '0x100\t16\t_crc32\tcrc32\tfunc\n'
                             '0x200\t1024\t_crc_table\tcrc32\tdata\n')
        self.assertEqual([(c.kind, c.channel, c.size) for c in claims],
                         [('func', 'zlib-map', 16), ('data', 'zlib-data-map', 1024)])

    def test_unknown_kind_is_rejected(self):
        with self.assertRaises(ValueError):
            self.claims('rva\tsize\tname\tunit\tkind\n0x100\t16\t_crc32\tcrc32\ttypo\n')


class RelocAliasTest(unittest.TestCase):
    def test_nonzero_addends_collapse_interior_targets_to_owner_base(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "aliases.tsv"
            path.write_text(
                "function_rva\ttarget_rva\tsite_rva\towner\taddend\toccurrences\n"
                "0x100\t0x2048\t0x110\t?table@@3PAHA\t0x48\t1\n"
                "0x100\t0x206c\t0x120\t?table@@3PAHA\t0x6c\t1\n"
            )

            claims = providers.reloc_aliases(path)

        self.assertEqual(len(claims), 1)
        self.assertEqual(claims[0].rva, 0x2000)
        self.assertEqual(claims[0].name, "?table@@3PAHA")

    def test_zero_addend_keeps_target_as_owner_base(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "aliases.tsv"
            path.write_text(
                "function_rva\ttarget_rva\tsite_rva\towner\taddend\toccurrences\n"
                "0x100\t0x2000\t0x110\t?datum@@3HA\t0x0\t1\n"
            )

            claims = providers.reloc_aliases(path)

        self.assertEqual([(c.rva, c.name) for c in claims],
                         [(0x2000, "?datum@@3HA")])


if __name__ == "__main__":
    unittest.main()
