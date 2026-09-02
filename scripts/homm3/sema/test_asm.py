"""Unit tests for code/data boundaries in semantic disassembly."""

import unittest

from homm3.sema import _asm


class CodeInstructionTests(unittest.TestCase):
    def test_trailing_switch_pool_is_not_decoded_as_code(self):
        text = """\
00000000 <fn>:
       0: 75 03\tjne\t0x5
       2: c3\tret
       3: 8b ff\tmov\tedi, edi
       5: 00 00\tadd\tbyte ptr [eax], al
                 00000005:  IMAGE_REL_I386_DIR32 $Lcase
       7: 00 00\tadd\tbyte ptr [eax], al
"""
        self.assertEqual(_asm.code_insns(text),
                         [(0, "jne 0x5"), (2, "ret")])

    def test_operand_relocation_does_not_mark_a_pool(self):
        text = """\
00000000 <fn>:
       0: a1 00 00 00 00\tmov\teax, dword ptr [0x0]
                 00000001:  IMAGE_REL_I386_DIR32 global
       5: c3\tret
"""
        self.assertEqual(_asm.code_insns(text),
                         [(0, "mov eax, dword ptr [0x0]"), (5, "ret")])


class LiteRenderTests(unittest.TestCase):
    """The default `disasm` listing keeps what matching reads and drops
    the objdump columns `--verbose` keeps."""
    LISTING = (
        "00000000 <fn>:\n"
        "       0: e8 00 00 00 00\tcall\t0x5 <fn+0x5>\n"
        "\t\t\t00000001:  IMAGE_REL_I386_REL32\t?callee@@YAXXZ\n"
        "       5: 8b 0d 00 00 00 00\tmov\tecx, dword ptr [0x0]\n"
        "\t\t\t00000007:  IMAGE_REL_I386_DIR32\tgVar\n"
        "       b: 68 00 00 00 00\tpush\t0x0\n"
        "\t\t\t0000000c:  IMAGE_REL_I386_DIR32\tgTable\n"
        "      10: 8b 14 b5 f4 ff ff ff\tmov\tedx, dword ptr [4*esi - 0xc]\n"
        "\t\t\t00000013:  IMAGE_REL_I386_DIR32\tgNames\n"
        "      17: ff 24 8d 00 00 00 00\tjmp\tdword ptr [4*ecx]\n"
        "\t\t\t0000001a:  IMAGE_REL_I386_DIR32\t$L1\n"
        "      1e: 74 02\tje\t0x22 <fn+0x22>\n"
        "      20: 33 c0\txor\teax, eax\n"
        "      22: c3\tret\n"
        "      23: cc\tint3\n"
        "00000024 <$L1>:\n"
        "      24: 00 00\tadd\tbyte ptr [eax], al\n"
        "\t\t\t00000024:  IMAGE_REL_I386_DIR32\t$L2\n"
        "      26: 00 00\tadd\tbyte ptr [eax], al\n"
        "      28: 22 00\tand\tal, byte ptr [eax]\n"
        "\t\t\t00000028:  IMAGE_REL_I386_DIR32\tfn\n"
        "      2a: 00 00\tadd\tbyte ptr [eax], al\n"
        "      2c: 01 02\tadd\tdword ptr [edx], eax\n")

    def test_default_rows_fold_relocs_and_keep_addresses(self):
        self.assertEqual(_asm.lite(self.LISTING), (
            "00000000 <fn>:\n"
            "   0: call ?callee@@YAXXZ\n"
            "   5: mov ecx, dword ptr [gVar]\n"
            "   b: push offset gTable\n"
            "  10: mov edx, dword ptr [4*esi + gNames-0xc]\n"
            "  17: jmp dword ptr [4*ecx + $L1]\n"
            "  1e: je 0x22\n"
            "  20: xor eax, eax\n"
            "  22: ret\n"
            "  23: int3\n"
            "00000024 <$L1>:\n"
            "  24: dd $L2\n"
            "  28: dd fn+0x22\n"
            "  2c: db 01 02\n"))

    def test_default_drops_the_verbose_only_columns(self):
        rendered = _asm.lite(self.LISTING)
        self.assertNotIn("IMAGE_REL", rendered)
        self.assertNotIn("e8 00 00 00 00", rendered)
        self.assertNotIn("<fn+0x5>", rendered)
        self.assertNotIn("<fn+0x22>", rendered)

    def test_unlocatable_reloc_field_falls_back_to_a_note(self):
        text = ("00000000 <fn>:\n"
                "       0: 81 3d 00 00 00 00 00 00 00 00\tcmp\tdword ptr [0x0], 0x0\n"
                "\t\t\t00000002:  IMAGE_REL_I386_DIR32\tgFlag\n"
                "       a: c3\tret\n")
        self.assertEqual(_asm.lite(text), (
            "00000000 <fn>:\n"
            "  0: cmp dword ptr [0x0], 0x0 <gFlag>\n"
            "  a: ret\n"))

    def test_image_notes_survive_except_own_body_offsets(self):
        text = ("00401000 <f>:\n"
                "  401000: e8 0b 00 00 00\tcall\t0x401010 <g>\n"
                "  401005: 7e f9\tjle\t0x401000 <f>\n"
                "  401007: eb 02\tjmp\t0x40100b <f+0xb>\n"
                "  401009: 33 c0\txor\teax, eax\n"
                "  40100b: c3\tret\n")
        self.assertEqual(_asm.lite(text), (
            "00401000 <f>:\n"
            "  401000: call 0x401010 <g>\n"
            "  401005: jle 0x401000 <f>\n"
            "  401007: jmp 0x40100b\n"
            "  401009: xor eax, eax\n"
            "  40100b: ret\n"))

    def test_rows_carry_offsets_for_statement_headings(self):
        rows = _asm.lite_rows(self.LISTING)
        self.assertEqual([offset for offset, _line in rows][:4],
                         [None, 0x0, 0x5, 0xb])
        self.assertEqual(rows[-1][0], 0x2c)


if __name__ == "__main__":
    unittest.main()


class RelocRowsAndCensusTests(unittest.TestCase):
    def test_reloc_rows_pairs_each_instruction_with_its_relocs(self):
        rows = _asm.reloc_rows(LiteRenderTests.LISTING)
        self.assertEqual(len(rows), 14)
        offset, raw, body, relocs = rows[0]
        self.assertEqual((offset, raw, body), (0, b"\xe8\0\0\0\0", "call 0x5 <fn+0x5>"))
        self.assertEqual(relocs, [(1, "REL32", "?callee@@YAXXZ")])
        pool = {row[0]: row for row in rows}
        self.assertEqual(pool[0x24][3], [(0x24, "DIR32", "$L2")])
        self.assertEqual(pool[0x1e][3], [])

    def test_skeleton_census_counts_and_first_marks(self):
        base = [(0, ["push ebp", "cmp eax, 0x1"], "jcc B2 | fall B1"),
                (0x8, ["xor eax, eax"], "fall B2"),
                (0xa, ["pop ebp"], "ret")]
        target = [(0, ["push ebp", "cmp eax, 0x1"], "jcc B2 | fall B1"),
                  (0x8, ["xor eax, eax", "nop"], "fall B2"),
                  (0xb, ["pop ebp"], "ret"),
                  (0xc, ["int3"], "end")]
        census = _asm.skeleton_census(base, target)
        self.assertEqual(census["blocks"], (3, 4))
        self.assertEqual((census["exact"], census["size"], census["shift"],
                          census["flow"], census["missing"]), (2, 1, 0, 0, 1))
        self.assertEqual(census["first_differs"], (1, "size"))
        self.assertIsNone(census["first_flow"])
        self.assertFalse(census["same"])
        text, same = _asm.skeleton_diff(base, target)
        self.assertFalse(same)
        self.assertIn("[skeleton diff: base 3 vs target 4 blocks; 2 exact, 1 size-only, "
                      "0 target-shift, 0 flow-kind, 1 missing]", text)
        self.assertIn("[legend:", text)
        self.assertTrue(_asm.skeleton_census(base, base)["same"])


class LocalRangeTests(unittest.TestCase):
    def test_parse_accepts_disassembly_spelling_and_open_endpoints(self):
        self.assertEqual(_asm.parse_local_range("+0xc00:+0xcdc"),
                         (0xc00, 0xcdc))
        self.assertEqual(_asm.parse_local_range(":0x20"), (None, 0x20))
        self.assertEqual(_asm.parse_local_range("16:"), (16, None))
        for bad in ("", ":", "10", "20:10", "-1:2", "wat:2"):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                _asm.parse_local_range(bad)

    def test_slice_is_local_to_function_origin_and_keeps_reloc(self):
        text = ("00001000 <fn>:\n"
                "    1000: 90\tnop\n"
                "    1001: e8 00 00 00 00\tcall\t0x1006\n"
                "\t\t\t00001002:  IMAGE_REL_I386_REL32\t?callee@@YAXXZ\n"
                "    1006: 75 02\tjne\t0x100a\n"
                "    1008: 33 c0\txor\teax, eax\n"
                "    100a: c3\tret\n")
        sliced = _asm.slice_local_range(text, (1, 8))
        self.assertNotIn("1000: 90", sliced)
        self.assertIn("1001: e8", sliced)
        self.assertIn("IMAGE_REL_I386_REL32\t?callee", sliced)
        self.assertIn("1006: 75", sliced)
        self.assertNotIn("1008: 33", sliced)  # end is origin + 8
        cfg = _asm.cfg(sliced)
        self.assertEqual(cfg[-1][2], "jcc <ext>")

    def test_external_conditional_keeps_its_fallthrough_edge(self):
        text = ("00000000 <selected-range>:\n"
                "       0: 75 08\tjne\t0xa\n"
                "       2: 33 c0\txor\teax, eax\n"
                "       4: c3\tret\n")
        self.assertEqual(_asm.cfg(text)[0][2],
                         "jcc <ext> | fall B1")


class RefreshUnitTests(unittest.TestCase):
    """The in-place unit refresh: ninja target, normalize, report."""

    class _Run:
        def __init__(self, ninja_rc=0, ninja_out="ninja: no work to do.\n"):
            self.calls = []
            self.ninja_rc, self.ninja_out = ninja_rc, ninja_out

        def __call__(self, argv, **kw):
            import subprocess
            self.calls.append(argv)
            if argv[0] == "ninja":
                return subprocess.CompletedProcess(argv, self.ninja_rc, self.ninja_out, "")
            return subprocess.CompletedProcess(argv, 0, "", "")

    def setUp(self):
        import tempfile
        from pathlib import Path
        from homm3.build import normalize_objs
        self.dir = tempfile.TemporaryDirectory()
        ninja = Path(self.dir.name) / "build.ninja"
        ninja.write_text("# fixture\n")
        self._saved = (_asm.NINJA_FILE, _asm.REFRESH_LOCK, normalize_objs.normalize_unit)
        _asm.NINJA_FILE = ninja
        _asm.REFRESH_LOCK = Path(self.dir.name) / "lock"
        self.wrote = 0
        normalize_objs.normalize_unit = lambda unit, symbol_rvas=None: {"wrote": self.wrote}

    def tearDown(self):
        from homm3.build import normalize_objs
        _asm.NINJA_FILE, _asm.REFRESH_LOCK, normalize_objs.normalize_unit = self._saved
        self.dir.cleanup()

    def test_fresh_unit_costs_one_ninja_call_and_no_report(self):
        run = self._Run()
        self.assertIsNone(_asm.refresh_unit("philai", run=run))
        self.assertEqual([c[0] for c in run.calls], ["ninja"])
        self.assertEqual(run.calls[0][-1], "build/objdiff/base/philai.obj")

    def test_recompiled_unit_is_normalized_and_reported(self):
        run = self._Run(ninja_out="[1/1] VC6 philai\n")
        self.wrote = 1
        note = _asm.refresh_unit("philai", run=run)
        self.assertIn("[refreshed philai: compiled + normalized + report", note)
        self.assertEqual([c[0] for c in run.calls], ["ninja", "objdiff-cli"])

    def test_compile_error_dies_with_the_compiler_output(self):
        import contextlib
        import io
        run = self._Run(ninja_rc=1, ninja_out="src/philai.cpp(12) : error C2065: 'x'\n")
        with contextlib.redirect_stderr(io.StringIO()) as err:
            with self.assertRaises(SystemExit) as stop:
                _asm.refresh_unit("philai", run=run)
        self.assertEqual(stop.exception.code, 2)
        self.assertIn("error C2065", err.getvalue())
        self.assertIn("--no-build", err.getvalue())

    def test_no_ninja_graph_means_no_refresh(self):
        _asm.NINJA_FILE = _asm.NINJA_FILE.with_name("absent.ninja")
        run = self._Run()
        self.assertIsNone(_asm.refresh_unit("philai", run=run))
        self.assertEqual(run.calls, [])
