"""Negative control for the <fstream> facet block customcampaign emits.

`basic_filebuf<char>::_Initcvt` pulls `codecvt<char,char,int>` out of the
buffer's locale, and everything that conversion reaches was unclaimable:
the generic template tail reduces `?_Initcvt@?$basic_filebuf@D...` to
`std_basic_filebuf__initcvt` and `?_Save@?$_Tidyfac@V?$codecvt@D...` to
`std__tidyfac__save` - spellings no `VA_COMPGEN` owner can produce, and the
second of which collides with the numpunct/ctype/num_get/num_put
instantiations of the same static.

The facet's constant-return virtuals add a second hazard: /OPT:ICF folded
`do_encoding` with `do_max_length` and `do_in` with `do_out`, so each PAIR
is one retail row. They key as one member on purpose - two keys would leave
one spelling of each pair with a claim and no row.

Every case below is either a defect the arms must detect or an established
key they must leave alone.
"""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


TRAITS = "U?$char_traits@D@std@@"
FILEBUF = f"?$basic_filebuf@D{TRAITS}@std@@"
STREAMBUF = f"?$basic_streambuf@D{TRAITS}@std@@"
TIDYFAC = "?$_Tidyfac@V?$codecvt@DDH@std@@@std@@"


class FstreamFacetKeyTest(unittest.TestCase):
    def test_the_conversion_setup_keys(self):
        self.assertEqual(source._demangle_key(f"?_Initcvt@{FILEBUF}IAEXXZ"),
                         "char@filebuf_initcvt")
        self.assertEqual(
            source._demangle_key(f"?getloc@{STREAMBUF}QAE?AVlocale@2@XZ"),
            "char@streambuf_getloc")

    def test_the_facet_installation_keys(self):
        self.assertEqual(
            source._demangle_key(
                "?_Addfac@std@@YI?AVlocale@1@V21@PAV?$codecvt@DDH@1@@Z"),
            "char@locale_addfac_codecvt")
        self.assertEqual(
            source._demangle_key(
                f"?_Save@{TIDYFAC}SAPAV?$codecvt@DDH@2@PAV32@@Z"),
            "char@tidyfac_codecvt_save")
        self.assertEqual(source._demangle_key(f"?_Tidy@{TIDYFAC}SAXXZ"),
                         "char@tidyfac_codecvt_tidy")

    def test_the_icf_folded_virtuals_share_one_key_each(self):
        # THE defect: two keys per folded pair leaves one spelling of each
        # pair claimed against a row that does not exist
        self.assertEqual(
            source._demangle_key("?do_encoding@codecvt_base@std@@MBEHXZ"),
            source._demangle_key("?do_max_length@codecvt_base@std@@MBEHXZ"))
        self.assertEqual(
            source._demangle_key(
                "?do_in@?$codecvt@DDH@std@@MBEHAAHPBD1AAPBDPAD3AAPAD@Z"),
            source._demangle_key(
                "?do_out@?$codecvt@DDH@std@@MBEHAAHPBD1AAPBDPAD3AAPAD@Z"))
        self.assertEqual(
            source._demangle_key("?do_encoding@codecvt_base@std@@MBEHXZ"),
            "char@codecvt_base_do_encoding")
        self.assertEqual(
            source._demangle_key(
                "?do_in@?$codecvt@DDH@std@@MBEHAAHPBD1AAPBDPAD3AAPAD@Z"),
            "char@codecvt_do_in")

    def test_the_unfolded_virtual_keeps_its_own_key(self):
        # do_always_noconv is `mov al,1 / ret` - three bytes of its own,
        # NOT part of either fold
        key = source._demangle_key(
            "?do_always_noconv@codecvt_base@std@@MBE_NXZ")
        self.assertEqual(key, "char@codecvt_base_do_always_noconv")
        self.assertNotEqual(
            key, source._demangle_key("?do_encoding@codecvt_base@std@@MBEHXZ"))

    def test_the_other_tidyfac_instantiations_are_untouched(self):
        # the collision the generic `std__tidyfac__save` spelling had: four
        # more facets share this static, and each already has its own key
        for facet, member in (("numpunct@D", "numpunct"),
                              ("ctype@D", "ctype"),
                              ("num_get@D", "num_get"),
                              ("num_put@D", "num_put")):
            self.assertEqual(
                source._demangle_key(
                    f"?_Save@?$_Tidyfac@V?${facet}@std@@@std@@"
                    f"SAPAV?${facet}@2@PAV32@@Z"),
                f"char@tidyfac_{member}_save")

    def test_filebuf_init_is_not_shadowed_by_initcvt(self):
        # `?_Init@?$basic_filebuf@D` is a PREFIX of nothing here, but the
        # two members differ by three characters and sit in one table
        self.assertEqual(source._demangle_key(f"?_Init@{FILEBUF}IAEXPAU_iobuf@@W4_Initfl@12@@Z"),
                         "char@filebuf_init")
        self.assertEqual(source._demangle_key(f"?_Init@{STREAMBUF}IAEXXZ"),
                         "char@streambuf_init")

    def test_ios_base_getloc_keeps_its_own_arm(self):
        # the established entry the streambuf one must not swallow
        self.assertEqual(
            source._demangle_key("?getloc@ios_base@std@@QBE?AVlocale@2@XZ"),
            "char@ios_base_getloc")

    def test_the_new_kinds_are_registered_on_both_sides(self):
        for kind in ("FILEBUF_INITCVT", "STREAMBUF_GETLOC",
                     "TIDYFAC_CODECVT_SAVE", "TIDYFAC_CODECVT_TIDY",
                     "LOCALE_ADDFAC_CODECVT",
                     "CODECVT_BASE_DO_ALWAYS_NOCONV",
                     "CODECVT_BASE_DO_ENCODING", "CODECVT_DO_IN"):
            self.assertIn(kind, source.COMPGEN_KINDS)
            self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
