"""Negative control for the overload-group size oracle.

`join_unit` pairs a claim group with its base object's mangled names by
DEFINITION order.  That premise breaks the moment a compiler-generated
member joins the group, and when it breaks it swaps two real names of two
real symbols - a defect nothing downstream can see, because both halves
stay plausible and the ratchet keeps scoring by name.  The regression that
motivated this file put `TGzInflateBuf::TDataError`'s 175-byte default ctor
(0x4d65e0) under the copy ctor's mangled name and its 343-byte copy ctor
(0x4d6690) under the default's, losing 518 banked bytes silently.

The gate must therefore be able to FAIL: each case below is a defect the
oracle has to detect, or a shape it must decline to judge.
"""

import unittest

from homm3.retail_labels import source


def claims(*sizes):
    return [{"rva": 0x1000 + i * 0x100, "size": size}
            for i, size in enumerate(sizes)]


class SizePairingTest(unittest.TestCase):
    #: The real gzinflatebuf group: claims in rva order are the 175-byte
    #: default ctor then the 343-byte copy ctor, while the base obj emits
    #: the compiler-generated copy ctor FIRST.
    DEFAULT_CTOR = "??0TDataError@TGzInflateBuf@@QAE@XZ"
    COPY_CTOR = "??0TDataError@TGzInflateBuf@@QAE@ABV01@@Z"
    GROUP = [(COPY_CTOR, 343), (DEFAULT_CTOR, 175)]

    def test_compiler_generated_member_out_of_definition_order(self):
        self.assertEqual(
            source._size_pairing(claims(175, 343), self.GROUP),
            [self.DEFAULT_CTOR, self.COPY_CTOR],
        )

    def test_the_zip_would_have_swapped_them(self):
        # the control on the control: the premise this oracle overrides
        # really does produce the wrong answer on this group
        self.assertEqual([name for name, _c in self.GROUP],
                         [self.COPY_CTOR, self.DEFAULT_CTOR])

    def test_definition_order_group_is_confirmed_not_disturbed(self):
        group = [("??0T@@QAE@XZ", 8), ("??0T@@QAE@H@Z", 16)]
        self.assertEqual(source._size_pairing(claims(8, 16), group),
                         ["??0T@@QAE@XZ", "??0T@@QAE@H@Z"])

    def test_equal_sizes_are_ambiguous_and_decline(self):
        group = [("??0T@@QAE@XZ", 8), ("??0T@@QAE@H@Z", 8)]
        self.assertIsNone(source._size_pairing(claims(8, 8), group))

    def test_a_claim_no_symbol_fits_declines(self):
        # an unmatched reconstruction: its retail extent is nobody's
        # compiled length, so the sizes decide nothing
        group = [("??0T@@QAE@XZ", 8), ("??0T@@QAE@H@Z", 16)]
        self.assertIsNone(source._size_pairing(claims(8, 99), group))

    def test_oversized_group_declines_rather_than_brute_forcing(self):
        big = source.MAX_SIZE_PAIRING_GROUP + 1
        group = [(f"??0T@@QAE@{i}@Z", i) for i in range(big)]
        self.assertIsNone(source._size_pairing(claims(*range(big)), group))

    def test_empty_group_declines(self):
        self.assertIsNone(source._size_pairing([], []))


if __name__ == "__main__":
    unittest.main()
