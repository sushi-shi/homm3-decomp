"""Bounded x86 string effects must retain count, direction and identity gaps."""
import struct
import unittest
from collections import defaultdict

from homm3.analysis.data_effects import State, analyze, integer
from homm3.analysis.data_initialization import compare_profiles
from homm3.analysis.candidate_data import inventory
from homm3.analysis.data_functions import Functions
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
from homm3.sema.data_match import Identities
from homm3.sema.test_data_match import binding
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


def immediate(opcode, value):
    return bytes([opcode])+struct.pack('<I', value)


def writes(result):
    return [e for e in result['events'] if e['kind'] == 'write']


def fill(count=3, value=0x728195a3, destination=0x405087, instruction='f3ab'):
    return (immediate(0xb9, count)+immediate(0xb8, value)+immediate(0xbf, destination)+
            bytes.fromhex(instruction)+b'\xc3')


class RepeatedEffectsTest(unittest.TestCase):
    def test_bounded_stores_and_register_postconditions(self):
        for opcode, width, expected in [('f3aa', 1, 0xa3), ('66f3ab', 2, 0x95a3), ('f3ab', 4, 0x728195a3)]:
            with self.subTest(opcode=opcode):
                raw = fill(instruction=opcode)[:-1]+bytes.fromhex('890d90504000893d94504000c3')
                result = analyze(raw)
                self.assertTrue(result['complete'])
                self.assertEqual([(e['address'][1], e['width'], e['value'][1]) for e in writes(result)[:3]],
                                 [(0x405087+i*width, width, expected) for i in range(3)])
                self.assertEqual([e['value'] for e in writes(result)[3:]], [integer(0), integer(0x405087+3*width)])

    def test_reverse_and_single_element_forms(self):
        result = analyze(b'\xfd'+fill())
        self.assertTrue(result['complete'])
        self.assertEqual([e['address'][1] for e in writes(result)], [0x405087, 0x405083, 0x40507f])
        self.assertEqual([e['string_operation']['direction'] for e in writes(result)], [-1]*3)
        self.assertEqual(len(writes(analyze(fill(count=9000, instruction='ab')))), 1)

    def test_stack_aggregate_and_byte_tail(self):
        # Two unrelated constants in a local aggregate, copied as dword + word
        # + byte. These are emitted instructions, not copied retail values.
        raw = (bytes.fromhex('687856341268debc9a78')+bytes.fromhex('89e6')+
               immediate(0xbf, 0x407039)+immediate(0xb9, 1)+bytes.fromhex('f3a566a5a4c3'))
        result = analyze(raw)
        self.assertTrue(result['complete'])
        self.assertEqual([(e['width'], e['value'][1]) for e in writes(result)],
                         [(4, 0x789abcde), (2, 0x5678), (1, 0x34)])
        self.assertEqual([e['string_operation']['source'] for e in writes(result)],
                         [('stack', -8), ('stack', -4), ('stack', -2)])

    def test_stack_overlap_is_sequential_not_memmove(self):
        # Stack bytes 1,2,3,4; forward copy 3 bytes to source+1 -> 1,1,1,1.
        raw = bytes.fromhex('680102030489e68d7c2401b903000000f3a48b0424a387504000c3')
        result = analyze(raw)
        # Partial overwrite conservatively discards untouched bytes of a stored
        # word. It must never manufacture a snapshot copy of 1,2,3.
        self.assertNotEqual(writes(result)[0]['value'], integer(0x03020101))
        self.assertFalse(result['complete'])

    def test_complete_stack_byte_overlap(self):
        raw = bytes.fromhex('83ec04c6042401c644240102c644240203c64424030489e68d7c2401b903000000f3a48b0424a387504000c3')
        result = analyze(raw)
        self.assertTrue(result['complete'])
        self.assertEqual(writes(result)[0]['value'], integer(0x01010101))

    def test_zero_count_does_not_read_write_or_require_addresses_direction(self):
        result = analyze(bytes.fromhex('31c9f3a5c3'), entry_direction=None)
        self.assertTrue(result['complete'])
        self.assertEqual(result['events'], [])

    def test_unknown_oversized_counts_and_unknown_direction_are_incomplete(self):
        cases = [(fill(count=1025), 'unbounded-string-count', 1),
                 (bytes.fromhex('f3abc3'), 'unbounded-string-count', 1),
                 (fill(), 'unknown-string-direction', None),
                 (b'\x9d'+fill(), 'unknown-string-direction', 1)]
        for raw, reason, direction in cases:
            with self.subTest(reason=reason, raw=raw):
                result = analyze(raw, entry_direction=direction)
                self.assertFalse(result['complete'])
                self.assertIn(reason, [i['kind'] for i in result['issues']])
                self.assertIsNone(writes(result)[0]['address'])
        self.assertTrue(analyze(b'\xfc'+fill(), entry_direction=None)['complete'])

    def test_conflicting_direction_join_remains_unknown(self):
        raw = bytes.fromhex('85d27403fceb01fd')+fill()
        result = analyze(raw)
        self.assertIn('unknown-string-direction', [i['kind'] for i in result['issues']])

    def test_address_size_segment_and_repeatne_overrides_are_not_flat_copies(self):
        for prefix in ('67f3', '64f3', 'f2'):
            result = analyze(fill(instruction=prefix+'ab'))
            self.assertFalse(result['complete'])
            self.assertIn('unsupported-string-addressing', [i['kind'] for i in result['issues']])

    def test_unknown_source_destination_and_relocation_are_not_zero_values(self):
        copy = immediate(0xb9, 2)+immediate(0xbf, 0x406050)+bytes.fromhex('f3a5c3')
        for raw in (copy, immediate(0xbe, 0x408000)+copy):
            result = analyze(raw)
            self.assertFalse(result['complete'])
            self.assertEqual([e['value'] for e in writes(result)], [None, None])
            self.assertEqual(len([e for e in result['events'] if e['kind'] == 'read']), 2)
        raw = immediate(0xb9, 1)+bytes.fromhex('31c0f3abc3')
        self.assertIsNone(writes(analyze(raw))[0]['address'])
        raw = bytes.fromhex('680000000089e6')+copy
        def unresolved(ins, field):
            return (True, None) if ins.mnemonic == 'push' and field == 'imm' else (False, None)
        result = analyze(raw, relocation=unresolved)
        self.assertIsNone(writes(result)[0]['value'])
        self.assertFalse(result['complete'])

    def test_unknown_copy_clobbers_stack_and_unknown_call_clobbers_direction(self):
        raw = bytes.fromhex('6807000000f3a58b0424a387504000c3')
        self.assertIsNone(writes(analyze(raw))[-1]['value'])
        raw = bytes.fromhex('e800000000')+fill()
        self.assertIn('unknown-string-direction', [i['kind'] for i in analyze(raw)['issues']])

    def test_symbolic_stack_value_preserved_only_for_exact_width(self):
        state = State({}, {})
        state.store(('stack', -4), 4, ('code', ('other', 17)))
        self.assertEqual(state.load(('stack', -4), 4), ('code', ('other', 17)))
        self.assertIsNone(state.load(('stack', -4), 2))

    def test_initializer_comparison_exposes_count_value_and_destination_changes(self):
        def profile(raw):
            result = analyze(raw)
            return dict(complete=result['complete'], writes=[dict(e, rva=e['address'][1]-0x400000)
                        for e in writes(result)], owners=[], anchors=[], reads=[])
        retail = profile(fill())
        self.assertEqual(compare_profiles(retail, retail)['matched_effect_bytes'], 12)
        for changed in (fill(count=2), fill(value=8), fill(destination=0x405088)):
            self.assertEqual(compare_profiles(retail, profile(changed))['status'], 'effects-differ')

    def test_raw_coff_pointer_copy_requires_independent_referent_binding(self):
        def profile(target, bind_referent=True):
            code = bytes.fromhex('680000000089e6bf00000000b901000000f3a5c3')
            raw = bytearray(_coff((FixtureSection('.text', code, ((1, target, 6), (8, 1, 6))),
                                  FixtureSection('.data', bytes(12), ())),
                                 (_symbol('startup', 0, 1, 0x20, 2), _symbol('slot', 0, 2, 0, 2),
                                  _symbol('first', 4, 2, 0, 2), _symbol('second', 8, 2, 0, 2))))
            struct.pack_into('<I', raw, 96, 0xc0300040)
            obj = CoffObject(bytes(raw))
            rows = inventory('unrelated', obj, 'raw')
            bindings = [binding(row, 0x2000+16*i, 4) for i, row in enumerate(rows)
                        if bind_referent or i == 0]
            identities = Identities(rows, bindings, {'unrelated': obj})
            provider = Functions(Layout(fixture()), {}, defaultdict(set), {'unrelated': obj}, identities)
            return provider.closure(('unrelated', 0))
        for target, value in ((2, 0x402010), (3, 0x402020)):
            result = profile(target)
            self.assertTrue(result['complete'])
            self.assertEqual(result['writes'][0]['value'], integer(value))
        result = profile(2, bind_referent=False)
        self.assertFalse(result['complete'])
        self.assertEqual(result['writes'][0]['rva'], 0x2000)
        self.assertIsNone(result['writes'][0]['value'])


if __name__ == '__main__':
    unittest.main()
