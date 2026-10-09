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


class DataExtentTest(unittest.TestCase):
    def test_a_compiled_extent_stops_at_the_next_placed_datum(self):
        rows = [(0x70c, 8, "data", "?use", "victor", "-"),
                (0x710, 4, "data", "?create", "victor", "-"),
                (0x100, 0x30, "func", "?f", "victor", "-")]
        self.assertEqual([row[:2] for row in placements.clip_data_extents(rows)],
                         [(0x70c, 4), (0x710, 4), (0x100, 0x30)])


class ScalarSizeTest(unittest.TestCase):
    def test_a_guard_is_one_byte_whatever_follows_it(self):
        guard = ("_?$S27@?1??initializeCreatureTypeTraits@@YAXHABV?$vector@PADV?$allocator"
                 "@PAD@std@@@std@@@Z@4EA")
        self.assertEqual(placements.scalar_size(guard), 1)

    def test_fundamental_types_give_their_size(self):
        self.assertEqual(placements.scalar_size("?blue_mask@TPalette16@@0IA"), 4)
        self.assertEqual(placements.scalar_size("?SaturatedGraphicsEasterEgg@ResourceManager@@3EA"), 1)
        self.assertEqual(placements.scalar_size("?g_flag@@3_NA"), 1)
        self.assertEqual(placements.scalar_size("?g_scale@@3NB"), 8)

    def test_pointers_arrays_and_classes_keep_their_extent(self):
        for name in ("?g_spellTraitsImp@@3PAUTSpellTraits@@A",
                     "_?spellNames@?1??initializeSpellTraits@@YAXXZ@4PAVTAutoStrPtr@?A0x1@@A",
                     "_g_victorLeadingBits$S29135", "??_C@_08OBBL@Acid?4wav?$AA@"):
            self.assertIsNone(placements.scalar_size(name))


if __name__ == "__main__":
    unittest.main()
