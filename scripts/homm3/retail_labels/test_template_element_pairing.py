"""Negative control for the element oracle behind a shared single-argument
TEMPLATE DESTRUCTOR group.

`_demangle_key`'s generic `??1` arm keeps only a class template's stable
name, so every `??1?$auto_ptr@T@std@@` a TU emits keys to one
`auto_ptr_auto_ptr@dtor` group - the second group this module shares by
construction, after the vector constructors.  forcefeedback.obj emits four
of them and both older oracles fail on it:

  * `_size_pairing` cannot separate `~auto_ptr<CImmEnclosure>` from
    `~auto_ptr<CImmMouse>`: both bodies are the SAME 19 bytes
    (`cmpb $0,(%ecx)` / load the pointer / virtual `delete`), so two
    perfect matchings exist and it declines;
  * the positional zip has nothing to appeal to either, because retail's
    /OPT:ICF folded those twins onto ONE row - four base COMDATs against
    three retail bodies is a count mismatch, not an order question.

With the group unresolved the claim banked 0.0000 with the ratchet clean.
The element the claim's own two-part owner names settles it, and retail
proves the assignment independently: 0x4b7020 is reached from the enclosure
map's teardown, and `~auto_ptr<CImmProject>` (33 B) and `~auto_ptr<char>`
(16 B) are different lengths again.
"""

import unittest

from homm3.retail_labels import source


#: the real forcefeedback group, in the COFF order the base obj emits it,
#: with each symbol's content size
ENCLOSURE = "??1?$auto_ptr@VCImmEnclosure@@@std@@QAE@XZ"
MOUSE = "??1?$auto_ptr@VCImmMouse@@@std@@QAE@XZ"
CHAR = "??1?$auto_ptr@D@std@@QAE@XZ"
PROJECT = "??1?$auto_ptr@VCImmProject@@@std@@QAE@XZ"
COFF_GROUP = [(ENCLOSURE, 0x13), (MOUSE, 0x13), (CHAR, 0x10), (PROJECT, 0x21)]


def claim(rva, owner, size=0x13, unit="forcefeedback"):
    return {"rva": rva, "size": size, "channel": "src-VA_COMPGEN",
            "name": f"__h3cg${unit}$implicit_dtor${owner}"}


class TemplateElementPairingTest(unittest.TestCase):
    def test_the_folded_twin_pairs_by_element(self):
        rows = [claim(0xb7020, "CImmEnclosure_auto_ptr")]
        self.assertEqual(
            source._template_element_pairing(rows, COFF_GROUP),
            {0xb7020: ENCLOSURE})

    def test_the_sizes_could_not_have_done_it(self):
        # the control on the control: the two twins are 0x13 on both sides,
        # so the older oracle finds two perfect matchings and declines
        rows = [{"rva": 0xb7020, "size": 0x13}, {"rva": 0xb7040, "size": 0x13}]
        self.assertIsNone(
            source._size_pairing(rows, [(ENCLOSURE, 0x13), (MOUSE, 0x13)]))

    def test_every_element_spelling_in_the_group_is_reachable(self):
        rows = [claim(0x1000, "CImmEnclosure_auto_ptr"),
                claim(0x2000, "CImmMouse_auto_ptr"),
                claim(0x3000, "char_auto_ptr", 0x10),
                claim(0x4000, "CImmProject_auto_ptr", 0x21)]
        self.assertEqual(
            source._template_element_pairing(rows, COFF_GROUP),
            {0x1000: ENCLOSURE, 0x2000: MOUSE, 0x3000: CHAR,
             0x4000: PROJECT})

    def test_an_unknown_element_declines(self):
        self.assertEqual(
            source._template_element_pairing(
                [claim(0x1000, "CImmJoystick_auto_ptr")], COFF_GROUP), {})

    def test_an_element_naming_two_symbols_declines(self):
        self.assertEqual(
            source._template_element_pairing(
                [claim(0x1000, "CImmEnclosure_auto_ptr")],
                [(ENCLOSURE, 0x13), (ENCLOSURE, 0x13)]), {})

    def test_a_one_part_owner_is_left_to_the_old_key(self):
        # `IMPLICIT_DTOR, CMapHeaderData` and `IMPLICIT_DTOR, map` are
        # ordinary class dtors and must keep keying `<owner>_<owner>@dtor`
        self.assertEqual(
            source._template_element_pairing(
                [claim(0x1000, "CMapHeaderData"), claim(0x2000, "map")],
                COFF_GROUP), {})
        self.assertIsNone(source._template_dtor_owner("CMapHeaderData"))
        self.assertIsNone(source._template_dtor_owner("map"))
        self.assertIsNone(source._template_dtor_owner("auto_ptr"))

    def test_a_claim_of_another_kind_is_left_alone(self):
        row = {"rva": 0x1000, "size": 0x13, "channel": "src-VA_COMPGEN",
               "name": "__h3cg$u$class_ctor$CImmEnclosure_auto_ptr"}
        self.assertEqual(
            source._template_element_pairing([row], COFF_GROUP), {})

    def test_an_empty_group_declines(self):
        self.assertEqual(
            source._template_element_pairing(
                [claim(0x1000, "CImmEnclosure_auto_ptr")], []), {})


#: `CAutoArray`'s two instantiations are the OTHER shape this oracle has to
#: cover, and they are NOT ICF-folded: each `??_G` stores its own vftable,
#: so the two bodies (and their two vftables) stay distinct - retail carries
#: 0x512670 for CDPlaySession and 0x558490 for CDPlayPlayer. One key,
#: two real rows, and the element is the only thing that separates them.
GDTOR_SESSION = "??_G?$CAutoArray@VCDPlaySession@@@@UAEPAXI@Z"
GDTOR_PLAYER = "??_G?$CAutoArray@VCDPlayPlayer@@@@UAEPAXI@Z"
DTOR_SESSION = "??1?$CAutoArray@VCDPlaySession@@@@UAE@XZ"
DTOR_PLAYER = "??1?$CAutoArray@VCDPlayPlayer@@@@UAE@XZ"


class ScalarDeletingDtorElementTest(unittest.TestCase):
    """The same oracle over `??_G`, whose owner casing differs from its
    claim spelling - the template is `CAutoArray`, the owner `cautoarray`."""

    GDTORS = [(GDTOR_SESSION, 0x70), (GDTOR_PLAYER, 0x70)]
    DTORS = [(DTOR_SESSION, 0x60), (DTOR_PLAYER, 0x60)]

    def gclaim(self, rva, owner):
        return {"rva": rva, "size": 0x6C, "channel": "src-VA_COMPGEN",
                "name": f"__h3cg$remote$scalar_deleting_dtor${owner}"}

    def test_the_scalar_deleting_dtor_pairs_by_element(self):
        self.assertEqual(
            source._template_element_pairing(
                [self.gclaim(0x158490, "CDPlayPlayer_CAutoArray")],
                self.GDTORS),
            {0x158490: GDTOR_PLAYER})

    def test_the_ordinary_dtor_of_the_same_template_pairs_too(self):
        self.assertEqual(
            source._template_element_pairing(
                [claim(0x1583b0, "CDPlayPlayer_CAutoArray", 0x54, "remote")],
                self.DTORS),
            {0x1583b0: DTOR_PLAYER})

    def test_a_gdtor_claim_never_reads_the_dtor_group(self):
        # the marker chooses the mangled PREFIX; a ??_G claim offered the
        # ??1 names must find nothing rather than bind the wrong function
        self.assertEqual(
            source._template_element_pairing(
                [self.gclaim(0x158490, "CDPlayPlayer_CAutoArray")],
                self.DTORS), {})
        self.assertEqual(
            source._template_element_pairing(
                [claim(0x1583b0, "CDPlayPlayer_CAutoArray", 0x54, "remote")],
                self.GDTORS), {})

    def test_a_one_part_cautoarray_owner_keeps_the_old_key(self):
        # dxplay and multiplayerwindow claim plain `CAutoArray`; those must
        # keep keying `cautoarray_cautoarray@gdtor` and be left here
        self.assertIsNone(source._template_dtor_owner("cautoarray"))
        self.assertEqual(
            source._template_element_pairing(
                [self.gclaim(0x112670, "CAutoArray")], self.GDTORS), {})

    def test_the_template_casing_is_read_from_the_table(self):
        self.assertEqual(source._template_dtor_owner("CDPlayPlayer_CAutoArray"),
                         ("cdplayplayer", "cautoarray"))
        self.assertEqual(
            source._mangled_template_element(GDTOR_PLAYER, "cautoarray",
                                             "??_G"),
            "cdplayplayer")
        # ...and the lowercased spelling is NOT what the mangled name has,
        # so a table lookup that regressed to the key would return None
        self.assertIsNone(
            source._mangled_template_element(
                "??_G?$cautoarray@VCDPlayPlayer@@@@UAEPAXI@Z", "cautoarray",
                "??_G"))

    def test_both_instantiations_still_share_one_key(self):
        for name in (GDTOR_SESSION, GDTOR_PLAYER):
            self.assertEqual(source._demangle_key(name),
                             "cautoarray_cautoarray@gdtor")
        for name in (DTOR_SESSION, DTOR_PLAYER):
            self.assertEqual(source._demangle_key(name),
                             "cautoarray_cautoarray@dtor")


class TemplateDtorOwnerTest(unittest.TestCase):
    """The two decodes, on their own."""

    def test_the_owner_splits_lowercased(self):
        self.assertEqual(source._template_dtor_owner("CImmEnclosure_auto_ptr"),
                         ("cimmenclosure", "auto_ptr"))
        self.assertEqual(source._template_dtor_owner("char_auto_ptr"),
                         ("char", "auto_ptr"))

    def test_a_bare_template_name_is_not_a_two_part_owner(self):
        # nothing precedes the template, so there is no element to read
        self.assertIsNone(source._template_dtor_owner("_auto_ptr"))

    def test_the_mangled_argument_decodes_both_kinds(self):
        self.assertEqual(
            source._mangled_template_element(ENCLOSURE, "auto_ptr"),
            "cimmenclosure")
        self.assertEqual(source._mangled_template_element(CHAR, "auto_ptr"),
                         "char")

    def test_another_template_and_another_member_decline(self):
        self.assertIsNone(
            source._mangled_template_element(ENCLOSURE, "unique_ptr"))
        self.assertIsNone(source._mangled_template_element(
            "??0?$auto_ptr@VCImmEnclosure@@@std@@QAE@PAVCImmEnclosure@@@Z",
            "auto_ptr"))

    def test_every_instantiation_still_shares_one_key(self):
        for name, _size in COFF_GROUP:
            self.assertEqual(source._demangle_key(name),
                             "auto_ptr_auto_ptr@dtor")


if __name__ == "__main__":
    unittest.main()
