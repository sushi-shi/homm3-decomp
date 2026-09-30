"""Keep ctype<char>'s SDK cleanup claim tied to its exact static signature."""

import unittest

from homm3.compare.canonicalize import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


class CtypeTermKeyTest(unittest.TestCase):
    def test_char_cleanup_has_a_source_claim_key(self):
        self.assertEqual(source._demangle_key("?_Term@?$ctype@D@std@@KAXXZ"),
                         "char@ctype_term")

    def test_other_specialization_and_signatures_stay_distinct(self):
        for symbol in ("?_Term@?$ctype@G@std@@KAXXZ",
                       "?_Term@?$ctype@D@std@@KAEXZ",
                       "?_Term@?$ctype@D@std@@KAXH@Z",
                       "?_Term@?$codecvt@DDH@std@@KAXXZ"):
            with self.subTest(symbol=symbol):
                self.assertNotEqual(source._demangle_key(symbol),
                                    "char@ctype_term")

    def test_claim_uses_the_existing_emitted_symbol(self):
        self.assertIn("CTYPE_TERM", source.COMPGEN_KINDS)
        self.assertIn("CTYPE_TERM", DIRECT_SYMBOL_COMPGEN_KINDS)
        self.assertNotIn("CTYPE_TERM", source.ANONYMOUS_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
