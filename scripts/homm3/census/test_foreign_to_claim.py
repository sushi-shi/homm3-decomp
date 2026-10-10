"""Masked identity never names an address the image's own VA() claims for
a body that reaches other referents."""
import unittest

from homm3.census.placements import foreign_to_claim

BODIES = {
    "claimed": (b"", {0x10: ("dword_writer", 20)}),
    "twin": (b"", {0x10: ("dword_writer", 20)}),
    "other": (b"", {0x10: ("short_writer", 20)}),
}


class ForeignToClaimTest(unittest.TestCase):
    def test_other_referents_are_foreign(self):
        self.assertTrue(foreign_to_claim("other", {"claimed"}, BODIES))

    def test_an_identical_code_twin_is_not(self):
        self.assertFalse(foreign_to_claim("twin", {"claimed"}, BODIES))

    def test_the_claimed_name_and_unclaimed_addresses_are_not(self):
        self.assertFalse(foreign_to_claim("claimed", {"claimed"}, BODIES))
        self.assertFalse(foreign_to_claim("other", set(), BODIES))


if __name__ == "__main__":
    unittest.main()
