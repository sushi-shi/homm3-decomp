"""Deque map growth retains its element, pointer return and unsigned size ABI."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source


GROW = "?_Growmap@?$deque@HV?$allocator@H@std@@@std@@IAEPAPAHI@Z"


class DequeGrowmapKeysTest(unittest.TestCase):
    def test_equal_size_primitive_bodies_join_by_element(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "deque.cpp"
            path.write_text(
                "VA_COMPGEN(0x00401000, 0x6D, DEQUE_GROWMAP, int)\n"
                "VA_COMPGEN(0x00402000, 0x6D, DEQUE_GROWMAP, unsigned_int)\n")
            rows = source.scan_file(path, {0x1000, 0x2000})
        symbols = [GROW, GROW.replace("@H", "@I").replace("PAPAH", "PAPAI")]
        groups = {source._demangle_key(symbol): [(symbol, 109)]
                  for symbol in reversed(symbols)}
        with mock.patch.object(source, "_base_authority_scan", return_value=(groups, {})):
            source.join_unit("deque", rows)
        self.assertEqual([row.get("joined") for row in rows], symbols)

    def test_other_allocators_overloads_and_members_do_not_claim_int(self):
        self.assertEqual(source._demangle_key(GROW), "int@deque_growmap")
        for other in (
            GROW.replace("allocator@H", "allocator@I"),
            GROW.replace("PAPAHI", "PAPAII"),
            GROW.replace("PAPAHI", "PAPAHH"),
            GROW.replace("PAPAHI", "PAHI"),
            GROW.replace("IAE", "QAE"),
            GROW.replace("IAE", "IBE"),
            GROW.replace("_Growmap", "_Buyback"),
            GROW + "suffix",
        ):
            with self.subTest(symbol=other):
                self.assertNotEqual(source._demangle_key(other), "int@deque_growmap")


if __name__ == "__main__":
    unittest.main()
