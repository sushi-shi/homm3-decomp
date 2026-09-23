"""Raw-code call controls: argument/receiver substitution and invalidation."""
import unittest

from homm3.analysis.access_expressions import Expression as E, substitute
from homm3.analysis.access_calls import Summaries, read_only
from homm3.analysis.data_accesses import Storage, analyze, compare


class Bodies:
    def __init__(self, code, targets, pops=None):
        self.code = {k: bytes.fromhex(v) for k, v in code.items()}
        self.targets = targets
        self.pops = pops or {}

    def body(self, key):
        return self.code[key], 0

    def key(self, token):
        return token[1] if token and token[0] == 'code' else None

    def pop_count(self, token):
        return self.pops.get(self.key(token), 0)

    def relocation_resolver(self, key):
        def resolve(ins, field):
            if field == 'imm' and (key, ins.address) in self.targets:
                return True, ('code', self.targets[key, ins.address])
            return False, None
        return resolve


class CallsTest(unittest.TestCase):
    def summaries(self, code, targets, pops=None):
        self.storage = Storage(0x400000, [dict(rva=0x1000, size=40, status='enrolled')])
        return Summaries(Bodies(code, targets, pops), self.storage.normalize)

    def test_read_only_getter_transports_argument_and_returned_pointer(self):
        summaries = self.summaries(dict(caller='ff742404 e800000000 83c404 8b08 c3',
                                       getter='8b442404 83c008 c3'), {('caller',4):'getter'})
        report = summaries.analyze('caller')
        self.assertTrue(read_only(report))
        self.assertEqual(report['accesses'][0]['address'], E.atom('argument',0,4).offset(8))
        self.assertEqual(report['calls'][0]['status'],'read-only-summary')

    def test_this_pointer_field_stride_survives_a_proved_getter(self):
        summaries = self.summaries(dict(caller='8b4c2404 e800000000 8b4c2408 0fafc1 8b54240c 8b0c02 c3',
                                       getter='8b4104 c3'), {('caller',4):'getter'})
        report = summaries.analyze('caller')
        self.assertEqual(report['calls'][0]['status'],'read-only-summary')
        self.assertTrue(all(r['address'] is not None for r in report['accesses']))
        # An extra pixel-size multiply changes the machine byte address even
        # when the same field and backing buffer are used.
        changed = self.summaries(dict(caller='8b4c2404 e800000000 8b4c2408 0fafc1 8b54240c 8b0c42 c3',
                                     getter='8b4104 c3'), {('caller',4):'getter'}).analyze('caller')
        self.assertEqual(compare(report,changed)['status'],'observed-expressions-differ')

    def test_caller_guard_bounds_callee_array_read(self):
        summaries = self.summaries(dict(caller='8b4c2404 83f90a 7309 51 e800000000 83c404 c3',
                                       reader='8b4c2404 8b048d00104000 c3'), {('caller',10):'reader'})
        report = summaries.analyze('caller')
        row = next(r for r in report['accesses'] if r.get('call_path'))
        facts={b['atom']:b['bounds'] for b in row['bounds']}
        self.assertEqual(self.storage.extent(row['address'],4,facts)['byte_range'],[0,40])

    def test_mutating_callee_cannot_reuse_a_pure_summary(self):
        summaries = self.summaries(dict(caller='e800000000 8b08 c3',
                                       callee='b800104000 c70001000000 c3'), {('caller',0):'callee'})
        report = summaries.analyze('caller')
        self.assertEqual(report['calls'][0]['status'],'effects-unproved')
        self.assertIsNone(report['accesses'][0]['address'])

    def test_memory_mutation_prevents_returning_an_initial_field_value(self):
        summaries = self.summaries(dict(caller='c7050010400001000000 e800000000 8b08 c3',
                                       getter='a100104000 c3'), {('caller',10):'getter'})
        report = summaries.analyze('caller')
        self.assertIsNone(report['accesses'][-1]['address'])

    def test_clobbered_nonvolatile_or_caller_argument_rejects_summary(self):
        for callee in ('bb01000000 c3','c744240401000000 c3'):
            summaries = self.summaries(dict(caller='e800000000 c3',callee=callee),{('caller',0):'callee'})
            self.assertEqual(summaries.analyze('caller')['calls'][0]['status'],'effects-unproved')

    def test_nested_calls_and_callee_cleanup_are_instantiated(self):
        summaries=self.summaries(dict(caller='ff742404 e800000000 8b08 c3',
                                      middle='ff742404 e800000000 c20400', leaf='8b442404 83c004 c20400'),
                                  {('caller',4):'middle',('middle',4):'leaf'}, {'middle':4,'leaf':4})
        report=summaries.analyze('caller')
        self.assertTrue(read_only(report))
        self.assertEqual(report['accesses'][0]['address'],E.atom('argument',0,4).offset(4))

    def test_recursive_call_is_explicitly_unproved(self):
        summaries=self.summaries(dict(caller='e800000000 c3'),{('caller',0):'caller'})
        report=summaries.analyze('caller')
        self.assertFalse(read_only(report))
        self.assertEqual(report['calls'][0]['status'],'effects-unproved')

    def test_unknown_or_unaligned_argument_does_not_inherit_another_argument(self):
        self.assertIsNone(substitute(E.atom('argument',4,4),[E.constant(1)],{}))
        self.assertIsNone(substitute(E.atom('argument',1,4),[E.constant(1)],{}))


if __name__ == '__main__':
    unittest.main()
