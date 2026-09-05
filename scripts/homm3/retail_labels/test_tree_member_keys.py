"""Negative control for the two `_Tree` members that had no join key.

`_Tree::_Init` and `_Tree::operator=` are ordinary named COMDATs VC6 emits
for any `std::map`, and both were unclaimable: their mangled names reached
`_demangle_key`'s GENERIC template arms, which produce spellings no
`VA_COMPGEN` owner can name.

  * `?_Init@?$_Tree@...` fell to TEMPLATE_MEMBER_RE as `std__tree__init`,
    a namespace-flavoured key that carries no element and therefore cannot
    tell one map's `_Init` from another's - and collides in shape with
    `basic_streambuf`/`basic_filebuf`'s own nullary `_Init`, which the
    CHAR_STREAM arms key separately.
  * `??4?$_Tree@...` fell to the `??4` arm, which splits the mangled class
    name on `@@` - a separator a TEMPLATE ARGUMENT LIST already carries -
    and yielded the unusable `?$_tree_?$_tree_operator`.

Each case below is a defect the keys must detect, or an established key the
new arms must leave alone.  The claim-side half of the contract (a kind in
`COMPGEN_KINDS` must also sit in the canonicalizer's
`DIRECT_SYMBOL_COMPGEN_KINDS`, or the build dies naming an overload group's
duplicate key) is checked here too, because that footgun has no other gate.
"""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


#: the real singleselectionwindow group - `map<int, type_map_hero_info>`,
#: CMapHeaderData's heroPlayerSetups
TREE = ("?$_Tree@HU?$pair@$$CBHUtype_map_hero_info@@@std@@U_Kfn@?$map@H"
        "Utype_map_hero_info@@U?$less@H@std@@V?$allocator@"
        "Utype_map_hero_info@@@3@@2@U?$less@H@2@V?$allocator@"
        "Utype_map_hero_info@@@2@@std@@")
INIT = f"?_Init@{TREE}IAEXXZ"
COPY_ASSIGN = f"??4{TREE}QAEAAV12@ABV12@@Z"


class TreeMemberKeyTest(unittest.TestCase):
    def test_init_keys_on_the_tree_owner(self):
        self.assertEqual(source._demangle_key(INIT),
                         "type_map_hero_info@tree_init")

    def test_copy_assign_keys_on_the_tree_owner(self):
        self.assertEqual(source._demangle_key(COPY_ASSIGN),
                         "type_map_hero_info@tree_copy_assign")

    def test_the_two_owners_agree_with_the_rest_of_the_family(self):
        # the established arms this pair must read beside
        erase = f"?_Erase@{TREE}IAEXPAU_Node@12@@Z"
        self.assertEqual(source._demangle_key(erase),
                         "type_map_hero_info@tree_erase")

    def test_a_second_map_gets_a_second_key(self):
        # the control on the owner: the generic spellings could not do this
        other = TREE.replace("type_map_hero_info", "type_map_town_info")
        self.assertEqual(source._demangle_key(f"?_Init@{other}IAEXXZ"),
                         "type_map_town_info@tree_init")

    def test_streambuf_init_is_not_captured(self):
        # `_Init` is not a _Tree-only member name: the char-stream arms own
        # these two and must keep them
        self.assertEqual(
            source._demangle_key(
                "?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@"
                "IAEXXZ"),
            "char@streambuf_init")
        self.assertEqual(
            source._demangle_key(
                "?_Init@?$basic_stringbuf@DU?$char_traits@D@std@@"
                "V?$allocator@D@2@@std@@IAEXPAD0H@Z"),
            "char@stringbuf_init")

    def test_vector_copy_assign_still_owns_its_own_arm(self):
        # `??4` over a template: the vector arm precedes the tree one and
        # must not be shadowed
        self.assertEqual(
            source._demangle_key(
                "??4?$vector@W4TArtifact@@V?$allocator@W4TArtifact@@@std@@"
                "@std@@QAEAAV01@ABV01@@Z"),
            "tartifact@vector_copy_assign")

    def test_an_unkeyed_tree_member_does_not_gain_a_tree_key(self):
        # the arms are per-member by contract: a member with no claim kind
        # must stay on the generic spelling rather than borrow a sibling's
        unrelated = source._demangle_key(f"?_Rotate@{TREE}IAEXPAU_Node@12@@Z")
        self.assertNotIn("@tree_", unrelated or "")


class CompgenKindRegistrationTest(unittest.TestCase):
    """claim lane 21's rule, as a gate.

    A kind listed in `COMPGEN_KINDS` but not in the canonicalizer's
    `DIRECT_SYMBOL_COMPGEN_KINDS` reaches that module as a pending
    SEMANTIC claim; an overload group then trips its duplicate-name check
    and the build dies far from the missing line.  The only kinds that may
    differ are the four anonymous static-initialization ones, which name no
    pre-existing COFF symbol."""

    ANONYMOUS = {"STATIC_INIT_DISPATCH", "STATIC_ATEXIT", "STATIC_DTOR",
                 "STATIC_CTOR"}

    def test_every_direct_symbol_kind_is_a_known_compgen_kind(self):
        self.assertEqual(DIRECT_SYMBOL_COMPGEN_KINDS
                         - source.COMPGEN_KINDS, set())

    def test_every_named_kind_is_registered_in_the_canonicalizer(self):
        self.assertEqual(source.COMPGEN_KINDS - DIRECT_SYMBOL_COMPGEN_KINDS,
                         self.ANONYMOUS)

    def test_the_new_tree_kinds_are_registered_both_sides(self):
        for kind in ("TREE_INIT", "TREE_COPY_ASSIGN"):
            self.assertIn(kind, source.COMPGEN_KINDS)
            self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)

    def test_the_join_admits_exactly_the_named_kinds(self):
        # the THIRD list of the same partition, and the one with no
        # diagnostic: a kind missing from join_unit's marker set reaches
        # the join as an unjoinable row that banks 0.0000 with the ratchet
        # clean. It is derived from COMPGEN_KINDS now; this pins the
        # derivation against the canonicalizer's independent copy.
        self.assertEqual(
            set(source.JOINED_COMPGEN_MARKERS),
            {f"${kind.lower()}$" for kind in DIRECT_SYMBOL_COMPGEN_KINDS})

    def test_the_anonymous_kinds_are_kept_out_of_the_join(self):
        for kind in source.ANONYMOUS_COMPGEN_KINDS:
            self.assertNotIn(f"${kind.lower()}$",
                             source.JOINED_COMPGEN_MARKERS)
            self.assertIn(kind, source.COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
