import unittest

from homm3.analysis.access_expressions import Expression as E
from homm3.analysis import access_bounds as bounds
from homm3.analysis.data_accesses import analyze, Storage


class BoundsTest(unittest.TestCase):
    def test_signed_and_unsigned_guards_keep_the_correct_half(self):
        atom = ('argument', 0, 4)
        predicate = ('cmp', E.atom(*atom), E.constant(10))
        self.assertEqual(bounds.refine({}, predicate, 'jae', False)[atom], (0, 9))
        self.assertEqual(bounds.refine({}, predicate, 'jl', True)[atom], (bounds.MIN, 9))
        self.assertEqual(bounds.refine({}, predicate, 'jae', True)[atom], (bounds.MIN, bounds.MAX))
        self.assertIsNone(bounds.refine({atom: (0, 9)}, predicate, 'jae', True))

    def test_modular_affine_comparison_is_not_solved_as_an_integer_inequality(self):
        x = E.atom('argument', 0, 4)
        self.assertEqual(bounds.refine({}, ('cmp', x.offset(1), E.constant(10)), 'jl', True), {})

    def test_js_after_subtract_cannot_ignore_overflow(self):
        x = E.atom('argument', 0, 4)
        self.assertEqual(bounds.refine({}, ('cmp', x, E.constant(1)), 'js', True), {})
        self.assertEqual(bounds.refine({}, ('test', x, E.constant(0)), 'js', False)[('argument', 0, 4)], (0, bounds.MAX))

    def test_all_predicate_spans_agree_with_sampled_32bit_comparisons(self):
        values = [bounds.MIN, -123, -1, 0, 1, 122, bounds.MAX]
        relations = {'eq': lambda a,b:a==b, 'ne': lambda a,b:a!=b, 'lt':lambda a,b:a<b,
                     'le':lambda a,b:a<=b, 'gt':lambda a,b:a>b, 'ge':lambda a,b:a>=b}
        for relation, test in relations.items():
            for unsigned in (False, True):
                for limit in values:
                    spans = bounds.ranges(('u' if unsigned else '')+relation, limit)
                    for value in values:
                        a,b=(value & 0xffffffff,limit & 0xffffffff) if unsigned else (value,limit)
                        self.assertEqual(any(lo<=value<=hi for lo,hi in spans),test(a,b))


class BranchAccessTest(unittest.TestCase):
    def profile(self, raw):
        self.storage = Storage(0x400000, [dict(rva=0x1000,size=40,status='enrolled')])
        return analyze(bytes.fromhex(raw), normalize_address=self.storage.normalize)

    def test_unsigned_guard_bounds_the_subsequent_table_access(self):
        p = self.profile('8b4c2404 83f90a 7307 8b048d00104000 c3')
        row = p['accesses'][0]
        facts = {r['atom']:r['bounds'] for r in row['bounds']}
        self.assertEqual(facts[('argument',0,4)],(0,9))
        self.assertEqual(self.storage.extent(row['address'],4,facts)['byte_range'],[0,40])

    def test_changed_consumer_limit_exposes_a_short_table(self):
        p = self.profile('8b4c2404 83f90b 7307 8b048d00104000 c3')
        row = p['accesses'][0]
        facts = {r['atom']:r['bounds'] for r in row['bounds']}
        self.assertEqual(self.storage.extent(row['address'],4,facts)['status'],'outside-source-extent')

    def test_signed_two_sided_guard_excludes_negative_indices(self):
        p = self.profile('8b4c2404 85c9 780c 83f90a 7d07 8b048d00104000 c3')
        facts = {r['atom']:r['bounds'] for r in p['accesses'][0]['bounds']}
        self.assertEqual(facts[('argument',0,4)],(0,9))

    def test_join_does_not_keep_a_guard_bypassed_on_another_path(self):
        p = self.profile('8b4c2404 85c0 7405 83f90a 7307 8b048d00104000 c3')
        facts = {r['atom']:r['bounds'] for r in p['accesses'][0]['bounds']}
        self.assertEqual(facts[('argument',0,4)],(bounds.MIN,bounds.MAX))

    def test_flag_clobber_prevents_a_stale_comparison_guard(self):
        p = self.profile('8b4c2404 83f90a 40 7307 8b048d00104000 c3')
        facts = {r['atom']:r['bounds'] for r in p['accesses'][0]['bounds']}
        self.assertEqual(facts[('argument',0,4)],(bounds.MIN,bounds.MAX))

    def test_mask_proves_byte_range_without_using_source_names(self):
        p = self.profile('8b4c2404 81e1ff000000 8b048d00104000 c3')
        facts = {r['atom']:r['bounds'] for r in p['accesses'][0]['bounds']}
        self.assertIn((0,255),facts.values())


if __name__ == '__main__':
    unittest.main()
