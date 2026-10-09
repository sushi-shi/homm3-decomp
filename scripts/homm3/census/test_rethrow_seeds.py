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


class TableSeedsAfterCallsTest(unittest.TestCase):
    def test_vtable_slot_after_a_call_is_a_seed(self):
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        vtable = {0x3000 + 4 * i for i in range(10)}
        self.assertEqual(functions.table_seeds_after_calls([(0x100a, 0x3000)], covered, ends, vtable),
                         [(0x100a, 0x3000)])

    def test_short_eh_table_slot_is_not(self):
        covered = {0x1005: 0x1000, 0x100a: 0x1000}
        ends = {0x100a: 0x1005}
        scope = {0x3000, 0x3004}
        self.assertEqual(functions.table_seeds_after_calls([(0x100a, 0x3000)], covered, ends, scope), [])

    def test_slot_not_after_a_call_is_not(self):
        covered = {0x100a: 0x1000}
        self.assertEqual(functions.table_seeds_after_calls([(0x100a, 0x3000)], covered, {}, {0x3000}), [])


if __name__ == '__main__':
    unittest.main()
