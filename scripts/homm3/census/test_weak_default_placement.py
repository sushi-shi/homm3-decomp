"""A placed vtordisp thunk's jump to the weak `??_E` names the scalar `??_G`
it defaults to, so placement reaches the `??_G` body; so does a vtable's
slot 0, which names the weak `??_E` in the compiled table."""
import struct
import unittest

from homm3.build.test_weak_default_normalization import SCALAR, THUNK, WEAK, weak_coff
from homm3.census.placements import _functions_of, _vtable_slots
from homm3.delink.coffx import Obj

VTABLE = "??_7A@@6B@"
DIR32 = 0x06


def vtable_coff(*, define_scalar=True) -> bytes:
    """.text holding `??_G` (when asked) and .rdata holding `??_7A` whose one
    slot relocates to the weak `??_E` defaulting to `??_G`."""
    strings = bytearray(struct.pack("<I", 4))

    def field(name):
        offset = len(strings)
        strings.extend(name.encode("latin-1") + b"\0")
        return struct.pack("<II", 0, offset)

    text, rdata = b"\xc3", bytes(4)
    records = [field(VTABLE) + struct.pack("<IhHBB", 0, 2, 0, 2, 0),
               field(SCALAR) + struct.pack("<IhHBB", 0, 1 if define_scalar else 0, 0x20, 2, 0),
               field(WEAK) + struct.pack("<IhHBB", 0, 0, 0x20, 105, 1),
               struct.pack("<III", 1, 3, 0).ljust(18, b"\0")]
    header_end = 20 + 2 * 40
    text_at = header_end
    rdata_at = text_at + len(text)
    reloc_at = rdata_at + len(rdata)
    symbol_offset = reloc_at + 10
    struct.pack_into("<I", strings, 0, len(strings))
    header = struct.pack("<HHIIIHH", 0x14C, 2, 0, symbol_offset, len(records), 0, 0)
    sections = (struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(text), text_at, 0, 0, 0, 0,
                            0x60500020)
                + struct.pack("<8sIIIIIIHHI", b".rdata", 0, 0, len(rdata), rdata_at, reloc_at,
                              0, 1, 0, 0x40300040))
    relocation = struct.pack("<IIH", 0, 2, DIR32)
    return header + sections + text + rdata + relocation + b"".join(records) + bytes(strings)


def thunk_referent(payload: bytes) -> str:
    rows = {name: relocs for name, _sec, _off, _body, relocs in _functions_of(Obj(payload))}
    ((referent, _kind),) = rows[THUNK].values()
    return referent


class WeakDefaultPlacementTest(unittest.TestCase):
    def test_the_thunk_names_the_defined_scalar_destructor(self):
        self.assertEqual(thunk_referent(weak_coff()), SCALAR)

    def test_an_undefined_default_keeps_the_weak_name(self):
        self.assertEqual(thunk_referent(weak_coff(define_scalar=False)), WEAK)


class WeakVtableSlotTest(unittest.TestCase):
    def test_slot_zero_names_the_defined_scalar_destructor(self):
        self.assertEqual(_vtable_slots(Obj(vtable_coff())), {VTABLE: [SCALAR]})

    def test_an_undefined_default_keeps_the_weak_name(self):
        self.assertEqual(_vtable_slots(Obj(vtable_coff(define_scalar=False))), {VTABLE: [WEAK]})


if __name__ == "__main__":
    unittest.main()
