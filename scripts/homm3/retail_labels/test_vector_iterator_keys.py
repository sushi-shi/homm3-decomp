"""Bind native vector iterator getters without borrowing another owner or ABI."""

import unittest
from unittest.mock import patch

from homm3.compare.canonicalize import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


WIDGET_VECTOR = "?$vector@PAVwidget@@V?$allocator@PAVwidget@@@std@@@std@@"


def symbol(member):
    return f"?{member}@{WIDGET_VECTOR}QAEPAPAVwidget@@XZ"


class VectorIteratorKeyTests(unittest.TestCase):
    def test_member_and_element_owners_stay_distinct(self):
        for member in ("begin", "end"):
            name = symbol(member)
            self.assertEqual(source._demangle_key(name), f"widget@vector_{member}")
            self.assertEqual(source._demangle_key(name.replace("widget", "hero")),
                             f"hero@vector_{member}")

    def test_const_overloads_and_other_abis_do_not_join(self):
        for member in ("begin", "end"):
            name = symbol(member)
            for other in (name.replace("QAE", "QBE"),
                          name.replace("XZ", "H@Z"),
                          name.replace("?$vector@", "?$deque@"),
                          f"?{member}@WidgetVector@@QAEPAPAVwidget@@XZ"):
                with self.subTest(symbol=other):
                    self.assertNotEqual(source._demangle_key(other),
                                        f"widget@vector_{member}")

    def test_source_claims_bind_actual_emitted_names(self):
        for member in ("begin", "end"):
            name = symbol(member)
            row = dict(rva=0xe64e0, size=4, channel="src-VA_COMPGEN",
                       name=f"__h3cg$hero$vector_{member}$widget")
            with patch.object(source, "_base_authority_scan", return_value=(
                    {f"widget@vector_{member}": [(name, 4)]}, {})):
                source.join_unit("hero", [row])
            self.assertEqual(row["joined"], name)

    def test_equal_size_other_owner_cannot_supply_missing_body(self):
        for member in ("begin", "end"):
            row = dict(rva=0xe64e0, size=4, channel="src-VA_COMPGEN",
                       name=f"__h3cg$hero$vector_{member}$widget")
            with patch.object(source, "_base_authority_scan", return_value=(
                    {f"hero@vector_{member}": [(symbol(member).replace(
                        "widget", "hero"), 4)]}, {})):
                source.join_unit("hero", [row])
            self.assertNotIn("joined", row)

    def test_claims_use_native_bodies_without_renaming_them(self):
        for kind in ("VECTOR_BEGIN", "VECTOR_END"):
            self.assertIn(kind, source.COMPGEN_KINDS)
            self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)
            self.assertNotIn(kind, source.ANONYMOUS_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
