"""Negative control for basic_streambuf<char>'s five remaining defaulted
virtuals.

bottomviewsubwindow.obj is the object whose copies of the stream base's
defaulted virtuals the linker kept, and `uflow`, `seekoff` and `seekpos`
were already claimed there. The other five - `overflow`, `showmanyc`,
`underflow`, `setbuf` and `imbue` - sit in the same 0x454050..0x454270 run,
in vtable-slot order, and had no key.

The hazard the arms must detect is a COLLISION rather than a bare miss:
`overflow` and `underflow` already have basic_stringbuf arms, so without a
basic_streambuf arm of their own the BASE class's copies fall through to the
generic template tail as `std_basic_streambuf_overflow` - a spelling no
VA_COMPGEN owner can produce, which reaches the join as an unjoinable row
that banks 0.0000 with the ratchet clean.

Every case below is either that defect or an established key the new arms
must leave alone.
"""

import unittest

from homm3.build.canonicalize_data_symbols import DIRECT_SYMBOL_COMPGEN_KINDS
from homm3.retail_labels import source


TRAITS = "U?$char_traits@D@std@@"
STREAMBUF = f"?$basic_streambuf@D{TRAITS}@std@@"
STRINGBUF = f"?$basic_stringbuf@D{TRAITS}V?$allocator@D@2@@std@@"

#: (member, mangled signature tail, expected key suffix)
DEFAULTED_VIRTUALS = (
    ("overflow", "MAEHH@Z", "streambuf_overflow"),
    ("showmanyc", "MAEHXZ", "streambuf_showmanyc"),
    ("underflow", "MAEHXZ", "streambuf_underflow"),
    ("setbuf", "MAEPAV12@PADH@Z", "streambuf_setbuf"),
    ("imbue", "MAEXABVlocale@2@@Z", "streambuf_imbue"),
)


class StreambufDefaultVirtualKeyTest(unittest.TestCase):
    def test_each_defaulted_virtual_gets_its_own_key(self):
        # THE defect: without its own arm each of these falls through to the
        # generic template tail, which no VA_COMPGEN owner can spell.
        for member, tail, expected in DEFAULTED_VIRTUALS:
            key = source._demangle_key(f"?{member}@{STREAMBUF}{tail}")
            self.assertEqual(key, f"char@{expected}", member)
            self.assertNotEqual(key, f"std_basic_streambuf_{member}", member)

    def test_the_keys_are_distinct(self):
        keys = {source._demangle_key(f"?{m}@{STREAMBUF}{t}")
                for m, t, _ in DEFAULTED_VIRTUALS}
        self.assertEqual(len(keys), len(DEFAULTED_VIRTUALS))

    def test_the_stringbuf_overrides_keep_their_own_keys(self):
        # THE second defect: an arm keyed on the bare member name would
        # swallow basic_stringbuf's overrides, which are separate retail rows
        # already claimed in resourcemanager.
        self.assertEqual(
            source._demangle_key(f"?overflow@{STRINGBUF}MAEHH@Z"),
            "char@stringbuf_overflow")
        self.assertEqual(
            source._demangle_key(f"?underflow@{STRINGBUF}MAEHXZ"),
            "char@stringbuf_underflow")
        self.assertNotEqual(
            source._demangle_key(f"?overflow@{STREAMBUF}MAEHH@Z"),
            source._demangle_key(f"?overflow@{STRINGBUF}MAEHH@Z"))
        self.assertNotEqual(
            source._demangle_key(f"?underflow@{STREAMBUF}MAEHXZ"),
            source._demangle_key(f"?underflow@{STRINGBUF}MAEHXZ"))

    def test_the_neighbouring_streambuf_keys_are_unmoved(self):
        for member, tail, expected in (
                ("uflow", "MAEHXZ", "streambuf_uflow"),
                ("seekoff", "MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z",
                 "streambuf_seekoff"),
                ("seekpos", "MAE?AV?$fpos@H@2@V32@H@Z", "streambuf_seekpos")):
            self.assertEqual(
                source._demangle_key(f"?{member}@{STREAMBUF}{tail}"),
                f"char@{expected}", member)

    def test_the_kinds_are_direct_symbol_kinds(self):
        # Both halves of the partition must agree, or the claim reaches the
        # join as an anonymous rename target instead of a name-authority row.
        for _member, _tail, expected in DEFAULTED_VIRTUALS:
            kind = expected.upper()
            self.assertIn(kind, source.COMPGEN_KINDS)
            self.assertIn(kind, DIRECT_SYMBOL_COMPGEN_KINDS)
            self.assertNotIn(kind, source.ANONYMOUS_COMPGEN_KINDS)


if __name__ == "__main__":
    unittest.main()
