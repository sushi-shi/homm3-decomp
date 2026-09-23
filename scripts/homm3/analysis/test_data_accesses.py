"""Generic machine-code controls for byte addressing and lost evidence."""
import unittest
import struct

from homm3.analysis.access_expressions import Expression as E, affine_range
from homm3.analysis.data_accesses import State, Storage, analyze, compare, serial


def code(hexadecimal, **kwargs):
    return analyze(bytes.fromhex(hexadecimal), **kwargs)


class ExpressionsTest(unittest.TestCase):
    def test_polynomials_canonicalize_scale_and_factor_order(self):
        x, y = E.atom('argument', 0, 4), E.atom('argument', 4, 4)
        self.assertEqual(x.times(y).times(E.constant(2)), y.times(x.plus(x)))
        self.assertEqual(x.plus(E.constant(3)).times(E.constant(4)), x.times(E.constant(4)).offset(12))
        self.assertEqual(E.constant(0xffffffff).offset(2), E.constant(1))

    def test_range_needs_all_bounds_and_no_wrap(self):
        atom = ('index',)
        expression = E.atom(*atom).times(E.constant(4)).offset(8)
        self.assertEqual(affine_range(expression, {atom: (0, 9)}), (8, 44))
        self.assertIsNone(affine_range(expression, {}))
        self.assertIsNone(affine_range(expression, {atom: (0, 0x40000000)}))
        self.assertIsNone(affine_range(expression.times(E.atom('other')), {atom: (0, 9)}))

    def test_expression_budget_does_not_drop_terms(self):
        self.assertIsNone(E.make([(((str(i),),), 1) for i in range(65)]))
        x = E.atom('x')
        self.assertIsNone(x.times(x).times(x).times(x).times(x))

    def test_expression_export_is_structured_not_python_repr(self):
        import json
        raw = serial(E.atom('argument', 0, 4).offset(8))
        self.assertIsInstance(raw, list)
        self.assertEqual(json.loads(json.dumps(raw)), raw)


class DataAccessesTest(unittest.TestCase):
    def test_register_allocation_and_frame_pointer_do_not_define_identity(self):
        # Both read *(arg0 + arg1*4); one saves EBP and uses different registers.
        a = code('8b442404 8b4c2408 8b1488 c3')
        b = code('55 8bec 8b5508 8b450c 8b0c82 5d c3')
        self.assertEqual(compare(a, b)['status'], 'observed-expressions-agree')
        self.assertEqual(len(a['accesses']), 1)

    def test_doubled_byte_stride_changes_expression_without_changing_storage(self):
        # buffer + row*pitch versus buffer + row*pitch*2. No semantic names
        # enter the rule: all three quantities are incoming machine arguments.
        prefix = '8b442404 8b4c2408 8b54240c 0fafca '
        a = code(prefix+'8b1408 c3')
        b = code(prefix+'8b1448 c3')
        difference = compare(a, b)
        self.assertEqual(difference['status'], 'observed-expressions-differ')
        self.assertTrue(difference['complete_observation'])
        self.assertEqual(len(difference['missing']), 1)

    def test_lea_and_multiply_produce_same_byte_expression(self):
        prefix = '8b442404 8b4c2408 '
        a = code(prefix+'8d1449 8b0490 c3')  # index*3 then *4
        b = code(prefix+'6bc90c 8b0408 c3')
        self.assertEqual(compare(a, b)['status'], 'observed-expressions-agree')

    def test_width_is_part_of_access_contract(self):
        a = code('8b442404 8b08 c3')
        b = code('8b442404 0fb708 c3')
        self.assertEqual(compare(a, b)['status'], 'observed-expressions-differ')

    def test_global_relocation_identity_changes_shared_storage(self):
        def resolver(address):
            return lambda ins, field: ((True, ('integer', address)) if ins.address == 0 and field == 'disp' else (False, None))
        a = code('a100000000 c3', relocation=resolver(0x500000))
        b = code('a100000000 c3', relocation=resolver(0x500008))
        self.assertEqual(compare(a, b)['status'], 'observed-expressions-differ')
        unknown = code('a100000000 c3', relocation=lambda ins, field: (field == 'disp', None))
        self.assertIsNone(unknown['accesses'][0]['address'])
        self.assertFalse(compare(unknown, unknown)['complete_observation'])

    def test_field_loads_preserve_offsets_and_scaling(self):
        # arg0->buffer + arg1 * arg0->stride, where fields are at +0 and +4.
        prefix = '8b442404 8b4c2408 8b5004 0fafca 8b00 '
        a, b = code(prefix+'8b1408 c3'), code(prefix+'8b1448 c3')
        self.assertEqual(compare(a, b)['status'], 'observed-expressions-differ')
        self.assertTrue(all(r['address'] is not None for r in a['accesses']))

    def test_memory_mutation_does_not_reuse_initial_load_identity(self):
        a = code('8b442404 8b08 c70000000000 8b10 8b02 c3')
        self.assertIsNone(a['accesses'][-1]['address'])
        self.assertIn('load-after-memory-mutation', {i['kind'] for i in a['issues']})

    def test_argument_overwrite_and_join_never_resurrect_input(self):
        a = code('85c0 7405 c644240400 8b442404 8b00 c3')
        self.assertIsNone(a['accesses'][-1]['address'])
        self.assertTrue(a['branch_sites'])
        x = State({}, {(4, 1): None})
        self.assertIn((4, 1), x.join(State({}, {})).stack)

    def test_counted_loop_has_complete_index_domain_not_sample_iteration(self):
        a = code('8b442404 31c9 8b1488 41 83f904 7cf7 c3')
        self.assertTrue(a['back_edges'])
        self.assertIsNotNone(a['accesses'][0]['address'])
        self.assertEqual(next(iter(a['loops'].values()))['bounds'], (0, 3))
        self.assertFalse(compare(a, a)['complete_observation'])

    def test_loop_consumer_exposes_short_storage(self):
        storage = Storage(0x400000, [dict(rva=0x1000, size=12, status='enrolled')])
        a = code('b800104000 31c9 8b1488 41 83f904 7cf7 c3', normalize_address=storage.normalize)
        domains = {r['atom']: r['bounds'] for r in a['loops'].values()}
        extent = storage.extent(a['accesses'][0]['address'], 4, domains)
        self.assertEqual(extent['status'], 'outside-source-extent')
        self.assertEqual(extent['byte_range'], [0, 16])
        storage = Storage(0x400000, [dict(rva=0x1000, size=16, status='enrolled')])
        self.assertEqual(storage.extent(a['accesses'][0]['address'], 4, domains)['status'], 'within-source-extent')

    def test_loop_with_skipped_update_remains_unbounded(self):
        a = code('8b442404 31c9 8b1488 85d2 7401 41 83f904 7cf3 c3')
        self.assertFalse(a['loops'])
        self.assertFalse(compare(a, a)['complete_observation'])

    def test_loop_domain_uses_mathematical_difference_and_rejects_wrap(self):
        # INT_MIN .. INT_MAX is a large domain, never one sampled iteration.
        a = code('b900000080 8b1488 41 81f9ffffff7f 7cf4 c3')
        self.assertEqual(next(iter(a['loops'].values()))['iterations'], 0xffffffff)
        # <= INT_MAX followed by increment wraps: no finite linear domain.
        b = code('b900000080 8b1488 41 81f9ffffff7f 7ef4 c3')
        self.assertFalse(b['loops'])

    def test_non_divisible_not_equal_loop_is_not_bounded(self):
        a = code('31c9 8b1488 83c103 83f904 75f5 c3')
        self.assertFalse(a['loops'])

    def test_outer_back_edge_cannot_reuse_first_inner_counter_domain(self):
        # Outer EAX increments; the inner ECX starts at the current EAX.
        # Its first domain cannot stand for subsequent outer iterations.
        a = code('31c0 89c1 8b14ce 41 83f904 7cf7 40 83f803 7cef c3')
        self.assertFalse(a['loops'])

    def test_unsupported_loop_control_does_not_keep_first_iteration_address(self):
        # Capstone puts LOOP in BRANCH_RELATIVE, but not its JUMP group.
        # Its ECX mutation and back edge must still invalidate the first address.
        a = code('b904000000 b800104000 8b1488 e2fb c3')
        self.assertTrue(a['back_edges'])
        self.assertIsNone(a['accesses'][0]['address'])
        self.assertFalse(compare(a, a)['complete_observation'])

    def test_loop_bounds_agree_with_independent_counter_execution(self):
        predicates = {0x7c: lambda a, b: a < b, 0x7e: lambda a, b: a <= b,
                      0x7f: lambda a, b: a > b, 0x7d: lambda a, b: a >= b,
                      0x75: lambda a, b: a != b}
        checked = 0
        for initial in range(-4, 5):
            for limit in range(-4, 5):
                for step in (-3, -1, 1, 3):
                    for opcode, predicate in predicates.items():
                        raw = (b'\xb9'+struct.pack('<i', initial)+b'\x8b\x04\x8a'+
                               b'\x81\xc1'+struct.pack('<i', step)+b'\x81\xf9'+struct.pack('<i', limit)+
                               bytes((opcode, 0xef, 0xc3)))
                        report = analyze(raw)
                        if not report['loops']:
                            continue
                        domain = next(iter(report['loops'].values()))
                        seen, counter = [], initial
                        for _ in range(64):
                            seen.append(counter)
                            counter += step
                            if not predicate(counter, limit):
                                break
                        else:
                            self.fail('admitted a nonterminating counter')
                        self.assertEqual(domain['iterations'], len(seen))
                        self.assertEqual(domain['bounds'], (0, len(seen)-1))
                        checked += 1
        self.assertGreater(checked, 150)

    def test_calls_invalidate_loaded_fields_and_volatile_registers(self):
        a = code('8b442404 e800000000 8b00 c3')
        self.assertIsNone(a['accesses'][-1]['address'])
        self.assertEqual(a['issues'][0]['kind'], 'call-effects-unproved')

    def test_rep_operand_width_alone_cannot_prove_any_access(self):
        # ECX=0 makes REP MOVSD access zero bytes even though its operand is 4B.
        a = code('31c9 f3a5 c3')
        self.assertTrue(all(r['execution'] == 'repeat-count-unproved' for r in a['accesses']))
        self.assertFalse(compare(a, a)['complete_observation'])


if __name__ == '__main__':
    unittest.main()
