"""Arithmetic and ordering claims must bind by operator and owner, even at equal sizes."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source


OPERATORS = (
    ("TRmgVector TRmgVector::operator+(TRmgVector other) const",
     "??HTRmgVector@@QBE?AU0@U0@@Z", "trmgvector_operator_plus"),
    ("TRmgVector TRmgVector::operator *(int scale) const",
     "??DTRmgVector@@QBE?AU0@H@Z", "trmgvector_operator_multiply"),
    ("TRmgVector TRmgVector::operator / (int divisor) const",
     "??KTRmgVector@@QBE?AU0@H@Z", "trmgvector_operator_divide"),
    ("TPoint operator+(TPoint point, TRmgVector offset)",
     "??H@YI?AUTPoint@@U0@UTRmgVector@@@Z", "operator_plus"),
    ("TRmgVector operator-(TPoint left, TPoint right)",
     "??G@YI?AUTRmgVector@@UTPoint@@0@Z", "operator_minus"),
    ("bool operator<(const TRmgGridPoint& left, const TRmgGridPoint& right)",
     "??M@YI_NABUTRmgGridPoint@@0@Z", "operator_less"),
    ("TRmgGridPoint& TRmgGridPoint::operator+=(const TPoint& offset)",
     "??YTRmgGridPoint@@QAEAAU0@ABUTPoint@@@Z", "trmggridpoint_operator_plus_assign"),
)


class ArithmeticOperatorKeysTest(unittest.TestCase):
    def scan(self, declarations):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "arithmetic.cpp"
            path.write_text("\n".join(
                f"VA(0x{0x401000 + i * 0x20:08x}, 0x20)\n{decl};"
                for i, decl in enumerate(declarations)))
            return source.scan_file(
                path, {0x1000 + i * 0x20 for i in range(len(declarations))})

    def test_equal_size_bodies_bind_without_coalescing_operators(self):
        rows = self.scan([decl for decl, _, _ in OPERATORS])
        # Reverse authority insertion order; none of these equal-size bodies
        # may be paired by its position or mistaken for another operation.
        groups = {source._demangle_key(symbol): [(symbol, 0x20)]
                  for _, symbol, _ in reversed(OPERATORS)}
        self.assertEqual(len(groups), len(OPERATORS))
        with mock.patch.object(source, "_base_authority_scan",
                               return_value=(groups, {})):
            source.join_unit("arithmetic", rows)
        for row, (_, symbol, key) in zip(rows, OPERATORS):
            self.assertEqual(row["name"].lower(), key)
            self.assertEqual(row.get("joined"), symbol)
            self.assertEqual(row["channel"], "src-VA+base")

    def test_qualified_owners_match_without_return_type_prefixes(self):
        rows = self.scan([
            "Result geometry::Vector::operator+(Vector right) const",
            "Result geometry::operator+(Point left, Vector right)",
        ])
        for row, symbol, key in zip(rows, (
                "??HVector@geometry@@QBE?AU01@U01@@Z",
                "??Hgeometry@@YI?AUResult@@UPoint@@UVector@@@Z"), (
                "geometry_vector_operator_plus", "geometry_operator_plus")):
            self.assertEqual(row["name"].lower(), key)
            self.assertEqual(source._demangle_key(symbol), key)

    def test_other_operators_and_template_owners_are_not_swallowed(self):
        for decl in (
            "Vector& Vector::operator-=(Vector right)",
            "Vector& Vector::operator++()",
            "bool Vector::operator<=(const Vector& right) const",
            "Vector operator<<(Vector left, int shift)",
            "bool Vector::operator==(Vector right) const",
            "Vector vector<int>::operator+(Vector right)",
            "Vector ns::vector<int>::operator*(int scale)",
        ):
            self.assertIsNone(source.VALUE_OPERATOR_RE.search(decl), decl)
        for symbol in (
            "??ZVector@@QAEAAV0@ABV0@@Z",  # -=
            "??EVector@@QAEAAV0@XZ",      # ++
            "??NVector@@QBE_NABV0@@Z",    # <=
            "??6@YI?AVVector@@V0@H@Z",    # <<
            "??H?$vector@H@std@@QAEHH@Z", # template owner
        ):
            self.assertIsNone(source._demangle_key(symbol), symbol)
        self.assertEqual(source._demangle_key("??8Vector@@QBE_NABV0@@Z"),
                         "vector_operator_equal")


if __name__ == "__main__":
    unittest.main()
