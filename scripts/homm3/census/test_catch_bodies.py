"""With an image pin's `census_joins_catch_bodies`, a function's code on both
sides of its catch blocks stays one function: the parent's jumps run on past
its own catch funclets and past weak seeds, and a catch block's jump back
into the parent is no tail call."""
import unittest
from types import SimpleNamespace

from homm3.census import functions

# 0x1000 nop                 parent: a loop head at 0x1001
# 0x1001 jmp 0x1006          over its catch block
# 0x1003 jmp 0x1001          catch block: back into the parent's loop
# 0x1005 int3                padding
# 0x1006 ret                 the parent's code after the catch block
CODE = bytes([0x90, 0xEB, 0x03, 0xEB, 0xFC, 0xCC, 0xC3])


class _Image:
    image_base = 0x400000

    def __init__(self, code):
        self.text = SimpleNamespace(name=".text", executable=True, rva=0x1000, size=len(code),
                                    raw_offset=0)
        self.sections = [self.text]
        self.data = code

    def blob(self, section):
        return self.data

    def section_of(self, rva):
        return self.text if 0x1000 <= rva < 0x1000 + len(self.data) else None


def census(catch_bodies):
    c = functions.Census(_Image(CODE), catch_bodies=catch_bodies)
    c.starts = {0x1000: "call", 0x1003: "funclet:catch"}
    c.funclets = {0x1003: (0x900, "catch", 0)}
    return c


class CatchBodiesTest(unittest.TestCase):
    def test_parent_jumps_past_its_catch_block(self):
        seen, _spans, _calls, tails, _imms = census(True).descend(0x1000)
        self.assertIn(0x1006, seen)
        self.assertEqual(tails, set())

    def test_catch_block_jump_back_is_no_tail(self):
        _seen, _spans, _calls, tails, _imms = census(True).descend(0x1003)
        self.assertEqual(tails, set())

    def test_weak_seed_does_not_bound_the_jump(self):
        c = census(True)
        c.starts[0x1005] = "data@0x2000"
        self.assertEqual(c.tail_limit(0x1000), c.text_hi)
        c.starts[0x1005] = "call"
        self.assertEqual(c.tail_limit(0x1000), 0x1005)

    def test_without_the_pin_both_jumps_are_tails(self):
        _seen, _spans, _calls, tails, _imms = census(False).descend(0x1000)
        self.assertEqual(tails, {0x1006})
        _seen, _spans, _calls, tails, _imms = census(False).descend(0x1003)
        self.assertEqual(tails, {0x1001})


if __name__ == "__main__":
    unittest.main()
