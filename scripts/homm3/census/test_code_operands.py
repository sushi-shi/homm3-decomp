"""Native function boundaries recover operands missed by a linear decoder."""
import unittest
from types import SimpleNamespace

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

from homm3.census import code_operands


class CodeOperandTests(unittest.TestCase):
    def sites(self, payload):
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        decoder.detail = True
        instructions = {ins.address - 0x400000: ins
                        for ins in decoder.disasm(bytes.fromhex(payload), 0x401000)}
        image = SimpleNamespace(image_base=0x400000,
                                section_of=lambda rva: object() if 0x1000 <= rva < 0x3000 else None)
        census = SimpleNamespace(image=image, reached={0x1000: set(instructions)},
                                 insn=instructions.get)
        return code_operands.sites(census)

    def test_absolute_pointer_return(self):
        found = self.sites('b800204000c3')
        self.assertEqual(set(found), {0x1001})
        self.assertEqual(found[0x1001].target, 0x402000)

    def test_absolute_memory_operand(self):
        self.assertEqual(set(self.sites('8b0d00204000c3')), {0x1002})

    def test_relative_call_is_not_an_absolute_site(self):
        self.assertEqual(self.sites('e8fb0f0000c3'), {})

    def test_unmapped_address_like_flag_is_not_a_pointer(self):
        self.assertEqual(self.sites('a900004000c3'), {})

    def test_register_relative_displacement_is_not_an_address(self):
        self.assertEqual(self.sites('8b8100204000c3'), {})


if __name__ == '__main__':
    unittest.main()
