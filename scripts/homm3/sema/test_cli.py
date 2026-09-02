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

    def test_local_ranges_parse_for_diff_and_disasm(self):
        args = _build_parser().parse_args(
            ["diff", "CastSpell", "--base-range", "+0xb90:+0xc60",
             "--target-range", "+0xc00:+0xcdc"])
        self.assertEqual(args.base_range, "+0xb90:+0xc60")
        self.assertEqual(args.target_range, "+0xc00:+0xcdc")
        args = _build_parser().parse_args(
            ["disasm", "CastSpell", "--range", "+0xc00:+0xcdc"])
        self.assertEqual(args.range, "+0xc00:+0xcdc")



class InventedFlagsTest(unittest.TestCase):
    """The flags agents typed before they existed (the usage-log audit)."""

    def test_aliases_share_their_dest(self):
        parse = _build_parser().parse_args
        self.assertTrue(parse(["diff", "f", "--blocks"]).structure)
        self.assertTrue(parse(["diff", "f", "--skeleton"]).structure)
        self.assertTrue(parse(["disasm", "f", "--candidate"]).base)
        self.assertTrue(parse(["disasm", "f", "--target"]).target_side)
        self.assertEqual(parse(["disasm", "f", "--target"]).target, "f")
        self.assertTrue(parse(["xref", "f", "--calls"]).callees)
        self.assertTrue(parse(["xref", "f", "--to"]).to)

    def test_new_diff_views_parse_alone_and_with_verbose(self):
        parse = _build_parser().parse_args
        for flag, attr in (("--calls", "calls"), ("--relocs", "relocs"),
                           ("--summary", "summary"), ("--why-bytes", "why_bytes")):
            self.assertTrue(getattr(parse(["diff", "f", flag]), attr))
            self.assertTrue(parse(["diff", "f", flag, "--verbose"]).verbose)

    def test_new_diff_views_are_mutually_exclusive(self):
        for pair in (["--calls", "--relocs"], ["--summary", "--why-bytes"],
                     ["--why-bytes", "--source"], ["--blocks", "--calls"]):
            with contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):
                    _build_parser().parse_args(["diff", "f", *pair])



class WrongNamespaceTest(unittest.TestCase):
    def test_vc6_and_guessed_subcommands_name_their_home(self):
        import os
        from pathlib import Path
        from homm3.sema import _common, __main__ as main_module
        saved = _common.LOG
        _common.LOG = Path(os.devnull)
        try:
            for argv, hint in ((["why-reg", "0x1"], "homm3 vc6 why-reg"),
                               (["blocks", "0x1"], "homm3 sema disasm TARGET --blocks"),
                               (["xrefs", "0x1"], "homm3 sema xref")):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    with self.assertRaises(SystemExit) as stop:
                        main_module.main(argv)
                self.assertEqual(stop.exception.code, 2)
                self.assertIn(hint, err.getvalue())
            with contextlib.redirect_stderr(io.StringIO()) as err:
                with self.assertRaises(SystemExit):
                    main_module.main(["nonsense"])
            self.assertIn("not a homm3 sema command", err.getvalue())
        finally:
            _common.LOG = saved


if __name__ == "__main__":
    unittest.main()
