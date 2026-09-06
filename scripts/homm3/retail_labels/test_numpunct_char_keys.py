"""Negative control for numpunct<char>'s two character accessors.

`numpunct<char>` publishes SIX virtuals through vtbl_245728, and the three
that return a `basic_string` already have keys.  The other two -
`do_decimal_point` (slot 1, retail 0x455930) and `do_thousands_sep`
(slot 2, retail 0x455940) - are `mov al,[ecx+0xc] / ret` and
`mov al,[ecx+0xd] / ret`, four bytes each, and had none: the generic
template tail reduced them to `std_numpunct_do_decimal_point` and
`std_numpunct_do_thousands_sep`, spellings no `VA_COMPGEN` owner can
produce, so bottomviewsubwindow could not claim either row.

The two must NOT share one key the way the ICF-folded codecvt pairs do -
they read different members and are two distinct retail rows.
"""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


STRING = "?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@"
DP = "?do_decimal_point@?$numpunct@D@std@@MBEDXZ"
KS = "?do_thousands_sep@?$numpunct@D@std@@MBEDXZ"


class NumpunctCharKeyTest(unittest.TestCase):
    def test_the_character_accessor_keys(self):
        # THE defect: without their own arms both fall through to the
        # generic template tail, which no VA_COMPGEN owner can spell.
        self.assertEqual(source._demangle_key(DP),
                         "char@numpunct_do_decimal_point")
        self.assertNotEqual(source._demangle_key(DP),
                            "std_numpunct_do_decimal_point")
        self.assertEqual(source._demangle_key(KS),
                         "char@numpunct_do_thousands_sep")
        self.assertNotEqual(source._demangle_key(KS),
                            "std_numpunct_do_thousands_sep")

    def test_the_two_accessors_do_not_share_a_key(self):
        # they are two retail rows reading two different members; sharing
        # one key would leave one spelling claimed against no row
        self.assertNotEqual(source._demangle_key(DP),
                            source._demangle_key(KS))

    def test_the_string_accessors_are_untouched(self):
        for member in ("grouping", "falsename", "truename"):
            self.assertEqual(
                source._demangle_key(
                    f"?do_{member}@?$numpunct@D@std@@MBE{STRING}XZ"),
                f"char@numpunct_do_{member}")
            self.assertEqual(
                source._demangle_key(
                    f"?{member}@?$numpunct@D@std@@QBE{STRING}XZ"),
                f"char@numpunct_{member}")

    def test_the_new_kinds_are_registered_on_both_sides(self):
        for kind in ("NUMPUNCT_DO_DECIMAL_POINT",
                     "NUMPUNCT_DO_THOUSANDS_SEP"):
            self.assertIn(kind, source.COMPGEN_KINDS)
            self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
