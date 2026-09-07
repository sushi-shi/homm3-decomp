"""Negative control for the retained basic_string<char>::max_size COMDAT."""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


class BasicStringMaxSizeKeyTest(unittest.TestCase):
    def test_char_specialization_has_a_claimable_key(self):
        mangled = ("?max_size@?$basic_string@DU?$char_traits@D@std@@"
                   "V?$allocator@D@2@@std@@QBEIXZ")
        self.assertEqual(source._demangle_key(mangled),
                         "char@basic_string_max_size")

    def test_other_specializations_do_not_share_the_key(self):
        wide = ("?max_size@?$basic_string@GU?$char_traits@G@std@@"
                "V?$allocator@G@2@@std@@QBEIXZ")
        self.assertNotEqual(source._demangle_key(wide),
                            "char@basic_string_max_size")

    def test_kind_is_a_direct_symbol_claim(self):
        kind = "BASIC_STRING_MAX_SIZE"
        self.assertIn(kind, source.COMPGEN_KINDS)
        self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)
        self.assertNotIn(kind, source.ANONYMOUS_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
