"""Scalar _Construct claims must pair by type and exact source signature."""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source


class ScalarConstructKeysTest(unittest.TestCase):
    def test_observed_unsigned_byte_and_other_integer_types(self):
        for code, owner in source.DEQUE_PRIMITIVE_ELEMENT.items():
            with self.subTest(owner=owner):
                symbol = f"?_Construct@std@@YIXPA{code}AB{code}@Z"
                self.assertEqual(source._demangle_key(symbol), owner + "@std_construct")

    def test_mismatched_types_qualifiers_and_overloads_do_not_join(self):
        for symbol in (
            "?_Construct@std@@YIXPAEABD@Z",
            "?_Construct@std@@YIXPAEAAE@Z",
            "?_Construct@std@@YIXPBEABE@Z",
            "?_Construct@std@@YIXPAEABE0@Z",
            "?_Construct@std@@YAXPAEABE@Z",
            "?_Construct@another@@YIXPAEABE@Z",
        ):
            with self.subTest(symbol=symbol):
                self.assertNotEqual(source._demangle_key(symbol), "unsigned_char@std_construct")

    def test_equal_size_claims_join_by_type_with_shuffled_emission(self):
        owners = ("unsigned_char", "char", "short")
        symbols = tuple(f"?_Construct@std@@YIXPA{code}AB{code}@Z" for code in "EDF")
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "customcampaign.cpp"
            path.write_text("\n".join(
                f"VA_COMPGEN(0x{0x401000 + i * 0x100:x}, 0x09, STD_CONSTRUCT, {owner})"
                for i, owner in enumerate(owners)))
            rows = source.scan_file(path, {0x1000 + i * 0x100 for i in range(3)})
        groups = {source._demangle_key(symbol): [(symbol, 9)] for symbol in reversed(symbols)}
        with mock.patch.object(source, "_base_authority_scan", return_value=(groups, {})):
            source.join_unit("customcampaign", rows)
        for row, symbol in zip(rows, symbols):
            self.assertEqual(row.get("joined"), symbol)
            self.assertEqual(row["channel"], "src-VA+base")


if __name__ == "__main__":
    unittest.main()
