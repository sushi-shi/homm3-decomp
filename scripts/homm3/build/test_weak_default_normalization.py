"""A vtordisp thunk's jump to the weak vector deleting destructor `??_E`
binds to the scalar `??_G` it defaults to, as LINK binds it."""
import struct
import unittest

from homm3.build import canonicalize_data_symbols as canon
from homm3.build.normalize_objs import _resolve_weak_defaults
from homm3.build.test_icf_alias_normalization import coff

REL32 = 0x14
TEXT = 0x60500020
THUNK = "??_EA@@$4PPPPPPPM@A@AEPAXI@Z"
WEAK = "??_EA@@UAEPAXI@Z"
SCALAR = "??_GA@@UAEPAXI@Z"
BODY = b"\x2b\x49\xfc\xe9\0\0\0\0" + b"\xc2\x04\x00"


def weak_coff(*, define_scalar=True) -> bytes:
    """The thunk (defined), the weak `??_E` whose aux tag names an undefined
    `??_G` record, then `??_G` defined after the thunk when asked."""
    strings = bytearray(struct.pack("<I", 4))

    def field(name):
        offset = len(strings)
        strings.extend(name.encode("latin-1") + b"\0")
        return struct.pack("<II", 0, offset)

    records = [field(THUNK) + struct.pack("<IhHBB", 0, 1, 0x20, 2, 0),
               field(SCALAR) + struct.pack("<IhHBB", 0, 0, 0x20, 2, 0),
               field(WEAK) + struct.pack("<IhHBB", 0, 0, 0x20, 105, 1),
               struct.pack("<III", 1, 3, 0).ljust(18, b"\0")]
    if define_scalar:
        records.append(field(SCALAR) + struct.pack("<IhHBB", 8, 1, 0x20, 2, 0))
    header_end = 20 + 40
    reloc_offset = header_end + len(BODY)
    symbol_offset = reloc_offset + 10
    relocation = struct.pack("<IIH", 4, 2, REL32)
    struct.pack_into("<I", strings, 0, len(strings))
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, symbol_offset, len(records), 0, 0)
    section = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(BODY), header_end,
                          reloc_offset, 0, 1, 0, TEXT)
    return header + section + BODY + relocation + b"".join(records) + bytes(strings)


def jump_target(payload: bytes) -> str:
    coff_object = canon.CoffObject(payload)
    (relocation,) = coff_object.relocations
    return coff_object.symbols[relocation.symbol_index].name


class WeakDefaultTest(unittest.TestCase):
    def test_the_thunk_jumps_to_the_defined_scalar_destructor(self):
        target = coff(BODY, [(THUNK, 0), (SCALAR, 8)], [(4, SCALAR)])
        resolved, count = _resolve_weak_defaults(weak_coff(), target)
        self.assertEqual(count, 1)
        self.assertEqual(jump_target(resolved), SCALAR)
        symbol = canon.CoffObject(resolved).symbols[
            canon.CoffObject(resolved).relocations[0].symbol_index]
        self.assertGreater(symbol.section, 0)

    def test_a_target_naming_the_weak_name_keeps_it(self):
        target = coff(BODY, [(THUNK, 0), (WEAK, 8)], [(4, WEAK)])
        base = weak_coff()
        self.assertEqual(_resolve_weak_defaults(base, target), (base, 0))

    def test_an_undefined_default_stays_weak(self):
        target = coff(BODY, [(THUNK, 0), (SCALAR, 8)], [(4, SCALAR)])
        base = weak_coff(define_scalar=False)
        self.assertEqual(_resolve_weak_defaults(base, target), (base, 0))


if __name__ == "__main__":
    unittest.main()
