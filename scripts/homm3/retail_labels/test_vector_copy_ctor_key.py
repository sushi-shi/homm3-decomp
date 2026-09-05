"""Negative control for the element oracle behind VECTOR_COPY_CTOR.

Every `std::vector<T>` constructor a TU emits - default, allocator-taking
and copy, over every element - shares ONE join group.  `_demangle_key`'s
generic `??0` arm keeps only a class template's stable name, and a lexical
`std::vector<T>::vector` declarator reduces to the very same `vector_vector`
spelling, so the group is shared by construction and both of join_unit's
older oracles fail on it:

  * the POSITIONAL zip pairs claims in rva order against mangled names in
    COFF (definition) order, and definition order is only the same order
    for members the SOURCE writes.  A copy ctor is compiler-generated - cl
    emits it where it is first NEEDED - so the two orders diverge.
    singleselectionwindow's five element copy ctors sit in COFF order
    hero, int, vector<hero>, vector<type_artifact>, CampaignScenarioInfo
    against an rva order that leads with int: the zip would exchange the
    first two.
  * `_size_pairing` cannot separate two elements whose bodies are the same
    length, and the two nested-vector copy ctors are 108 bytes on BOTH
    sides.

The ELEMENT settles it, and retail proves the assignment independently:
0x5941b0 calls `_Construct<vector<hero>>`, 0x594220 calls
`_Construct<vector<type_artifact>>`.
"""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


#: the real singleselectionwindow group, in the COFF order the base obj
#: emits it, with each symbol's content size
COFF_GROUP = [
    ("??0?$vector@HV?$allocator@H@std@@@std@@QAE@ABV?$allocator@H@1@@Z",
     0x1b),
    ("??0?$vector@Vhero@@V?$allocator@Vhero@@@std@@@std@@QAE@ABV01@@Z",
     0x8d),
    ("??0?$vector@HV?$allocator@H@std@@@std@@QAE@ABV01@@Z", 0x66),
    ("??0?$vector@V?$vector@Vhero@@V?$allocator@Vhero@@@std@@@std@@"
     "V?$allocator@V?$vector@Vhero@@V?$allocator@Vhero@@@std@@@std@@@2@"
     "@std@@QAE@ABV01@@Z", 0x6c),
    ("??0?$vector@V?$vector@Utype_artifact@@V?$allocator@Utype_artifact@@"
     "@std@@@std@@V?$allocator@V?$vector@Utype_artifact@@"
     "V?$allocator@Utype_artifact@@@std@@@std@@@2@@std@@QAE@ABV01@@Z",
     0x6c),
    ("??0?$vector@UCampaignScenarioInfo@@"
     "V?$allocator@UCampaignScenarioInfo@@@std@@@std@@QAE@ABV01@@Z", 0x87),
]
ALLOCATOR_CTOR, HERO, INT, HERO_VEC, ARTIFACT_VEC, SCENARIO = (
    name for name, _size in COFF_GROUP)


def claim(rva, owner, size=0x6c):
    return {"rva": rva, "size": size, "channel": "src-VA_COMPGEN",
            "name": f"__h3cg$singleselectionwindow$vector_copy_ctor${owner}"}


class ElementPairingTest(unittest.TestCase):
    def test_the_two_same_length_twins_pair_by_element(self):
        rows = [claim(0x1941b0, "hero_vector"),
                claim(0x194220, "type_artifact_vector")]
        self.assertEqual(source._element_pairing(rows, COFF_GROUP),
                         {0x1941b0: HERO_VEC, 0x194220: ARTIFACT_VEC})

    def test_the_sizes_could_not_have_done_it(self):
        # the control on the control: both twins are 0x6c on both sides
        rows = [{"rva": 0x1941b0, "size": 0x6c},
                {"rva": 0x194220, "size": 0x6c}]
        group = [(HERO_VEC, 0x6c), (ARTIFACT_VEC, 0x6c)]
        self.assertIsNone(source._size_pairing(rows, group))

    def test_the_zip_would_have_swapped_two_others(self):
        # ...and the ORDER premise is wrong for the same group: COFF order
        # leads with hero where retail's rva order leads with int
        coff = [n for n, _s in COFF_GROUP if n != ALLOCATOR_CTOR]
        self.assertEqual(coff[:2], [HERO, INT])

    def test_every_element_spelling_in_the_group_is_reachable(self):
        rows = [claim(0x18fe80, "int", 0x66), claim(0x18fef0, "hero", 0x8d),
                claim(0x1941b0, "hero_vector"),
                claim(0x194220, "type_artifact_vector"),
                claim(0x194290, "campaignscenarioinfo", 0x87)]
        self.assertEqual(
            source._element_pairing(rows, COFF_GROUP),
            {0x18fe80: INT, 0x18fef0: HERO, 0x1941b0: HERO_VEC,
             0x194220: ARTIFACT_VEC, 0x194290: SCENARIO})

    def test_a_non_copy_ctor_is_never_offered(self):
        # the allocator-taking ctor is `vector<int>`'s too, and an `int`
        # claim must not be able to take it
        rows = [claim(0x18fe80, "int", 0x66)]
        self.assertEqual(
            source._element_pairing(rows, [(ALLOCATOR_CTOR, 0x1b)]), {})

    def test_an_element_naming_two_symbols_declines(self):
        rows = [claim(0x1941b0, "hero_vector")]
        self.assertEqual(
            source._element_pairing(rows, [(HERO_VEC, 0x6c),
                                           (HERO_VEC, 0x6c)]), {})

    def test_an_unknown_element_declines(self):
        self.assertEqual(
            source._element_pairing([claim(0x1000, "widget")], COFF_GROUP),
            {})

    def test_a_claim_of_another_kind_is_left_alone(self):
        row = {"rva": 0x1000, "size": 0x6c, "channel": "src-VA_COMPGEN",
               "name": "__h3cg$u$implicit_copy_ctor$hero_vector"}
        self.assertEqual(source._element_pairing([row], COFF_GROUP), {})

    def test_a_lexical_declarator_claim_is_left_alone(self):
        # claim lane 22 pairs three of these through a carcass VA()
        # declarator whose name IS `vector_vector`; the element oracle must
        # not touch it, and the size oracle must keep resolving it
        row = {"rva": 0x18fe80, "size": 0x66, "channel": "src-VA",
               "name": "vector_vector"}
        self.assertEqual(source._element_pairing([row], COFF_GROUP), {})

    def test_an_empty_group_declines(self):
        self.assertEqual(
            source._element_pairing([claim(0x1000, "hero_vector")], []), {})


class VectorOwnerTest(unittest.TestCase):
    """The element decode, on its own - it also still backs every keyed
    vector member, so the established spellings must not move."""

    def test_the_five_elements(self):
        self.assertEqual(
            [source._vector_owner(n) for n in
             (INT, HERO, HERO_VEC, ARTIFACT_VEC, SCENARIO)],
            ["int", "hero", "hero_vector", "type_artifact_vector",
             "campaignscenarioinfo"])

    def test_the_keyed_members_still_read_the_same(self):
        self.assertEqual(
            source._demangle_key(
                "??1?$vector@VBlackBoxData@@V?$allocator@VBlackBoxData@@"
                "@std@@@std@@QAE@XZ"),
            "blackboxdata@vector_dtor")
        self.assertEqual(
            source._demangle_key(
                "??4?$vector@W4TArtifact@@V?$allocator@W4TArtifact@@@std@@"
                "@std@@QAEAAV01@ABV01@@Z"),
            "tartifact@vector_copy_assign")
        self.assertEqual(
            source._demangle_key(
                "?_Ucopy@?$vector@VTSeerHut@@V?$allocator@VTSeerHut@@@std@@"
                "@std@@IAEPAVTSeerHut@@PBV3@0PAV3@@Z"),
            "tseerhut@vector_ucopy")

    def test_every_vector_ctor_still_shares_one_key(self):
        for name, _size in COFF_GROUP:
            self.assertEqual(source._demangle_key(name), "vector_vector")

    def test_a_non_vector_name_has_no_element(self):
        self.assertIsNone(source._vector_owner("??0NewSMapHeader@@QAE@XZ"))

    def test_the_kind_is_registered_both_sides(self):
        self.assertIn("VECTOR_COPY_CTOR", source.COMPGEN_KINDS)
        self.assertIn("VECTOR_COPY_CTOR", DIRECT_SYMBOL_COMPGEN_KINDS)


class CopyCtorTailTest(unittest.TestCase):
    """The predicate the element oracle is scoped by, on its own."""

    def test_it_accepts_both_qualifier_depths(self):
        self.assertTrue(source.COPY_CTOR_TAIL_RE.search("??0T@@QAE@ABV0@@Z"))
        self.assertTrue(
            source.COPY_CTOR_TAIL_RE.search("??0T@N@@QAE@ABV01@@Z"))
        self.assertTrue(
            source.COPY_CTOR_TAIL_RE.search("??0T@@QAE@ABU0@@Z"))

    def test_it_rejects_a_ctor_over_a_different_class(self):
        self.assertIsNone(source.COPY_CTOR_TAIL_RE.search(ALLOCATOR_CTOR))
        self.assertIsNone(
            source.COPY_CTOR_TAIL_RE.search("??0T@@QAE@ABVOther@@@Z"))

    def test_it_rejects_the_default_ctor(self):
        self.assertIsNone(source.COPY_CTOR_TAIL_RE.search("??0T@@QAE@XZ"))

    def test_it_rejects_a_two_argument_ctor_ending_in_the_class(self):
        self.assertIsNone(
            source.COPY_CTOR_TAIL_RE.search("??0T@@QAE@HABV0@@Z1@Z"))


if __name__ == "__main__":
    unittest.main()
