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


if __name__ == "__main__":
    unittest.main()
