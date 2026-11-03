"""Folded legacy copies inherit observed COFF alignment, not a size guess."""
import csv
import io
import unittest
from unittest.mock import patch

from homm3.delink.data_manifest import manifest_bytes


class FoldedDataAlignmentTests(unittest.TestCase):
    def test_legacy_copy_uses_its_section_backed_definition(self):
        row = dict(name="__CTA1_N", object="codec.c", rva=0x165fe0,
                   size=8, storage="rdata", section_ordinal=1,
                   section_offset=0, section={"alignment": 8})
        legacy = {k: v for k, v in row.items()
                  if k not in ("section_ordinal", "section_offset", "section")}
        legacy["object"] = "editor.c"
        other = dict(legacy, name="another_record", rva=0x165ff0)
        with patch("homm3.delink.data_manifest.declared_types", return_value={}):
            text = manifest_bytes([legacy, row, other]).decode()
        rows = list(csv.DictReader(io.StringIO(text), delimiter="\t"))
        self.assertEqual([r["alignment"] for r in rows], ["0x8", "0x8", "0x4"])
        self.assertEqual([r["section_ordinal"] for r in rows], ["-", "1", "-"])


if __name__ == "__main__":
    unittest.main()
