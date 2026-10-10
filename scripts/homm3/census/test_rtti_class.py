"""A primary vtable symbol names its class as vtables.tsv spells it."""
import unittest

from homm3.census.placements import rtti_class


class RttiClassTest(unittest.TestCase):
    def test_primary_tables(self):
        self.assertEqual(rtti_class("??_7TGenerator@@6B@"), "TGenerator")
        self.assertEqual(rtti_class("??_7TVisitor@TLossCondition@@6B@"), "TVisitor@TLossCondition")
        self.assertEqual(
            rtti_class("??_7TTester@?%C:\\Dev\\Editor\\V.cpp3269924907@@6B@"),
            "TTester@?%C:\\Dev\\Editor\\V.cpp3269924907")

    def test_secondary_tables_name_no_class(self):
        self.assertIsNone(rtti_class("??_7TRandomGenerator@@6BTAbstractRandomlyAlignedGenerator@@@"))


if __name__ == "__main__":
    unittest.main()
