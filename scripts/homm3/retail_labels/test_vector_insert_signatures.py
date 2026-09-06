"""A disappearing insert overload must not lend its identity to another ABI."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from homm3.core import undname
from homm3.retail_labels import source


VECTOR = ("?$vector@UTImageInfo@TObjectType@@"
          "V?$allocator@UTImageInfo@TObjectType@@@std@@@std@@")
SINGLE = f"?insert@{VECTOR}QAEPAUTImageInfo@TObjectType@@PAU34@ABU34@@Z"
COUNT = f"?insert@{VECTOR}QAEXPAUTImageInfo@TObjectType@@IABU34@@Z"
INT_VECTOR = "?$vector@HV?$allocator@H@std@@@std@@"
RANGE = f"?insert@{INT_VECTOR}QAEXPAHPBH1@Z"


def claim(kind, rva=0x6aeb0, size=0x2e4):
    return {"rva": rva, "size": size, "channel": "src-VA_COMPGEN",
            "name": f"__h3cg$objecttype${kind}$TImageInfo"}


def join(rows, group, taken=None):
    # Exercise the real join, including its one-claim/one-symbol fallback.
    authority = {"timageinfo@vector_insert": group}
    with patch.object(source, "_base_authority_scan", return_value=(authority, {})):
        source.join_unit("objecttype", rows, taken)


@unittest.skipUnless(undname.available(), "llvm-undname not on PATH")
class VectorInsertSignaturesTest(unittest.TestCase):
    def test_missing_count_does_not_take_single_even_at_the_same_size(self):
        row = claim("vector_insert_count")
        join([row], [(SINGLE, row["size"])])
        self.assertNotIn("joined", row)
        self.assertEqual(row["channel"], "src-VA_COMPGEN")

    def test_missing_single_does_not_take_count(self):
        row = claim("vector_insert_single", rva=0x8bf00, size=0x1ad)
        join([row], [(COUNT, row["size"])])
        self.assertNotIn("joined", row)

    def test_the_correct_overload_can_differ_in_size(self):
        row = claim("vector_insert_count")
        join([row], [(SINGLE, row["size"]), (COUNT, 0x300)])
        self.assertEqual(row["joined"], COUNT)

    def test_equal_sizes_and_reversed_order_do_not_swap_overloads(self):
        count = claim("vector_insert_count", rva=0x1000)
        single = claim("vector_insert_single", rva=0x2000)
        join([count, single], [(SINGLE, 0x2e4), (COUNT, 0x2e4)])
        self.assertEqual(count["joined"], COUNT)
        self.assertEqual(single["joined"], SINGLE)

    def test_duplicate_claims_stay_ambiguous(self):
        rows = [claim("vector_insert_count", rva=0x1000),
                claim("vector_insert_count", rva=0x2000)]
        join(rows, [(COUNT, 0x2e4), (SINGLE, 0x2e4)])
        self.assertTrue(all("joined" not in row for row in rows))

    def test_mixed_generic_claim_cannot_steal_the_explicit_overload(self):
        generic = claim("vector_insert", rva=0x1000)
        count = claim("vector_insert_count", rva=0x2000)
        join([generic, count], [(COUNT, 0x2e4), (SINGLE, 0x2e4)])
        self.assertEqual(count["joined"], COUNT)
        self.assertEqual(generic["joined"], SINGLE)

    def test_existing_generic_claims_keep_their_join(self):
        row = claim("vector_insert")
        join([row], [(COUNT, 0x2e4)])
        self.assertEqual(row["joined"], COUNT)

    def test_an_ir_bound_name_is_not_offered_twice(self):
        row = claim("vector_insert_count")
        join([row], [(COUNT, 0x2e4), (SINGLE, 0x2e4)], taken={COUNT})
        self.assertNotIn("joined", row)

    def test_decode_failure_never_falls_back_to_position(self):
        row = claim("vector_insert_count")
        with patch.object(undname, "demangle", return_value={}):
            join([row], [(COUNT, 0x2e4)])
        self.assertNotIn("joined", row)

    def test_the_iterator_range_overload_is_not_a_count_overload(self):
        declarations = undname.demangle([RANGE])
        self.assertIn(RANGE, declarations)
        self.assertIsNone(source._vector_insert_signature(declarations[RANGE]))

    def test_nested_template_parameters_and_pointer_elements(self):
        nested = (
            "?insert@?$vector@V?$vector@Vhero@@V?$allocator@Vhero@@@std@@"
            "@std@@V?$allocator@V?$vector@Vhero@@V?$allocator@Vhero@@@std@@"
            "@std@@@2@@std@@QAEXPAV?$vector@Vhero@@V?$allocator@Vhero@@"
            "@std@@@2@IABV32@@Z")
        pointer = (
            "?insert@?$vector@PAVTCampaignBonus@@V?$allocator@"
            "PAVTCampaignBonus@@@std@@@std@@QAEXPAPAVTCampaignBonus@@IABQAV3@@Z")
        declarations = undname.demangle([nested, pointer])
        for name in (nested, pointer):
            with self.subTest(name=name):
                self.assertEqual(source._vector_insert_signature(declarations[name]),
                                 "vector_insert_count")

    def test_annotation_scanner_accepts_explicit_overloads(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "objecttype.cpp"
            path.write_text(
                "VA_COMPGEN(0x0046aeb0, 0x2e4, VECTOR_INSERT_COUNT, TImageInfo)\n"
                "VA_COMPGEN(0x0048bf00, 0x1ad, VECTOR_INSERT_SINGLE, unsigned_char)\n")
            rows = source.scan_file(path, {0x6aeb0, 0x8bf00})
        self.assertEqual(len(rows), 2)
        self.assertIn("$vector_insert_count$", rows[0]["name"])
        self.assertIn("$vector_insert_single$", rows[1]["name"])


if __name__ == "__main__":
    unittest.main()
