"""A class's vfptr paths follow MSVC: the base classes that tell a table
from its siblings at each level, kept unchanged through single inheritance
and empty for a table alone at every level (VC6's own names for the
editor's hero property sheets)."""
import unittest

from homm3.census.vtables import _mangled_vftable, vfptr_names


def node(name, offset, *children):
    return (name, offset, False, list(children))


CWND = node("CPropertySheet", 0, node("CWnd", 0, node("CCmdTarget", 0, node("CObject", 0))))


def sheet(offset=0):
    return node("THeroPropsSheet", offset, CWND, node("TCreaturesParentSheet", 0x88))


class VfptrNames(unittest.TestCase):
    def test_named_namespace_is_shared_with_the_base_path(self):
        self.assertEqual(_mangled_vftable("Derived@Scope", ["Base@Scope"]),
                         "??_7Derived@Scope@@6BBase@1@@")

    def test_vc6_anonymous_namespace_is_one_backreference_component(self):
        scope = r"?%C:\Dev\Heroes 3 Exp 2\CampaignEditor\CampaignDoc.cpp1959726197"
        self.assertEqual(_mangled_vftable("TGzDeflatedScenarioMap@" + scope,
                                         ["TFileContent@" + scope]),
                         "??_7TGzDeflatedScenarioMap@" + scope + "@@6BTFileContent@1@@")

    def test_distinct_anonymous_namespaces_are_not_conflated(self):
        first, second = r"?%C:\Dev\A.cpp123", r"?%C:\Dev\B.cpp456"
        self.assertEqual(_mangled_vftable("Derived@" + first, ["Base@" + second]),
                         "??_7Derived@" + first + "@@6BBase@" + second + "@@@")

    def test_template_paths_keep_the_existing_uncompressed_spelling(self):
        self.assertEqual(_mangled_vftable("Derived", ["?$Base@H"]),
                         "??_7Derived@@6B?$Base@H@@@")

    def test_multiple_inheritance_names_each_base(self):
        self.assertEqual(vfptr_names(sheet(), {0, 0x88}),
                         {0: ["CPropertySheet"], 0x88: ["TCreaturesParentSheet"]})

    def test_single_inheritance_keeps_the_base_names(self):
        tree = node("TRandomHeroPropsSheet", 0, sheet())
        self.assertEqual(vfptr_names(tree, {0, 0x88}),
                         {0: ["CPropertySheet"], 0x88: ["TCreaturesParentSheet"]})

    def test_a_table_alone_at_every_level_has_an_empty_path(self):
        tree = node("TNonRandomHeroPropsSheet", 0, sheet(), node("TIdentifiedParentSheet", 0xd4))
        self.assertEqual(vfptr_names(tree, {0, 0x88, 0xd4}),
                         {0: ["CPropertySheet"], 0x88: ["TCreaturesParentSheet"], 0xd4: []})

    def test_two_single_chains_name_their_direct_bases(self):
        tree = node("T16bppDIBSection", 0,
                    node("?$T16bppBitmapBase@K", 0, node("?$TBitmap@GK", 0,
                                                         node("?$TBitmapBase@G", 0))),
                    node("CBitmap", 0x14, node("CGdiObject", 0x14, node("CObject", 0x14))))
        self.assertEqual(vfptr_names(tree, {0, 0x14}),
                         {0: ["?$T16bppBitmapBase@K"], 0x14: ["CBitmap"]})


if __name__ == "__main__":
    unittest.main()
