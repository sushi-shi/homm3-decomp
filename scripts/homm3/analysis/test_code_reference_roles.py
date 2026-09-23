"""Operand roles need real entries, instruction paths and relocation fields."""
import struct
import unittest
from homm3.analysis.code_reference_roles import Roles
from homm3.analysis.test_vendor_data import obj
from homm3.build.test_eh_handler_normalization import FixtureSection, _symbol


class ReferenceRolesTest(unittest.TestCase):
    def object(self, raw, refs, *, extra=(), entry=True):
        symbols=[_symbol('_func',0,1,0x20 if entry else 0,2),_symbol('_data',0,2,0,2),*extra]
        return obj([FixtureSection('.text',raw,refs),FixtureSection('.data',bytes(8),())],symbols).coff

    def test_register_comparison_records_the_exact_field_and_instruction_path(self):
        for addend in (-1,9,1234):
            raw=b'\x90'*8+b'\x81\xf9'+struct.pack('<i',addend)+b'\xc3'
            proof=Roles().inspect(self.object(raw,((10,1,6),)),1,10)
            self.assertEqual(proof['role'],'comparison-immediate')
            self.assertEqual(proof['instruction_offset'],8)
            self.assertEqual(proof['instruction_path'],list(range(9)))
            self.assertEqual(proof['entry_symbol_index'],0)
            self.assertEqual(proof['addend'],addend)
            self.assertEqual(proof['instruction_bytes'],raw[8:14].hex())

    def test_memory_operand_and_address_formation_are_not_comparison_proofs(self):
        for raw,site,role in [(b'\x81\x3d'+struct.pack('<ii',9,0)+b'\xc3',2,'memory-displacement'),
                              (b'\x8d\x0d'+struct.pack('<i',9)+b'\xc3',2,'address-formation'),
                              (b'\xb9'+struct.pack('<i',9)+b'\xc3',1,'unproved'),
                              (b'\x81\x3d'+struct.pack('<ii',0x402000,9)+b'\xc3',6,'unproved')]:
            with self.subTest(raw=raw):
                self.assertEqual(Roles().inspect(self.object(raw,((site,1,6),)),1,site)['role'],role)

    def test_wrong_type_duplicate_overlapping_or_wrong_instruction_field_is_unproved(self):
        raw=b'\x90'*8+b'\x81\xf9'+struct.pack('<i',9)+b'\xc3'
        for refs,site in [(((10,1,7),),10),(((10,1,20),),10),(((10,1,6),(10,1,6)),10),
                          (((10,1,6),(11,1,6)),10),(((9,1,6),),9),(((11,1,6),),11)]:
            with self.subTest(refs=refs):
                self.assertEqual(Roles().inspect(self.object(raw,refs),1,site)['role'],'unproved')

    def test_bytes_after_return_or_indirect_jump_do_not_become_instructions(self):
        for stop in (b'\xc3',b'\xff\xe0',b'\xcc'):
            raw=stop+b'\x81\xf9'+struct.pack('<i',9)+b'\xc3';site=len(stop)+2
            proof=Roles().inspect(self.object(raw,((site,1,6),)),1,site)
            self.assertEqual(proof['role'],'unproved')
            self.assertIn('instruction path',proof['reason'])

    def test_no_function_entry_means_no_instruction_proof(self):
        raw=b'\x81\xf9'+struct.pack('<i',9)+b'\xc3'
        self.assertEqual(Roles().inspect(self.object(raw,((2,1,6),),entry=False),1,2)['role'],'unproved')

    def test_direct_branch_can_skip_inline_bytes_but_cannot_expose_them(self):
        comparison=b'\x81\xf9'+struct.pack('<i',9)
        raw=b'\xeb\x06'+comparison+comparison+b'\xc3'
        candidate=self.object(raw,((4,1,6),(10,1,6)))
        roles=Roles()
        self.assertEqual(roles.inspect(candidate,1,4)['role'],'unproved')
        proof=roles.inspect(candidate,1,10)
        self.assertEqual(proof['role'],'comparison-immediate')
        self.assertEqual(proof['instruction_path'],[0,8])

    def test_symbolic_branch_uses_the_same_section_symbol_target_not_raw_displacement(self):
        raw=b'\xe9'+bytes(4)+b'\xcc'*11+b'\x81\xf9'+struct.pack('<i',9)+b'\xc3'
        extra=[_symbol('label',16,1,0,6)]
        candidate=self.object(raw,((1,2,20),(18,1,6)),extra=extra)
        proof=Roles().inspect(candidate,1,18)
        self.assertEqual(proof['role'],'comparison-immediate')
        self.assertEqual(proof['instruction_path'],[0,16])

    def test_external_symbolic_jump_cannot_fall_into_a_fake_comparison(self):
        raw=b'\xe9'+bytes(4)+b'\x81\xf9'+struct.pack('<i',9)+b'\xc3'
        candidate=self.object(raw,((1,2,20),(7,1,6)),extra=[_symbol('_extern',0,0,0,2)])
        self.assertEqual(Roles().inspect(candidate,1,7)['role'],'unproved')

    def test_overlapping_instruction_paths_are_not_a_choice_of_convenient_decode(self):
        raw=b'\x75\x01\xb8'+b'\x90'*4+b'\x81\xf9'+struct.pack('<i',9)+b'\xc3'
        proof=Roles().inspect(self.object(raw,((9,1,6),)),1,9)
        self.assertEqual(proof['role'],'unproved')
        self.assertEqual(proof['reason'],'overlapping instruction decodes')

    def test_relocation_in_a_predecessor_opcode_cannot_prove_the_later_path(self):
        raw=b'\xb8'+bytes(4)+b'\x81\xf9'+struct.pack('<i',9)+b'\xc3'
        proof=Roles().inspect(self.object(raw,((0,1,6),(7,1,6))),1,7)
        self.assertEqual(proof['role'],'unproved')
        self.assertIn('encoded instruction field',proof['reason'])


if __name__=='__main__':
    unittest.main()
