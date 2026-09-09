"""Keep the retained one-argument char resize distinct from its overload."""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


class BasicStringResizeKeyTest(unittest.TestCase):
    def test_one_argument_char_resize_maps_to_direct_claim(self):
        symbol = ("?resize@?$basic_string@DU?$char_traits@D@std@@"
                  "V?$allocator@D@2@@std@@QAEXI@Z")
        self.assertEqual(source._demangle_key(symbol),
                         "char@basic_string_resize")

    def test_two_argument_and_wide_overloads_do_not_share_the_key(self):
        symbols = [
            "?resize@?$basic_string@DU?$char_traits@D@std@@"
            "V?$allocator@D@2@@std@@QAEXID@Z",
            "?resize@?$basic_string@GU?$char_traits@G@std@@"
            "V?$allocator@G@2@@std@@QAEXI@Z",
        ]
        for symbol in symbols:
            with self.subTest(symbol=symbol):
                self.assertNotEqual(source._demangle_key(symbol),
                                    "char@basic_string_resize")

    def test_kind_is_a_direct_symbol_claim(self):
        kind = "BASIC_STRING_RESIZE"
        self.assertIn(kind, source.COMPGEN_KINDS)
        self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)
        self.assertNotIn(kind, source.ANONYMOUS_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
