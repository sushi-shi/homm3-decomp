"""Source-owned vector helper instances must not be paired by equal size."""

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.core import undname
from homm3.retail_labels import source


POINT = ("?saveVector@@YI_NPAVTAbstractFile@@AAV?$vector@Utype_point@@"
         "V?$allocator@Utype_point@@@std@@@std@@@Z")
LONG = ("?saveVector@@YI_NPAVTAbstractFile@@AAV?$vector@J"
        "V?$allocator@J@std@@@std@@@Z")
UNIVERSITY = ("?saveVector@@YI_NPAVTAbstractFile@@AAV?$vector@Utype_university@@"
              "V?$allocator@Utype_university@@@std@@@std@@@Z")


def claim(element, rva=0xd2ac0):
    return dict(rva=rva, size=96, channel="src-VA", name="saveVector",
                vector_helper_signature=("saveVector", "TAbstractFile", element))


def join(rows, group, taken=None):
    with patch.object(source, "_base_authority_scan",
                      return_value=({"savevector": group}, {})):
        source.join_unit("game", rows, taken)


class VectorHelperParserTests(unittest.TestCase):
    def test_named_and_wrapped_source_parameters(self):
        for declaration in (
                "bool saveVector(TAbstractFile* outfile, std::vector<type_point>& srcVector)",
                "bool saveVector(\n TAbstractFile * outfile,\n std::vector<type_point> & srcVector )",
                "bool saveVector(TAbstractFile*,std::vector<type_point>&)",
                "bool saveVector(TAbstractFile*,std::vector<type_point,std::allocator<type_point> >&)"):
            self.assertEqual(source._vector_helper_signature(declaration, source=True),
                             ("saveVector", "TAbstractFile", "type_point"))

    def test_unrecognised_source_is_not_a_signature_claim(self):
        original = "bool saveVector(TAbstractFile* outfile, std::vector<type_point>& srcVector)"
        for changed in (
                original.replace("bool", "unsigned char"),
                original.replace("& srcVector", "* srcVector"),
                original.replace("std::vector", "const std::vector"),
                original.replace("TAbstractFile*", "const TAbstractFile*"),
                original.replace("type_point>", "type_point, CustomAllocator>"),
                original.replace("saveVector", "Owner::saveVector"),
                original.replace("srcVector)", "srcVector, int extra)")):
            self.assertIsNone(source._vector_helper_signature(changed, source=True))

    def test_scanner_retains_the_inactive_declaration_type(self):
        with tempfile.TemporaryDirectory(prefix="vector-claim-test-") as directory:
            path = Path(directory) / "game.cpp"
            path.write_text("""#if 0 // @carcass
VA(0x004d2ac0, 0x60)
bool saveVector(TAbstractFile* outfile,
                std::vector<type_point>& srcVector)
{
    // @stub
}
VA(0x004d2b20, 0x60)
bool saveVector(TAbstractFile* outfile, std::vector<type_university>& srcVector)
{
    // @stub
}
#endif
""")
            rows = source.scan_file(path, {0xd2ac0, 0xd2b20})
        self.assertEqual([r["vector_helper_signature"] for r in rows],
                         [("saveVector", "TAbstractFile", "type_point"),
                          ("saveVector", "TAbstractFile", "type_university")])


@unittest.skipUnless(undname.available(), "llvm-undname not on PATH")
class VectorHelperJoinTests(unittest.TestCase):
    def test_three_equal_size_instances_bind_two_typed_claims(self):
        point, university = claim("type_point"), claim("type_university", 0xd2b20)
        join([point, university], [(UNIVERSITY, 96), (LONG, 96), (POINT, 96)])
        self.assertEqual(point["joined"], POINT)
        self.assertEqual(university["joined"], UNIVERSITY)

    def test_missing_instance_cannot_borrow_an_icf_twin(self):
        row = claim("type_point")
        join([row], [(LONG, 96)])
        self.assertNotIn("joined", row)
        self.assertEqual(row["channel"], "src-VA")

    def test_right_signature_may_have_a_different_code_size(self):
        row = claim("type_point")
        join([row], [(LONG, 96), (POINT, 160)])
        self.assertEqual(row["joined"], POINT)

    def test_duplicate_claims_stay_ambiguous(self):
        rows = [claim("type_point", 0x1000), claim("type_point", 0x2000)]
        join(rows, [(POINT, 96), (LONG, 96)])
        self.assertTrue(all("joined" not in row for row in rows))

    def test_wrong_return_or_parameter_abi_cannot_match_by_size(self):
        for name in (POINT.replace("@@YI_N", "@@YIE"),
                     POINT.replace("@@AAV?$vector", "@@PAV?$vector"),
                     POINT.replace("@@YI_N", "@@YA_N")):
            row = claim("type_point")
            join([row], [(name, 96)])
            self.assertNotIn("joined", row)

    def test_ir_bound_name_is_not_offered_twice(self):
        row = claim("type_point")
        join([row], [(POINT, 96), (LONG, 96)], taken={POINT})
        self.assertNotIn("joined", row)

    def test_decode_failure_has_no_positional_fallback(self):
        row = claim("type_point")
        with patch.object(undname, "demangle", return_value={}):
            join([row], [(POINT, 96)])
        self.assertNotIn("joined", row)

    def test_untyped_neighbour_cannot_take_the_typed_instance(self):
        point = claim("type_point")
        generic = dict(rva=0xd2b20, size=96, channel="src-VA", name="saveVector")
        join([generic, point], [(POINT, 96), (UNIVERSITY, 96)])
        self.assertEqual(point["joined"], POINT)
        self.assertEqual(generic["joined"], UNIVERSITY)


if __name__ == "__main__":
    unittest.main()
