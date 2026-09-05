"""Negative control for the constructor-kind oracle.

`join_unit` settles a count-mismatched claim group by EXACT content size.
That oracle is blind to the commonest shape in this tree: a class whose
default constructor and whose compiler-generated copy constructor share one
`Class_Class` key, where the linker took retail's default ctor from ANOTHER
compiland - so this unit's retail band contains only the copy, and one
claim faces a two-member group.  Sizes cannot decide it, because a claim's
retail extent and our COMDAT's length are not equal for a reconstruction
that has not closed: singleselectionwindow's `NewSMapHeader` copy ctor is
426 bytes in retail against 462 here, and `CMapHeaderData`'s is 701 against
678.

The KINDS decide it instead, and the gate must be able to fail: each case
below is a defect the oracle has to detect, or a shape it must decline to
judge rather than guess at.
"""

import unittest

from homm3.retail_labels import source


def claim(rva, marker, size=0x100, channel="src-VA_COMPGEN"):
    return {"rva": rva, "size": size, "channel": channel,
            "name": f"__h3cg$u{marker}X"}


COPY = "$implicit_copy_ctor$"
PLAIN = "$class_ctor$"

#: the real singleselectionwindow groups, base-obj content sizes
NEWSMAP = [("??0NewSMapHeader@@QAE@XZ", 0x1a3),
           ("??0NewSMapHeader@@QAE@ABV0@@Z", 0x1ce)]
MAPHEADER = [("??0CMapHeaderData@@QAE@XZ", 0x101),
             ("??0CMapHeaderData@@QAE@ABV0@@Z", 0x2a6)]


class CtorKindPairingTest(unittest.TestCase):
    def test_a_lone_copy_claim_takes_the_copy_ctor(self):
        rows = [claim(0x18fa60, COPY, 426)]
        self.assertEqual(source._ctor_kind_pairing(rows, NEWSMAP),
                         {0x18fa60: "??0NewSMapHeader@@QAE@ABV0@@Z"})

    def test_the_sizes_could_not_have_done_it(self):
        # the control on the control: 426 is neither 419 nor 462, so the
        # size oracle declines on exactly this input
        rows = [{"rva": 0x18fa60, "size": 426}]
        self.assertIsNone(source._size_pairing(rows, NEWSMAP))
        self.assertNotIn(426, [size for _n, size in NEWSMAP])

    def test_a_lone_default_claim_takes_the_default_ctor(self):
        rows = [claim(0x190810, PLAIN, 0x119)]
        self.assertEqual(source._ctor_kind_pairing(rows, MAPHEADER),
                         {0x190810: "??0CMapHeaderData@@QAE@XZ"})

    def test_both_halves_pair_at_once(self):
        rows = [claim(0x1000, PLAIN), claim(0x2000, COPY)]
        self.assertEqual(
            source._ctor_kind_pairing(rows, NEWSMAP),
            {0x1000: "??0NewSMapHeader@@QAE@XZ",
             0x2000: "??0NewSMapHeader@@QAE@ABV0@@Z"})

    def test_two_claims_of_one_kind_decline(self):
        rows = [claim(0x1000, COPY), claim(0x2000, COPY)]
        self.assertEqual(source._ctor_kind_pairing(rows, NEWSMAP), {})

    def test_two_symbols_of_one_kind_decline(self):
        group = NEWSMAP + [("??0NewSMapHeader@@QAE@H@Z", 0x40)]
        rows = [claim(0x1000, PLAIN)]
        self.assertEqual(source._ctor_kind_pairing(rows, group), {})

    def test_a_name_the_size_pass_already_took_is_not_offered_again(self):
        rows = [claim(0x1000, COPY)]
        used = {"??0NewSMapHeader@@QAE@ABV0@@Z"}
        self.assertEqual(source._ctor_kind_pairing(rows, NEWSMAP, used), {})

    def test_an_already_joined_claim_is_not_re_paired(self):
        rows = [claim(0x1000, COPY, channel="src-VA+base")]
        self.assertEqual(source._ctor_kind_pairing(rows, NEWSMAP), {})

    def test_an_unmarked_claim_is_left_alone(self):
        rows = [claim(0x1000, "$vector_dtor$")]
        self.assertEqual(source._ctor_kind_pairing(rows, NEWSMAP), {})

    def test_non_constructor_symbols_are_never_offered(self):
        # a dtor or an operator= keys to its own group, but the oracle must
        # not depend on that: only ??0 spellings are candidates
        group = [("??1NewSMapHeader@@QAE@XZ", 0x12e),
                 ("??4NewSMapHeader@@QAEAAV0@ABV0@@Z", 0x257)]
        self.assertEqual(source._ctor_kind_pairing(
            [claim(0x1000, COPY), claim(0x2000, PLAIN)], group), {})

    def test_a_nested_class_copy_ctor_is_recognised(self):
        group = [("??0TPlayerSlotAttributes@CMapHeaderData@@QAE@XZ", 0x45),
                 ("??0TPlayerSlotAttributes@CMapHeaderData@@QAE@ABV01@@Z",
                  0x247)]
        self.assertEqual(
            source._ctor_kind_pairing([claim(0x1000, COPY)], group),
            {0x1000: "??0TPlayerSlotAttributes@CMapHeaderData@@QAE@ABV01@@Z"})

    def test_an_empty_group_declines(self):
        self.assertEqual(source._ctor_kind_pairing([claim(0x1000, COPY)],
                                                   []), {})


if __name__ == "__main__":
    unittest.main()
