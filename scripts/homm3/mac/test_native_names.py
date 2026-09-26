"""Native metadata repairs MWLink labels without changing code identity or bytes."""
import struct
import unittest
from homm3.mac.object import ObjectError, native_symbol_names, parse_code_hunks, restore_listing_names


def native_object(names):
    data = bytearray(68)
    data[:8] = b"MWOBPPC "
    struct.pack_into(">I", data, 40, 48)
    data[48:52] = b"POWR"
    struct.pack_into(">II", data, 60, 20, len(names) + 1)
    for name in names:
        data.extend(b"\x00\xa7" + name.encode("ascii") + b"\0")
    return bytes(data)


class NativeNames(unittest.TestCase):
    def test_indexed_repair_preserves_body_and_relocation_offsets(self):
        full = ".fillLinesVector__4fontF" + "a" * 180
        shown = full[:190] + "\ufffd"
        listing = ('Names:\n\t    1: ' + shown + '\n'
                   'Hunk: Kind=HUNK_GLOBAL_CODE Class=PR Name="' + shown + '"(1) Size=4\n'
                   '00000000: 4E800020 blr\n'
                   'XRef: Kind=HUNK_XREF_24BIT Offset=$00000000 Name="' + shown + '"(1)\n')
        old = parse_code_hunks(listing)[0]
        new = parse_code_hunks(restore_listing_names(listing, native_object([full])))[0]
        self.assertEqual(new.name, full)
        self.assertEqual(new.data, old.data)
        self.assertEqual(new.xrefs, ((0, "HUNK_XREF_24BIT", full),))

    def test_shared_long_prefix_is_resolved_by_index_not_guessing(self):
        first = ".overloaded__" + "a" * 200 + "Fv"
        second = ".overloaded__" + "a" * 200 + "Fi"
        shown = first[:190] + "\ufffd"
        listing = "\n".join(
            f'Hunk: Kind=HUNK_GLOBAL_CODE Name="{shown}"({index}) Size=4\n'
            '00000000: 4E800020 blr' for index in (1, 2))
        hunks = parse_code_hunks(restore_listing_names(
            listing, native_object([first, second])))
        self.assertEqual([hunk.name for hunk in hunks], [first, second])

    def test_short_listing_is_unchanged(self):
        listing = 'Names:\n\t    1: .foo\nHunk: Kind=HUNK_GLOBAL_CODE Name=".foo"(1) Size=4\n00000000: 4E800020 blr\n'
        self.assertEqual(restore_listing_names(listing, native_object([".foo"])), listing)

    def test_index_disagreement_rejected_even_with_shared_prefix(self):
        prefix = ".overloaded__" + "a" * 200
        with self.assertRaises(ObjectError):
            restore_listing_names('Hunk: Name=".wrong"(1)', native_object([prefix]))
        with self.assertRaises(ObjectError):
            restore_listing_names('Hunk: Name=".foo"(2)', native_object([".foo"]))

    def test_missing_terminator_and_invalid_component_rejected(self):
        data = native_object([".foo"])
        with self.assertRaises(ObjectError):
            native_symbol_names(data[:-1])
        with self.assertRaises(ObjectError):
            native_symbol_names(data[:48] + b"NOPE" + data[52:])


if __name__ == "__main__":
    unittest.main()
