"""Negative control for the ICF oracle - the last resort in `join_unit`.

A class template whose body does not depend on its argument emits one
COMDAT per instantiation, all with the same content AND the same
relocations, and the retail link folds them onto ONE row.
`~TResourceHandle` is the case that forced this: ai_tactical.obj and
army.obj each emit

    ??1?$TResourceHandle@VCSprite@@@@QAE@XZ   12 B
    ??1?$TResourceHandle@Vsample@@@@QAE@XZ    12 B

as the same six instructions with NO relocation at all (`mov ecx,[ecx] /
test / je / mov eax,[ecx] / jmp [eax+4]` - the element reaches the body
only through a virtual slot), against a single 12-byte retail row at
0x43cb10.  Every older oracle declines:

  * the positional zip never runs (two names, one claim - a count
    mismatch, not an order question);
  * `_size_pairing` is not reached for the same reason, and could not
    split identical bodies anyway;
  * the count-mismatch fallback wants an EXACT content match unique in
    both directions, and here both names fit or neither does.

So the claim banked 0.0000 with the ratchet clean.  The oracle answers the
question by dissolving it: when the twins are the same bytes, the row IS
both functions and one claim binds it, the rest recorded as aliases.

THE DIGEST MUST COVER RELOCATIONS, and the counter-example is exactly the
pair this test was first written around.  remote.obj emits

    ??_G?$CAutoArray@VCDPlaySession@@@@UAEPAXI@Z   112 B
    ??_G?$CAutoArray@VCDPlayPlayer@@@@UAEPAXI@Z    112 B

with identical section BYTES, because the one thing separating them - the
vftable each stores - is a relocation whose field is zero in the object.
They are NOT folded: the two vftables 0x6400d8 and 0x640f24 differ in
exactly one slot, their own `??_G`, which is what keeps both the bodies
and the tables distinct.  A content-only digest read them as twins and
bound the claim to `CDPlaySession`, a spelling already proven at 0x512670
in multiplayerwindow - and the delinker refused the merge.  That pair is
pinned below as the negative control on the digest itself.

The controls also pin the oracle's narrowness, because a first-name
fallback that fired on ANY count mismatch would silently mis-bind real
overload groups: two claims decline, one name declines, and a group
holding two DIFFERENT digests declines however tempting its shape.
"""

import unittest

from homm3.retail_labels import source


SESSION = "??_G?$CAutoArray@VCDPlaySession@@@@UAEPAXI@Z"
PLAYER = "??_G?$CAutoArray@VCDPlayPlayer@@@@UAEPAXI@Z"
GROUP = [(SESSION, 0x70), (PLAYER, 0x70)]
FOLDED = {SESSION: "d0f8", PLAYER: "d0f8"}
DISTINCT = {SESSION: "d0f8", PLAYER: "51ab"}


def claim(rva, size=0x6C):
    return {"rva": rva, "size": size, "channel": "src-VA_COMPGEN",
            "name": "__h3cg$remote$scalar_deleting_dtor$CAutoArray"}


class IcfGroupPairingTest(unittest.TestCase):
    def test_the_folded_twins_bind_one_claim(self):
        rows = [claim(0x158490)]
        self.assertEqual(source._icf_group_pairing(rows, GROUP, FOLDED),
                         {0x158490: SESSION})
        self.assertEqual(rows[0]["icf_aliases"], [PLAYER])

    def test_the_older_oracles_could_not_have_done_it(self):
        # the control on the control, both ways: the sizes are equal, so
        # even a two-claim group has two perfect matchings...
        self.assertIsNone(source._size_pairing(
            [{"rva": 0x1000, "size": 0x70}, {"rva": 0x2000, "size": 0x70}],
            GROUP))
        # ...and the claim's own extent (108 B) fits NEITHER base symbol
        # (112 B), which is what the count-mismatch fallback needs.
        self.assertNotIn(claim(0x158490)["size"],
                         [content for _n, content in GROUP])

    def test_two_different_bodies_still_decline(self):
        # a real ambiguity: two names, two digests, one claim. Binding the
        # first would be a coin flip, so the group stays labeled.
        self.assertEqual(
            source._icf_group_pairing([claim(0x158490)], GROUP, DISTINCT), {})

    def test_two_claims_decline(self):
        # two retail rows against two identical bodies is an ORDER
        # question the zip above already owns; this oracle must not race it
        self.assertEqual(
            source._icf_group_pairing([claim(0x158490), claim(0x1583b0)],
                                      GROUP, FOLDED), {})

    def test_a_single_name_declines(self):
        # nothing was folded, so there is nothing for this oracle to say -
        # the count-mismatch size fallback owns a one-against-one group
        self.assertEqual(
            source._icf_group_pairing([claim(0x158490)],
                                      [(SESSION, 0x70)], FOLDED), {})

    def test_an_empty_group_declines(self):
        self.assertEqual(
            source._icf_group_pairing([claim(0x158490)], [], FOLDED), {})

    def test_no_claims_decline(self):
        self.assertEqual(source._icf_group_pairing([], GROUP, FOLDED), {})

    def test_a_missing_digest_declines(self):
        # an unreadable section must never read as "all the same"
        self.assertEqual(
            source._icf_group_pairing([claim(0x158490)], GROUP,
                                      {SESSION: "", PLAYER: ""}), {})
        self.assertEqual(
            source._icf_group_pairing([claim(0x158490)], GROUP, {}), {})

    def test_three_folded_names_bind_and_alias_the_rest(self):
        third = "??_G?$CAutoArray@VCDPlayGroup@@@@UAEPAXI@Z"
        rows = [claim(0x158490)]
        self.assertEqual(
            source._icf_group_pairing(rows, GROUP + [(third, 0x70)],
                                      dict(FOLDED, **{third: "d0f8"})),
            {0x158490: SESSION})
        self.assertEqual(rows[0]["icf_aliases"], [PLAYER, third])


SPRITE = "??1?$TResourceHandle@VCSprite@@@@QAE@XZ"
SAMPLE = "??1?$TResourceHandle@Vsample@@@@QAE@XZ"


class BaseAuthorityDigestTest(unittest.TestCase):
    """The side channel the oracle reads, against the real base object."""

    def test_the_reloc_free_twins_hash_equal(self):
        digests = source._base_authority_digests("ai_tactical")
        if not digests:
            self.skipTest("ai_tactical.obj is not built")
        self.assertTrue(digests[SPRITE])
        self.assertEqual(digests[SPRITE], digests[SAMPLE])

    def test_the_relocation_separates_the_cautoarray_pair(self):
        # NEGATIVE CONTROL ON THE DIGEST. Same bytes, different vftable
        # relocation, two distinct retail rows - digesting content alone
        # bound this claim to a name proven at another address.
        digests = source._base_authority_digests("remote")
        if not digests:
            self.skipTest("remote.obj is not built")
        self.assertTrue(digests[SESSION])
        self.assertNotEqual(digests[SESSION], digests[PLAYER])
        # ...and a DIFFERENT member of the same template differs too
        self.assertNotEqual(
            digests[SESSION],
            digests["??1?$CAutoArray@VCDPlaySession@@@@UAE@XZ"])

    def test_the_groups_half_is_unchanged(self):
        self.assertEqual(source._base_authority_names("remote"),
                         source._base_authority_scan("remote")[0])


if __name__ == "__main__":
    unittest.main()
