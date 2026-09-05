"""Negative control for `PAIR_CTOR`, the map<string,int> value_type's
two-argument constructor.

objecttype's registry (`map<std::string, int>`) reaches retail through
`??0?$pair@$$CBV?$basic_string@D...H@std@@QAE@ABV...@ABH@Z` at 0x517c30.
`_demangle_key`'s GENERIC `??0` arm reduces every class-template
constructor to `<template>_<template>`, so that name and the
`pair<iterator, bool>` constructor `_Tree::insert` returns - which the SAME
object emits - both key `pair_pair`. A two-member group whose halves are
0x13f and 0x18 bytes could only ever be separated by length, and the claim
side could not name either half at all.

Every case below is either a defect the arm must detect or an established
key it must leave alone.
"""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


STRING = ("?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@")
#: the real objecttype symbol
PAIR_CTOR = (f"??0?$pair@$$CBV{STRING}@std@@H@std@@QAE@"
             f"ABV{STRING}@1@ABH@Z")
#: `_Tree::insert`'s return value, emitted by the same object
ITERATOR_BOOL_CTOR = (
    "??0?$pair@Viterator@?$_Tree@V?$basic_string@DU?$char_traits@D@std@@"
    "V?$allocator@D@2@@std@@U?$pair@$$CBV?$basic_string@DU?$char_traits@D"
    "@std@@V?$allocator@D@2@@std@@H@2@U_Kfn@?$map@V?$basic_string@D"
    "U?$char_traits@D@std@@V?$allocator@D@2@@std@@HU?$less@V?$basic_string"
    "@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@H@2@@2@"
    "U?$less@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
    "@2@V?$allocator@H@2@@std@@_N@std@@QAE@ABViterator@?$_Tree@"
    "V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
    "U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
    "std@@H@2@U_Kfn@?$map@V?$basic_string@DU?$char_traits@D@std@@"
    "V?$allocator@D@2@@std@@HU?$less@V?$basic_string@DU?$char_traits@D@std@@"
    "V?$allocator@D@2@@std@@@2@V?$allocator@H@2@@2@U?$less@V?$basic_string@D"
    "U?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@H@2@@1@"
    "AB_N@Z")


class PairCtorKeyTest(unittest.TestCase):
    def test_the_value_type_constructor_keys_on_its_element(self):
        self.assertEqual(source._demangle_key(PAIR_CTOR),
                         "string_int_pair@pair_ctor")

    def test_it_reads_beside_the_same_value_type_s_other_members(self):
        # `_Construct` over the same pair already keys this way; the two
        # spellings must agree or the family stops reading as one
        self.assertEqual(
            source._demangle_key(
                f"?_Construct@std@@YIXPAU?$pair@$$CBV{STRING}@std@@H@1@"
                "ABU21@@Z"),
            "string_int_pair@std_construct")

    def test_the_iterator_bool_constructor_is_left_to_the_generic_arm(self):
        # THE defect: without the bound, one object's two pair constructors
        # share a key and the group can only be split by length
        self.assertEqual(source._demangle_key(ITERATOR_BOOL_CTOR),
                         "pair_pair")

    def test_a_non_int_mapped_type_is_not_captured(self):
        # the mutation control on the `@ABH@Z` tail: a pair whose FIRST is
        # the same string but whose second is a class is a different
        # instantiation and must not answer to this key
        other = PAIR_CTOR[:-len("ABH@Z")] + "ABUtype_map_hero_info@@@Z"
        self.assertNotEqual(source._demangle_key(other),
                            "string_int_pair@pair_ctor")

    def test_the_pair_destructor_arm_is_untouched(self):
        self.assertEqual(
            source._demangle_key(
                "??1?$pair@$$CBHUtype_map_hero_info@@@std@@QAE@XZ"),
            "type_map_hero_info@pair_const_int_dtor")

    def test_the_kind_is_registered_on_both_sides(self):
        self.assertIn("PAIR_CTOR", source.COMPGEN_KINDS)
        self.assertIn("PAIR_CTOR", DIRECT_SYMBOL_COMPGEN_KINDS)
        self.assertIn("$pair_ctor$", source.JOINED_COMPGEN_MARKERS)


if __name__ == "__main__":
    unittest.main()
