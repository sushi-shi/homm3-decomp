"""A data pointer to the code after a catch funclet's rethrow call starts a
function; one after a call in a parent body does not."""
import unittest
from types import SimpleNamespace

from homm3.census import functions


class RethrowSeedsTest(unittest.TestCase):
    def census(self):
        c = SimpleNamespace(starts={0x1000: "call", 0x1100: "funclet:catch"})
        return c

    def test_pointer_after_a_funclet_call_is_a_seed(self):
        c = self.census()
        covered = {0x1105: 0x1100, 0x110a: 0x1100}
        ends = {0x110a: 0x1105}
        self.assertEqual(functions.rethrow_seeds(c, [(0x110a, 0x2000)], covered, ends),
                         [(0x110a, 0x2000)])

    def test_pointer_after_a_parent_call_is_not(self):
        c = self.census()
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        self.assertEqual(functions.rethrow_seeds(c, [(0x100a, 0x2000)], covered, ends), [])

    def test_pointer_not_after_a_call_is_not(self):
        c = self.census()
        covered = {0x110a: 0x1100}
        self.assertEqual(functions.rethrow_seeds(c, [(0x110a, 0x2000)], covered, {}), [])


if __name__ == '__main__':
    unittest.main()
