"""Dataflow contracts: joins, unknown relocations and calls cannot invent values."""
import struct
import unittest

from homm3.analysis.data_effects import analyze, integer


class DataEffectsTest(unittest.TestCase):
    def test_constant_global_writes_and_register_aliases(self):
        # xor eax,eax; mov al,7; mov [0x403000],eax; or ecx,-1;
        # mov [0x403004],ecx; ret
        raw = bytes.fromhex('31c0b007a30030400083c9ff890d04304000c3')
        result = analyze(raw)
        self.assertTrue(result['complete'])
        self.assertEqual([(e['address'], e['value']) for e in result['events']],
                         [(integer(0x403000), integer(7)), (integer(0x403004), integer(-1))])

    def test_unknown_relocation_is_not_literal_zero(self):
        raw = bytes.fromhex('c7050000000001000000c3')
        result = analyze(raw, relocation=lambda ins, field: (True, None) if field == 'disp' else (False, None))
        self.assertFalse(result['complete'])
        self.assertIsNone(result['events'][0]['address'])

    def test_conflicting_branch_values_merge_to_unknown(self):
        # test ecx,ecx; je +7; mov eax,1; jmp +5; mov eax,2; store; ret
        raw = bytes.fromhex('85c97407b801000000eb05b802000000a300304000c3')
        result = analyze(raw)
        self.assertFalse(result['complete'])
        self.assertTrue(result['conditional'])
        self.assertIsNone(result['events'][0]['value'])

    def test_stack_spill_survives_supported_operations_but_not_unknown_call(self):
        prefix = bytes.fromhex('5589e583ec04c745fc07000000')
        suffix = bytes.fromhex('8b45fca300304000c9c3')
        result = analyze(prefix+suffix)
        self.assertTrue(result['complete'])
        self.assertEqual(result['events'][0]['value'], integer(7))
        result = analyze(prefix+bytes.fromhex('e800000000')+suffix)
        self.assertFalse(result['complete'])
        self.assertIsNone(result['events'][-1]['value'])

    def test_callback_argument_is_captured_at_call(self):
        raw = b'\x68'+struct.pack('<I', 0x401000)+b'\xe8'+bytes(4)+b'\xc3'
        result = analyze(raw)
        self.assertEqual(result['events'][0]['arguments'][0], integer(0x401000))
        self.assertFalse(result['complete'])

    def test_overwritten_stack_argument_cannot_reuse_a_stale_push(self):
        # push known callback; replace [esp] with unknown eax; call; ret
        raw = bytes.fromhex('6800104000890424e800000000c3')
        result = analyze(raw)
        call = next(e for e in result['events'] if e['kind'] == 'call')
        self.assertIsNone(call['arguments'][0])

    def test_loop_does_not_invent_a_fixed_iteration_value_or_completion(self):
        result = analyze(bytes.fromhex('31c04085c975fba300304000c3'))
        self.assertIsNone(result['events'][0]['value'])
        self.assertFalse(result['complete'])
        self.assertFalse(analyze(bytes.fromhex('90ebfd'))['complete'])

    def test_unknown_byte_register_upper_bits_remain_unknown(self):
        result = analyze(bytes.fromhex('b007a300304000c3'))
        self.assertFalse(result['complete'])
        self.assertIsNone(result['events'][0]['value'])


if __name__ == '__main__':
    unittest.main()
