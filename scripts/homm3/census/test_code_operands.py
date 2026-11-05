"""Native function boundaries recover operands missed by a linear decoder."""
import unittest
from types import SimpleNamespace

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

from homm3.census import code_operands


class CodeOperandTests(unittest.TestCase):
    def census(self, payload, tables=None, words=None):
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        decoder.detail = True
        instructions = {ins.address - 0x400000: ins
                        for ins in decoder.disasm(bytes.fromhex(payload), 0x401000)}
        image = SimpleNamespace(image_base=0x400000,
                                in_image=lambda value: 0x400000 <= value < 0x403000)
        return SimpleNamespace(image=image, reached={0x1000: set(instructions)},
                               insn=instructions.get, tables=tables or {},
                               dword=(words or {}).get,
                               in_text=lambda rva: 0x1000 <= rva < 0x2000)

    def sites(self, payload, **kwargs):
        return code_operands.sites(self.census(payload, **kwargs))

    def test_absolute_pointer_return(self):
        found = self.sites('b800204000c3')
        self.assertEqual(set(found), {0x1001})
        self.assertEqual(found[0x1001].target, 0x402000)

    def test_absolute_memory_operand(self):
        self.assertEqual(set(self.sites('8b0d00204000c3')), {0x1002})

    def test_relative_call_is_not_an_absolute_site(self):
        self.assertEqual(self.sites('e8fb0f0000c3'), {})

    def test_value_outside_the_image_is_not_a_pointer(self):
        self.assertEqual(self.sites('a900000010c3'), {})

    def test_image_base_flag_is_a_candidate_like_the_linear_decoder(self):
        # `test eax, 0x400000` is in the image; target_class drops it as
        # unmapped for both decoders alike.
        self.assertEqual(set(self.sites('a900004000c3')), {0x1001})

    def test_bounded_jump_table_entries_are_switch_sites(self):
        words = {0x1800: 0x401000, 0x1804: 0x401001, 0x1808: 0x402800}
        found = self.sites('c3', tables={0x1800: (0x1000, 12, 'jump'),
                                         0x1900: (0x1000, 4, 'byte')}, words=words)
        self.assertEqual({rva: site.channel for rva, site in found.items()},
                         {0x1800: 'switch', 0x1804: 'switch'})

    def test_linear_site_splitting_a_decoded_instruction_is_misaligned(self):
        from homm3.census.find_relocs import Site
        # mov eax, [ecx + 0x402000]; a resynchronised linear decoder reported
        # the four bytes one past the displacement field.
        census = self.census('8b8100204000c3')
        linear = {0x1002: Site(0x1002, 0x402000, 'code', 'mov'),
                  0x1003: Site(0x1003, 0xc3004020, 'code', 'add'),
                  0x1801: Site(0x1801, 0x402000, 'switch', 'table@0x1800')}
        self.assertEqual(code_operands.misaligned(census, linear), {0x1003})

    def test_register_relative_displacement_is_not_an_address(self):
        self.assertEqual(self.sites('8b8100204000c3'), {})


if __name__ == '__main__':
    unittest.main()
