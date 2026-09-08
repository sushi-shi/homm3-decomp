"""Keep the retained char _Copy distinct from wide strings and other members."""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


class BasicStringCopyKeyTest(unittest.TestCase):
    def test_char_copy_maps_to_direct_claim(self):
        symbol = ("?_Copy@?$basic_string@DU?$char_traits@D@std@@"
                  "V?$allocator@D@2@@std@@AAEXI@Z")
        self.assertEqual(source._demangle_key(symbol), "char@basic_string_copy")
        self.assertIn("BASIC_STRING_COPY", source.COMPGEN_KINDS)
        self.assertIn("BASIC_STRING_COPY", DIRECT_SYMBOL_COMPGEN_KINDS)
        self.assertNotIn("BASIC_STRING_COPY", source.ANONYMOUS_COMPGEN_KINDS)

    def test_other_specializations_and_members_do_not_match(self):
        symbols = [
            "?_Copy@?$basic_string@GU?$char_traits@G@std@@"
            "V?$allocator@G@2@@std@@AAEXI@Z",
            "?_Grow@?$basic_string@DU?$char_traits@D@std@@"
            "V?$allocator@D@2@@std@@AAE_NI_N@Z",
        ]
        for symbol in symbols:
            with self.subTest(symbol=symbol):
                self.assertNotEqual(source._demangle_key(symbol),
                                    "char@basic_string_copy")


if __name__ == "__main__":
    unittest.main()
