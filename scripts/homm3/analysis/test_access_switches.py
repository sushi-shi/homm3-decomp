"""Generic switch proofs reject missing bounds, identities and CFG boundaries."""
from collections import defaultdict
from dataclasses import replace
import struct
import unittest

from homm3.analysis.access_switches import graph
from homm3.analysis.data_accesses import analyze
from homm3.analysis.data_functions import Functions
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


class Code:
    def __init__(self, start=0x401780):
        self.start, self.raw, self.labels, self.fields = start, bytearray(), {}, []

    def label(self, name):
        self.labels[name] = len(self.raw)

    def emit(self, code):
        self.raw.extend(bytes.fromhex(code))

    def field(self, prefix, target, width=4, relative=False):
        self.emit(prefix)
        self.fields.append((len(self.raw), target, width, relative))
        self.raw.extend(bytes(width))

    def finish(self):
        for offset, target, width, relative in self.fields:
            value = self.labels[target]-(offset+width) if relative else self.start+self.labels[target]
            self.raw[offset:offset+width] = (value & ((1 << (width*8))-1)).to_bytes(width, 'little')
        return bytes(self.raw)


def switch(*, compressed=False, word=False, partial=False, bypass=False, late_bypass=False):
    code = Code()
    if bypass:
        code.emit('85db')
        code.field('75', 'dispatch', 1, True)
    code.label('compare')
    code.emit('83f905' if compressed else '83f902')
    code.label('guard')
    code.field('77', 'default', 1, True)
    if compressed:
        if partial:
            code.label('clear'); code.emit('31d2')
        code.label('lookup')
        code.field(('668b144d' if word else '8a91') if partial else
                   ('0fb7144d' if word else '0fb691'), 'indices')
    code.label('dispatch')
    code.field('ff2495' if compressed else 'ff248d', 'targets')
    code.label('default'); code.emit('c3')
    for i, name in enumerate(('red', 'green', 'blue')):
        code.label(name)
        if late_bypass and i == 2:
            code.field('eb', 'dispatch', 1, True)
        else:
            code.emit('c705'+struct.pack('<II', 0x407300+i*8, 7+i).hex()+'c3')
    code.emit('cccc')
    code.label('targets')
    for name in ('red', 'green', 'blue'):
        code.field('', name)
    if compressed:
        code.label('indices')
        for value in (2, 0, 1, 2, 2, 0):
            code.raw.extend(value.to_bytes(2 if word else 1, 'little'))
    code.finish()
    return code


def coff(code):
    raw, refs = bytearray(code.raw), []
    for site, target, width, relative in code.fields:
        if relative:
            continue
        struct.pack_into('<I', raw, site, code.labels[target])
        refs.append((site, 1, 6))
    return CoffObject(_coff((FixtureSection('.text', bytes(raw), tuple(refs)),),
                           (_symbol('owner', 0, 1, 0x20, 2), _symbol('.text', 0, 1, 0, 3))))


def provider(obj):
    return Functions(Layout(fixture()), {}, defaultdict(set), {'other': obj}, None)


class SwitchesTest(unittest.TestCase):
    def test_direct_and_compressed_tables_recover_every_reachable_case_write(self):
        for options in ({}, {'compressed': True}, {'compressed': True, 'partial': True},
                        {'compressed': True, 'word': True}, {'compressed': True, 'word': True, 'partial': True}):
            with self.subTest(options=options):
                code = switch(**options)
                flow = graph(code.raw, code.start)
                self.assertEqual(flow['issues'], [])
                self.assertEqual(len(flow['switches']), 1)
                proof = flow['switches'][0]
                names = ('blue', 'red', 'green', 'blue', 'blue', 'red') if options else ('red', 'green', 'blue')
                self.assertEqual(proof['case_targets'], [code.start+code.labels[n] for n in names])
                self.assertEqual(proof['index_domain'], [0, len(names)-1])
                report = analyze(code.raw, code.start)
                self.assertEqual(sum(r['access'] == 'write' for r in report['accesses']), 3)
                self.assertNotIn(code.start+code.labels['targets'], flow['instructions'])

    def test_jae_domain_excludes_the_limit(self):
        code = switch()
        code.raw[code.labels['guard']] = 0x73
        proof = graph(code.raw, code.start)['switches'][0]
        self.assertEqual(proof['index_domain'], [0, 1])
        self.assertEqual(len(proof['targets']), 2)

    def test_case_limit_is_bounded_and_never_truncates_the_domain(self):
        for count in (4096, 4097):
            code = Code()
            code.emit('81f9'+struct.pack('<I', count-1).hex())
            code.field('77', 'default', 1, True)
            code.field('ff248d', 'targets')
            code.label('default'); code.emit('c3')
            code.label('case'); code.emit('c3')
            code.label('targets')
            for _ in range(count):code.field('', 'case')
            flow = graph(code.finish(), code.start)
            if count == 4096:
                self.assertEqual(len(flow['switches'][0]['case_targets']), count)
            else:
                self.assertFalse(flow['switches'])

    def test_changed_valid_table_entry_changes_the_case_map(self):
        code = switch(compressed=True)
        before = graph(code.raw, code.start)['switches'][0]
        struct.pack_into('<I', code.raw, code.labels['targets'], code.start+code.labels['blue'])
        after = graph(code.raw, code.start)['switches'][0]
        self.assertNotEqual(before['case_targets'], after['case_targets'])
        self.assertEqual(after['case_targets'][1], code.start+code.labels['blue'])

    def test_wrong_signed_guard_index_and_large_negative_limit_remain_unproved(self):
        for offset, value in ((3, 0x7f), (1, 0xf8), (2, 0xff), (2, 0x80)):
            code = switch(); code.raw[offset] = value
            self.assertFalse(graph(code.raw, code.start)['switches'])
        code = switch(); code.raw[2] = 0; code.raw[3] = 0x73
        self.assertFalse(graph(code.raw, code.start)['switches'])

    def test_partial_index_requires_proved_zero_upper_bits(self):
        code = switch(compressed=True, partial=True)
        code.raw[code.labels['clear']:code.labels['clear']+2] = bytes.fromhex('89d2')
        flow = graph(code.raw, code.start)
        self.assertFalse(flow['switches'])
        self.assertIn('partial index lacks proved zero upper bits', [i.get('reason') for i in flow['issues']])

    def test_unscaled_word_lookup_does_not_read_disjoint_words(self):
        code = switch(compressed=True, word=True)
        code.raw[code.labels['lookup']+3] = 0x0d  # scale 1, not scale 2
        self.assertFalse(graph(code.raw, code.start)['switches'])

    def test_direct_and_newly_discovered_guard_bypasses_revoke_proof(self):
        for options in ({'bypass': True}, {'late_bypass': True}):
            code = switch(**options)
            flow = graph(code.raw, code.start)
            self.assertFalse(flow['switches'])
            self.assertTrue(any(i['kind'] == 'indirect-control-flow' for i in flow['issues']))

    def test_guard_cannot_send_out_of_range_indices_back_to_dispatch(self):
        code = switch()
        code.raw[4] = 0  # JA taken and fallthrough both reach dispatch.
        self.assertFalse(graph(code.raw, code.start)['switches'])

    def test_outside_body_truncated_table_and_table_byte_targets_are_rejected(self):
        for target in (0x809000, 'targets'):
            code = switch()
            value = code.start+code.labels[target] if isinstance(target, str) else target
            struct.pack_into('<I', code.raw, code.labels['targets'], value)
            self.assertFalse(graph(code.raw, code.start)['switches'])
        code = switch()
        self.assertFalse(graph(code.raw[:-1], code.start)['switches'])

    def test_target_inside_an_already_reachable_instruction_is_not_a_boundary(self):
        code = switch()
        struct.pack_into('<I', code.raw, code.labels['targets']+4, code.start+code.labels['red']+2)
        flow = graph(code.raw, code.start)
        self.assertFalse(flow['switches'])

    def test_target_table_must_not_overlap_reachable_code(self):
        code = switch()
        struct.pack_into('<I', code.raw, code.labels['dispatch']+3, code.start+code.labels['red'])
        self.assertFalse(graph(code.raw, code.start)['switches'])

    def test_local_coff_labels_resolve_without_using_retail_pointer_values(self):
        for compressed in (False, True):
            code = switch(compressed=compressed, partial=compressed)
            obj = coff(code)
            flow = provider(obj).control_flow(('other', 0))
            self.assertEqual(flow['issues'], [])
            self.assertEqual(len(flow['switches']), 1)
            self.assertEqual(flow['switches'][0]['targets'], [code.labels[n] for n in ('red', 'green', 'blue')])
            # Every encoded COFF address is section-relative here, not a
            # candidate or retail VA. Removing one relocation loses proof.
            obj.relocations = obj.relocations[:-1]
            self.assertFalse(provider(obj).control_flow(('other', 0))['switches'])

    def test_missing_external_wrong_width_and_overlapping_coff_relocations_fail(self):
        code = switch()
        for change in ('external', 'duplicate', 'wrong-kind', 'opcode', 'writable', 'relocated-limit'):
            obj = coff(code)
            if change == 'external': obj.symbols[1] = replace(obj.symbols[1], section=0)
            elif change == 'duplicate': obj.relocations = (*obj.relocations, obj.relocations[0])
            elif change == 'wrong-kind': obj.relocations = (replace(obj.relocations[0], typ=20), *obj.relocations[1:])
            elif change == 'opcode': obj.relocations = (replace(obj.relocations[0], site=code.labels['dispatch']), *obj.relocations[1:])
            elif change == 'writable': obj.sections = [replace(obj.sections[0], characteristics=obj.sections[0].characteristics|0x80000000)]
            else: obj.relocations = (*obj.relocations, replace(obj.relocations[0], site=2))
            with self.subTest(change=change):
                self.assertFalse(provider(obj).control_flow(('other', 0))['switches'])

    def test_coff_relative_branch_relocation_uses_symbol_addend_not_placeholder(self):
        raw = bytes.fromhex('e900000000c3c3')
        obj = CoffObject(_coff((FixtureSection('.text', raw, ((1, 1, 20),)),),
                              (_symbol('owner', 0, 1, 0x20, 2), _symbol('end', 6, 1, 0, 3))))
        flow = provider(obj).control_flow(('other', 0))
        self.assertEqual(flow['edges'][0], [6])
        self.assertNotIn(5, flow['instructions'])

    def test_far_jump_and_unbounded_indirect_jump_remain_explicit(self):
        for raw in ('ea000000000000', 'ffe0'):
            flow = graph(bytes.fromhex(raw))
            self.assertTrue(any(i['kind'] == 'indirect-control-flow' for i in flow['issues']))


if __name__ == '__main__':
    unittest.main()
