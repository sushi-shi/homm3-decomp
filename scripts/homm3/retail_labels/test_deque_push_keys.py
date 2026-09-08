"""Primitive deque append claims retain element identity and overload."""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source


class DequePushKeysTest(unittest.TestCase):
    def test_equal_size_primitive_appends_join_by_element(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "deque.cpp"
            path.write_text(
                "VA_COMPGEN(0x00401000, 0x2CF, DEQUE_PUSH_BACK, int)\n"
                "VA_COMPGEN(0x00402000, 0x2CF, DEQUE_PUSH_BACK, unsigned_int)\n")
            rows = source.scan_file(path, {0x1000, 0x2000})
        symbols = [
            f"?push_back@?$deque@{code}V?$allocator@{code}@std@@@std@@QAEXAB{code}@Z"
            for code in ("H", "I")]
        groups = {source._demangle_key(symbol): [(symbol, 0x2CF)]
                  for symbol in reversed(symbols)}
        with mock.patch.object(source, "_base_authority_scan", return_value=(groups, {})):
            source.join_unit("deque", rows)
        self.assertEqual([row.get("joined") for row in rows], symbols)

    def test_inconsistent_allocator_or_argument_does_not_claim_int(self):
        for symbol in (
            "?push_back@?$deque@HV?$allocator@I@std@@@std@@QAEXABH@Z",
            "?push_back@?$deque@HV?$allocator@H@std@@@std@@QAEXABI@Z",
        ):
            self.assertNotEqual(source._demangle_key(symbol), "int@deque_push_back")


if __name__ == "__main__":
    unittest.main()
