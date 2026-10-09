"""A piece the census cut off at a jump target joins the function before it
only when it directly follows it unaligned, the function jumps or falls into
it and nothing outside the function references it."""
import struct
import unittest
from types import SimpleNamespace

from homm3.census import functions

BASE = 0x400000
TEXT, RDATA = 0x1000, 0x2000


class _Image:
    image_base = BASE

    def __init__(self, text, rdata=b"\0" * 16):
        self.sections = [
            SimpleNamespace(name=".text", rva=TEXT, size=len(text), raw_offset=0,
                            executable=True),
            SimpleNamespace(name=".rdata", rva=RDATA, size=len(rdata),
                            raw_offset=len(text), executable=False),
        ]
        self.data = bytes(text) + bytes(rdata)

    def blob(self, sec):
        return self.data[sec.raw_offset:sec.raw_offset + sec.size]

    def section_of(self, rva):
        return next((s for s in self.sections if s.rva <= rva < s.rva + s.size), None)

    def in_image(self, va):
        return self.section_of(va - BASE) is not None


# 0x1000 push ebp; mov ebp, esp; test eax, eax; je 0x100c; mov eax, 1
# 0x100c pop ebp; ret                     (the piece: unaligned, fallen into)
# 0x1010 the next function
BODY = bytes.fromhex("558bec85c07405b801000000" "5dc3" "cccc")


def census(text, starts, rdata=b"\0" * 16):
    c = functions.Census(_Image(text, rdata))
    c.starts = dict(starts)
    for s in c.starts:
        c.reached[s] = c.descend(s)[0]
    return c


class TailJoinTest(unittest.TestCase):
    def test_fallen_into_piece_joins(self):
        c = census(BODY + bytes.fromhex("c3"), {0x1000: "call", 0x100c: "tail", 0x1010: "call"})
        merged, refused = functions.merges(c, {}, tails=True)
        self.assertEqual(merged, {0x100c: 0x1000})
        self.assertEqual(refused, [])
        rows = functions.partition(c, merged)
        self.assertEqual([(a, n) for a, n, _r in rows], [(0x1000, 0xe), (0x1010, 1)])

    def test_called_piece_stays(self):
        # 0x1010 call 0x100c; ret
        c = census(BODY + bytes.fromhex("e8f7ffffffc3"),
                   {0x1000: "call", 0x100c: "tail", 0x1010: "call"})
        merged, refused = functions.merges(c, {}, tails=True)
        self.assertEqual(merged, {})
        self.assertEqual(refused, [(0x100c, 0x1000, "tail", "call at 0x1010 in 0x1010")])

    def test_data_reference_keeps_piece(self):
        rdata = struct.pack("<I", BASE + 0x100c) + b"\0" * 12
        c = census(BODY + bytes.fromhex("c3"),
                   {0x1000: "call", 0x100c: "tail", 0x1010: "call"}, rdata)
        merged, refused = functions.merges(c, {}, tails=True)
        self.assertEqual(merged, {})
        self.assertEqual(refused[0][3], "data reference at 0x2000")

    def test_aligned_piece_stays_on_an_aligned_image(self):
        # 0x1000 test eax, eax; je 0x1010; 12 nops; 0x1010 ret
        text = bytes.fromhex("85c0740c") + b"\x90" * 12 + bytes.fromhex("c3")
        starts = {0x1000: "call", 0x1010: "tail"}
        merged, refused = functions.merges(census(text, starts), {}, tails=True, alignment=16)
        self.assertEqual((merged, refused[0][3]), ({}, "starts on a 16-byte boundary"))
        merged, _refused = functions.merges(census(text, starts), {}, tails=True)
        self.assertEqual(merged, {0x1010: 0x1000})

    def test_switch_off_keeps_piece(self):
        c = census(BODY + bytes.fromhex("c3"), {0x1000: "call", 0x100c: "tail", 0x1010: "call"})
        self.assertEqual(functions.merges(c, {}), ({}, []))


if __name__ == "__main__":
    unittest.main()
