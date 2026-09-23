import unittest

from homm3.analysis.data_accesses import analyze, compare, Storage
from homm3.analysis.data_consumers import relationships
from homm3.analysis.access_expressions import Expression as E


class ConsumerContractsTest(unittest.TestCase):
    def test_unique_factor_pair_reports_doubled_byte_coefficient(self):
        prefix='8b442404 8b4c2408 8b54240c 0fafca '
        a,b=(analyze(bytes.fromhex(prefix+suffix)) for suffix in ('8b1408 c3','8b1448 c3'))
        changes=compare(a,b)['scale_changes']
        self.assertEqual(len(changes),1)
        self.assertEqual(changes[0]['retail_byte_coefficient'],1)
        self.assertEqual(changes[0]['candidate_byte_coefficient'],2)

    def test_ambiguous_same_shape_sites_do_not_get_paired_by_ordinal(self):
        prefix='8b442404 8b4c2408 '
        a=analyze(bytes.fromhex(prefix+'8b1488 8b1488 c3'))
        b=analyze(bytes.fromhex(prefix+'8b1448 8b1448 c3'))
        self.assertFalse(compare(a,b)['scale_changes'])

    def test_changed_guard_limit_is_visible_with_identical_address_expression(self):
        a=analyze(bytes.fromhex('8b4c2404 83f90a 7307 8b048d00104000 c3'))
        b=analyze(bytes.fromhex('8b4c2404 83f90b 7307 8b048d00104000 c3'))
        result=compare(a,b)
        self.assertEqual(result['status'],'observed-domains-differ')
        self.assertEqual(len(result['range_changes']),1)

    def test_split_writer_and_reader_bases_are_visible_across_functions(self):
        rows=[]
        # Retail producer and consumer share storage 100; candidate producer
        # writes 200 while its reader still uses 100.
        for side,fid,owner,mode in [('retail',0,100,'write'),('retail',1,100,'read'),
                                    ('candidate',2,200,'write'),('candidate',3,100,'read')]:
            rows.append(dict(side=side,function_id=fid,access=mode,extent=dict(owner_rva=owner)))
        pairs=[dict(unit='u',symbol=str(i),rva=10+i,retail_function_id=i,candidate_function_id=i+2,
                    complete_observation=True) for i in range(2)]
        storage,issues=relationships(rows,pairs,dict(declarations=[]),[])
        self.assertEqual(len(issues),1)
        self.assertEqual(issues[0]['retail_only'],[100])
        self.assertEqual(issues[0]['candidate_only'],[200])
        self.assertEqual(storage[0]['retail_write_functions'],[0])
        self.assertEqual(storage[0]['candidate_write_functions'],[])
        self.assertEqual(storage[0]['candidate_read_functions'],[3])

    def test_projection_crossing_retains_distinct_emitted_span_evidence(self):
        storage=Storage(0x400000,[dict(rva=0x1000,size=1,physical_size=4,unit='u',status='enrolled')])
        row=storage.extent(storage.normalize(E.constant(0x401000)),4)
        self.assertEqual(row['status'],'outside-source-extent')
        self.assertTrue(row['within_all_emitted_spans'])
        self.assertEqual(row['source_size'],1)
        self.assertEqual(row['emitted_spans'],[dict(unit='u',size=4)])


if __name__ == '__main__':
    unittest.main()
