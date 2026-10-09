"""A catch handler folds into its parent only when the parent's own FuncInfo
names it and it directly follows (or sits inside) the parent's extent."""
import unittest
from types import SimpleNamespace

from homm3.census import functions


class _Census:
    """The attributes catch_folds and partition read: starts, funclets,
    stubs, reached, tables, insn, byte and text_hi."""

    def __init__(self, bodies, funclets, padding=(), stubs=(), text_hi=0x200):
        # bodies: start -> (first insn, end): one-byte instructions
        self.starts = {s: "test" for s in bodies}
        self.reached = {s: set(range(lo, hi)) for s, (lo, hi) in bodies.items()}
        self.funclets = funclets
        self.stubs = dict.fromkeys(stubs, 0)
        self.tables = {}
        self.text_hi = text_hi
        self._pad = set(padding)

    def insn(self, _rva):
        return SimpleNamespace(size=1)

    def byte(self, rva):
        return 0xCC if rva in self._pad else 0x55


class CatchFoldTest(unittest.TestCase):
    def test_adjacent_and_chained_handlers_fold(self):
        c = _Census({0x10: (0x10, 0x20), 0x22: (0x22, 0x28), 0x28: (0x28, 0x30),
                     0x40: (0x40, 0x48)},
                    {0x22: (0x900, "catch", 0), 0x28: (0x900, "catch", 1)},
                    padding=(0x20, 0x21, *range(0x30, 0x40)))
        folds, refused = functions.catch_folds(c, {0x900: 0x10})
        self.assertEqual(folds, {0x22: 0x10, 0x28: 0x10})
        self.assertEqual(refused, [])
        rows = functions.partition(c, folds)
        self.assertEqual([(a, n) for a, n, _r in rows], [(0x10, 0x20), (0x40, 0x1c0)])

    def test_embedded_handler_folds(self):
        # the parent's decoded body runs on past the handler it embeds
        c = _Census({0x10: (0x10, 0x30), 0x18: (0x18, 0x20)},
                    {0x18: (0x900, "catch", 0)})
        folds, refused = functions.catch_folds(c, {0x900: 0x10})
        self.assertEqual(folds, {0x18: 0x10})

    def test_foreign_or_detached_handlers_stay(self):
        c = _Census({0x10: (0x10, 0x20), 0x24: (0x24, 0x28), 0x30: (0x30, 0x38),
                     0x40: (0x40, 0x48)},
                    {0x24: (0x900, "catch", 0), 0x40: (0x900, "catch", 1),
                     0x30: (0x901, "unwind", 0)})
        folds, refused = functions.catch_folds(c, {0x900: 0x10})
        self.assertEqual(folds, {})
        self.assertEqual([(h, p) for h, p, _why in refused], [(0x24, 0x10), (0x40, 0x10)])

    def test_handler_without_parent_stays(self):
        c = _Census({0x10: (0x10, 0x20), 0x20: (0x20, 0x28)},
                    {0x20: (0x900, "catch", 0)})
        folds, refused = functions.catch_folds(c, {})
        self.assertEqual(folds, {})
        self.assertEqual(refused[0][:2], (0x20, None))


if __name__ == "__main__":
    unittest.main()
