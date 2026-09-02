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
