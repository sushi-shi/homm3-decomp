"""Negative control for `_Tree::begin()`.

`begin()` returns `iterator(_Head->_Left)` through a hidden return pointer,
so unlike the tree's inline size/empty accessors it really is a COMDAT. The
first admitted case is command.obj's copy for `set<int>` at 0x47a670, whose
four retail callers are ResourceManager's two graphics remappers and the
random-map cluster.

The defect the arm must detect is the generic template tail: without its own
arm the member falls out on TEMPLATE_MEMBER_RE as `std__tree_begin`, a
spelling no VA_COMPGEN owner can produce AND one that every tree
instantiation in the image would share - so a claim on any one of them would
be ambiguous even if the spelling were writable.
"""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


SET_INT = ("?$_Tree@HHU_Kfn@?$set@HU?$less@H@std@@V?$allocator@H@2@@std@@"
           "U?$less@H@3@V?$allocator@H@3@@std@@")
MAP_STRING_INT = (
    "?$_Tree@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
    "U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
    "std@@H@2@U_Kfn@?$map@V?$basic_string@DU?$char_traits@D@std@@"
    "V?$allocator@D@2@@std@@HU?$less@V?$basic_string@DU?$char_traits@D@std@@"
    "V?$allocator@D@2@@std@@@2@V?$allocator@H@2@@2@U?$less@V?$basic_string@D"
    "U?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@H@2@@std@@")


class TreeBeginKeyTest(unittest.TestCase):
    def test_the_set_int_tree_begin_key(self):
        # THE defect: the generic template tail cannot be spelled by a claim.
        key = source._demangle_key(f"?begin@{SET_INT}QAE?AViterator@12@XZ")
        self.assertEqual(key, "int_set@tree_begin")
        self.assertNotEqual(key, "std__tree_begin")

    def test_begin_keeps_the_tree_owner_apart(self):
        # ...and the second reason the generic tail is unusable: it is the
        # SAME string for every tree in the image.
        self.assertNotEqual(
            source._demangle_key(f"?begin@{SET_INT}QAE?AViterator@12@XZ"),
            source._demangle_key(
                f"?begin@{MAP_STRING_INT}QAE?AViterator@12@XZ"))
        self.assertEqual(
            source._demangle_key(
                f"?begin@{MAP_STRING_INT}QAE?AViterator@12@XZ"),
            "string@tree_begin")

    def test_the_neighbouring_tree_members_are_unmoved(self):
        self.assertEqual(
            source._demangle_key(f"?_Buynode@{SET_INT}IAEPAU_Node@12@PAU312@H@Z"),
            "int_set@tree_buynode")
        self.assertEqual(
            source._demangle_key(f"?_Erase@{SET_INT}IAEXPAU_Node@12@@Z"),
            "int_set@tree_erase")

    def test_the_claim_side_arm_exists(self):
        # THE second defect, and the one this key actually hit first: the
        # symbol-side arm alone leaves the claim in join_unit's fall-through,
        # where it banks a 0.0000 `__h3cg$command$tree_begin$int_set` row with
        # the ratchet clean. Both sides must key the same string.
        source_text = open(source.__file__, encoding="utf-8").read()
        self.assertIn('if "$tree_begin$" in row["name"]:', source_text)
        self.assertIn('f"{owner}@tree_begin"', source_text)

    def test_the_kind_is_a_direct_symbol_kind(self):
        self.assertIn("TREE_BEGIN", source.COMPGEN_KINDS)
        self.assertIn("TREE_BEGIN", DIRECT_SYMBOL_COMPGEN_KINDS)
        self.assertNotIn("TREE_BEGIN", source.ANONYMOUS_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
