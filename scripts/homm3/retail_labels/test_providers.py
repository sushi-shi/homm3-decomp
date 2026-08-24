"""Unit tests for hand-owned retail-label provider semantics."""

from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from homm3.retail_labels import providers


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
