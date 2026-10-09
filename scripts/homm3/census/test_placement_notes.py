"""An evidence note names a referencing body the same way in every checkout."""
import unittest

from homm3.census import placements


class ReferrerTest(unittest.TestCase):
    def test_ordinary_name_is_kept(self):
        name = "?insert@?$vector@HV?$allocator@H@std@@@std@@QAEXPAHIABH@Z"
        self.assertEqual(placements.referrer(name, 0x309b9), name)

    def test_anonymous_namespace_name_is_an_address(self):
        short = r"?f@?%Z:\a\GameMap.cpp1@@YAXXZ"
        long = r"?f@?%Z:\home\sheep\Projects\homm3\other-checkout\GameMap.cpp1@@YAXXZ"
        self.assertEqual(placements.referrer(short, 0x309b9), "0x000309b9")
        self.assertEqual(placements.referrer(short, 0x309b9), placements.referrer(long, 0x309b9))


if __name__ == "__main__":
    unittest.main()
