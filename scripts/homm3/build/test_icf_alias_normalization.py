"""ICF twin references compare by name only when their bodies are identical."""
import struct
import unittest

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalize_objs import (FunctionBody, _canonicalize_icf_aliases,
                                        _icf_identical)

TEXT = 0x60500020
REL32 = 0x14


def coff(text: bytes, functions, relocations, externs=()) -> bytes:
    """One .text section; functions are (name, offset); relocations are
    (site, symbol name); externs are undefined symbol names."""
    names = [name for name, _offset in functions] + list(externs)
    strings = bytearray(struct.pack("<I", 4))

    def field(name):
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            return raw.ljust(8, b"\0")
        offset = len(strings)
        strings.extend(raw + b"\0")
        return struct.pack("<II", 0, offset)

    header_end = 20 + 40
    reloc_offset = header_end + len(text)
    symbol_offset = reloc_offset + 10 * len(relocations)
    index = {name: position for position, name in enumerate(names)}
    records = b"".join(struct.pack("<IIH", site, index[name], REL32)
                       for site, name in relocations)
    symbols = b""
    for name, offset in functions:
        symbols += field(name) + struct.pack("<IhHBB", offset, 1, 0x20, 2, 0)
    for name in externs:
        symbols += field(name) + struct.pack("<IhHBB", 0, 0, 0x20, 2, 0)
    struct.pack_into("<I", strings, 0, len(strings))
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, symbol_offset, len(names), 0, 0)
    section = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(text), header_end,
                          reloc_offset, 0, len(relocations), 0, TEXT)
    return header + section + text + records + symbols + bytes(strings)


CALL = b"\xe8\0\0\0\0\xc3"   # call rel32; ret
JUMP_ONLY = FunctionBody(b"\xe9\0\0\0\0", (1,))
FOLDED = FunctionBody(b"\xe9\0\0\0\0\x90\x90\x90", (1,))


class IcfIdenticalTest(unittest.TestCase):
    def test_relocation_fields_and_linker_fill_are_ignored(self):
        candidate = FunctionBody(b"\x8b\x41\x04\xe9\x01\x02\x03\x04", (4,))
        retail = FunctionBody(b"\x8b\x41\x04\xe9\x00\x00\x00\x00\xcc\xcc", (4,))
        self.assertTrue(_icf_identical(candidate, retail))

    def test_different_code_or_sites_stay_visible(self):
        stores_vtable = FunctionBody(b"\xc7\x01\x00\x00\x00\x00\xe9\x00\x00\x00\x00",
                                     (2, 7))
        self.assertFalse(_icf_identical(stores_vtable, FOLDED))
        self.assertFalse(_icf_identical(FunctionBody(b"\x33\xc0\xc3", ()),
                                        FunctionBody(b"\x33\xc9\xc3", ())))

    def test_trailing_code_is_not_fill(self):
        self.assertFalse(_icf_identical(FunctionBody(b"\xc3", ()),
                                        FunctionBody(b"\xc3\xc3", ())))


class IcfAliasRewriteTest(unittest.TestCase):
    rvas = {"??_GA": (0x10, "func"), "??_GB": (0x20, "func"),
            "??1Folded": (0x30, "func")}

    def test_several_twins_point_at_the_surviving_label(self):
        base = coff(CALL * 2, [("??_GA", 0), ("??_GB", 6)],
                    [(1, "??1TwinA"), (7, "??1TwinB")], ["??1TwinA", "??1TwinB"])
        target = coff(CALL * 2 + FOLDED.payload,
                      [("??_GA", 0), ("??_GB", 6), ("??1Folded", 12)],
                      [(1, "??1Folded"), (7, "??1Folded")])
        index = ({"??1TwinA": JUMP_ONLY, "??1TwinB": JUMP_ONLY},
                 {"??1Folded": FOLDED})
        new_base, new_target, count = _canonicalize_icf_aliases(
            base, target, self.rvas, index)
        self.assertEqual((count, new_target), (2, target))
        parsed = canon.CoffObject(new_base)
        self.assertEqual({parsed.symbols[r.symbol_index].name
                          for r in parsed.relocations}, {"??1Folded"})

    def test_a_twin_with_different_code_stays_visible(self):
        base = coff(CALL, [("??_GA", 0)], [(1, "??1TwinA")], ["??1TwinA"])
        target = coff(CALL, [("??_GA", 0)], [(1, "??1Folded")], ["??1Folded"])
        stores_vtable = FunctionBody(b"\xc7\x01\0\0\0\0\xe9\0\0\0\0", (2, 7))
        index = ({"??1TwinA": stores_vtable}, {"??1Folded": FOLDED})
        self.assertEqual(_canonicalize_icf_aliases(base, target, self.rvas, index),
                         (base, target, 0))

    def test_a_single_twin_names_the_undefined_target_reference(self):
        base = coff(CALL, [("??_GA", 0)], [(1, "??1TwinA")], ["??1TwinA"])
        target = coff(CALL, [("??_GA", 0)], [(1, "??1Folded")], ["??1Folded"])
        index = ({"??1TwinA": JUMP_ONLY}, {"??1Folded": FOLDED})
        new_base, new_target, count = _canonicalize_icf_aliases(
            base, target, self.rvas, index)
        self.assertEqual((new_base, count), (base, 1))
        parsed = canon.CoffObject(new_target)
        self.assertEqual(parsed.symbols[parsed.relocations[0].symbol_index].name,
                         "??1TwinA")

    def test_a_twin_with_its_own_retail_address_stays_visible(self):
        base = coff(CALL, [("??_GA", 0)], [(1, "??1TwinA")], ["??1TwinA"])
        target = coff(CALL, [("??_GA", 0)], [(1, "??1Folded")], ["??1Folded"])
        index = ({"??1TwinA": JUMP_ONLY}, {"??1Folded": FOLDED, "??1TwinA": FOLDED})
        rvas = dict(self.rvas, **{"??1TwinA": (0x40, "func")})
        self.assertEqual(_canonicalize_icf_aliases(base, target, rvas, index),
                         (base, target, 0))

    def test_a_surviving_body_with_a_retail_twin_stays_visible(self):
        base = coff(CALL, [("??_GA", 0)], [(1, "??1TwinA")], ["??1TwinA"])
        target = coff(CALL, [("??_GA", 0)], [(1, "??1Folded")], ["??1Folded"])
        index = ({"??1TwinA": JUMP_ONLY}, {"??1Folded": FOLDED})
        twins = {0x30: (0x50,)}
        self.assertEqual(_canonicalize_icf_aliases(
            base, target, self.rvas, index, lambda rva: twins.get(rva, ())),
            (base, target, 0))


if __name__ == "__main__":
    unittest.main()
