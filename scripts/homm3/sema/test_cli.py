"""CLI contract tests for the sema reconstruction loop."""

import contextlib
import io
import unittest

from homm3.sema.__main__ import _build_parser


class SemaCliTests(unittest.TestCase):
    def test_structure_selects_explicit_default_view(self):
        args = _build_parser().parse_args(
            ["diff", "ProcessCombatMsg", "--structure"])
        self.assertTrue(args.structure)
        self.assertFalse(args.asm)
        self.assertFalse(args.branches)
        self.assertFalse(args.source)

    def test_structure_is_exclusive_with_other_diff_views(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                _build_parser().parse_args(
                    ["diff", "ProcessCombatMsg", "--structure", "--source"])


if __name__ == "__main__":
    unittest.main()
