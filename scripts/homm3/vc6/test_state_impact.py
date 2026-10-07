"""Contract tests for homm3.vc6.state_impact (pure functions; no toolchain)."""
import struct
import unittest

from homm3.vc6 import state_impact as si


class OrderModelTest(unittest.TestCase):
    ORDER = ["_$E1", "f1", "h@inline", "f2", "f3", "g@impl", "f4", "?put@?$T@H@@QAEXH@Z"]
    REFS = {"f1": ["h@inline"], "f3": []}
    OWN = {"f1", "f2", "f3", "f4"}

    def bodies(self):
        return si.classify(self.ORDER, self.REFS, self.OWN)

    def test_classification(self):
        kinds = [(b.name, b.kind) for b in self.bodies()]
        self.assertEqual(kinds, [("_$E1", "initializer"), ("f1", "own"),
                                 ("h@inline", "inline-copy"), ("f2", "own"), ("f3", "own"),
                                 ("g@impl", "inline-copy"), ("f4", "own"),
                                 ("?put@?$T@H@@QAEXH@Z", "deferred")])

    def test_identity_reproduces_the_object_order(self):
        self.assertEqual(si.reorder(self.bodies(), self.REFS, ["f1", "f2", "f3", "f4"]), self.ORDER)

    def test_groups_move_with_their_definition(self):
        new = si.reorder(self.bodies(), self.REFS, ["f3", "f1", "f2", "f4"])
        # the head initializer stays first; f3's group (g@impl) moves with it
        self.assertEqual(new, ["_$E1", "f3", "g@impl", "f1", "h@inline", "f2", "f4",
                               "?put@?$T@H@@QAEXH@Z"])

    def test_moved_function_becomes_first_user_of_its_candidates(self):
        # f2 moves above f3 and can need h@inline: h@inline stays with f1
        # (earlier); a candidate grouped after f3 is re-anchored to f2
        new = si.reorder(self.bodies(), self.REFS, ["f1", "f2", "f3", "f4"], moved="f2",
                         candidates={"f2": {"g@impl"}})
        self.assertEqual(new.index("g@impl"), new.index("f2") + 1)


class EditAndDecodeTest(unittest.TestCase):
    def test_swap_include_keeps_everything_else(self):
        text = '#include "a.h"\nint x;\n#include <b>\nint y;\n'
        self.assertEqual(si.apply_edit(text, "swap-include:a.h:b"),
                         '#include <b>\nint x;\n#include "a.h"\nint y;\n')

    def test_move_block(self):
        text = "VA(0x1)\nA\nVA(0x2)\nB\nVA(0x3)\nC\n"
        self.assertEqual(si.apply_edit(text, "move:0x3:before:0x1"),
                         "VA(0x3)\nC\nVA(0x1)\nA\nVA(0x2)\nB\n")

    def test_immediate_operand(self):
        self.assertTrue(si._immediate_operand("  27: c7 45 fc 00 00 00 00\tmov\tdword ptr [ebp - 0x4], 0x0"))
        self.assertTrue(si._immediate_operand("   5: 68 00 00 00 00\tpush\t0x0"))
        self.assertFalse(si._immediate_operand("  35: 8b 0d 00 00 00 00\tmov\tecx, dword ptr [0x0]"))

    def test_record_handle_forms(self):
        hw = 0x20000
        short = b"\x00\x02" + struct.pack("<H", 0x2864) + b"\x00?a@@3HA\x00"
        self.assertEqual(si.decode_record_handle(short, short.index(b"?"), hw), 0x2864)
        wide = b"\x00\x02" + struct.pack("<I", 0x0001C472) + b"\x00?b@@3HA\x00"
        self.assertEqual(si.decode_record_handle(wide, wide.index(b"?"), hw), 0xC472)
        static = b"\x02\x00" + struct.pack("<H", 0x28B2) + b"$g_tab\x00"
        self.assertEqual(si.decode_record_handle(static, static.index(b"$"), hw), 0x28B2)


if __name__ == "__main__":
    unittest.main()
