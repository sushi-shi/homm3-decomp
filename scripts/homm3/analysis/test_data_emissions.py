"""Compiler copies share ownership only through inputs/layout, never values."""
import hashlib
import struct
import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest import mock

from homm3.analysis import candidate_data, data_emissions as emissions
from homm3.analysis.test_candidate_data import coff
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.test_eh_handler_normalization import _symbol
from homm3.sema import data_match
from homm3.sema.retail_layout import Layout
from homm3.sema.test_data_match import binding
from homm3.sema.test_retail_layout import fixture


def stamp(obj, **changes):
    return dict(dict(schema=1, source='/source/a.c', flags=['/O2','/TC'], includes=['/include'],
                     toolchain='/vc6', files={'/source/a.c':'source-digest'}, trees={'/include':'header-digest'},
                     object_sha256=hashlib.sha256(obj.data).hexdigest()), **changes)


class DataEmissionsTest(unittest.TestCase):
    def prove(self, a, b, **changes):
        return emissions.prove('a', '/source/a.c', a, stamp(a), b, stamp(b, **changes))

    def test_clock_and_initializer_changes_do_not_choose_the_passing_copy(self):
        a, b = coff(), coff(raw=b'Abcdefgh')
        changed = bytearray(b.data)
        struct.pack_into('<I', changed, 4, 123456)
        b = CoffObject(changed)
        proof = self.prove(a, b)
        self.assertEqual(proof['status'], 'same-manifest-emission')
        self.assertNotEqual(proof['source_object_sha256'], proof['copy_object_sha256'])

    def test_equal_bytes_do_not_overrule_different_compiler_inputs(self):
        a = coff()
        for changes in [dict(flags=['/Od','/TC']), dict(source='/other/a.c'),
                        dict(trees={'/include':'changed'}), dict(toolchain='/other-vc6'),
                        dict(files={'/source/a.c':'changed'})]:
            proof = self.prove(a, a, **changes)
            self.assertEqual(proof['status'], 'different-compiler-inputs')
            self.assertFalse(proof['identity'])
        different_unit = emissions.prove('b', '/source/a.c', a, stamp(a), a, stamp(a))
        self.assertNotEqual(different_unit['identity'], self.prove(a, a)['identity'])

    def test_object_provenance_must_describe_the_actual_bytes(self):
        with self.assertRaisesRegex(ValueError, 'compiler provenance'):
            self.prove(coff(), coff(), object_sha256='wrong')
        with self.assertRaisesRegex(ValueError, 'incomplete compiler provenance'):
            emissions.prove('a','/source/a.c',coff(),{},coff(),{})

    def test_archive_members_and_unadmitted_units_are_not_compilation_copies(self):
        project=SimpleNamespace(manifest={'unit':[dict(unit='a',source='src/a.c')]})
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(emissions,'Project',return_value=project):
            for unit in ('','unadmitted'):
                copy=SimpleNamespace(unit=unit,coff=coff(),origin='archive.lib')
                self.assertEqual(emissions.identify(Path(tmp),{'a':coff()},{'copy':copy}),[])
            copy=SimpleNamespace(unit='a',coff=coff(),origin='copy.obj')
            with self.assertRaises(OSError):
                emissions.identify(Path(tmp),{'a':coff()},{'copy':copy})

    def test_changed_extent_symbol_or_linker_directive_is_not_the_same_layout(self):
        a = coff()
        for b in [coff(raw=b'abcdefghi'), coff(symbols=[_symbol('_other', 0, 1, 0, 2)]),
                  coff(section='.rdata')]:
            self.assertEqual(self.prove(a, b)['status'], 'different-coff-layout')
        a, b = coff(section='.drectve', raw=b'/lib:aaa'), coff(section='.drectve', raw=b'/lib:bbb')
        self.assertEqual(self.prove(a, b)['status'], 'different-coff-layout')

    def test_relocation_graph_changes_do_not_reuse_copy_identity(self):
        symbols=[_symbol('_ptr', 0, 1, 0, 2), _symbol('_one', 4, 1, 0, 2),
                 _symbol('_two', 6, 1, 0, 2)]
        a=coff(raw=bytes(8), symbols=symbols, relocations=((0,1,6),))
        b=coff(raw=bytes(8), symbols=symbols, relocations=((0,2,6),))
        self.assertEqual(self.prove(a,b)['status'], 'different-coff-layout')

    def copies(self, *, bad=False, part_size=5, part_kind='source', full_size=8, full_kind='coff-contribution'):
        a, b = coff(), coff(raw=b'Abcdefgh' if bad else b'abcdefgh')
        rows=candidate_data.inventory('a',a,'a')+candidate_data.inventory('copy',b,'b')
        proof=self.prove(a,b)
        proof['allocation_pairs']=[dict(source_id=rows[0]['id'],copy_id=rows[1]['id'],
                                       identity=proof['identity']+':1:0',physical_size=8)]
        bindings=[binding(rows[0],0x2000,part_size,macro='CODE',extent_kind=part_kind),
                  binding(rows[1],0x2000,full_size,macro='VENDOR',extent_kind=full_kind)]
        emissions.attach([proof],bindings)
        return rows,bindings,{'a':a,'copy':b}

    def test_logical_extent_and_physical_span_remain_separate_comparisons(self):
        rows,bindings,objects=self.copies()
        image=fixture();layout=Layout(image);start=layout.raw(0x2000,8)
        image[start:start+8]=b'abcdefgh'
        report=data_match.compare(Layout(image),rows,bindings,objects)
        self.assertEqual(report['summary']['matched_initialized_bytes'],8)
        self.assertEqual(report['summary']['compared_allocations'],2)
        self.assertEqual([(r['size'],r['extent_kind']) for r in report['enrollment']],
                         [(5,'source'),(8,'coff-contribution')])

    def test_one_bad_copy_remains_a_byte_difference_even_when_another_matches(self):
        rows,bindings,objects=self.copies(bad=True)
        image=fixture();layout=Layout(image);start=layout.raw(0x2000,8)
        image[start:start+8]=b'abcdefgh'
        report=data_match.compare(Layout(image),rows,bindings,objects)
        self.assertEqual(report['summary']['bytes_by_status']['fixed-mismatch'],1)
        self.assertEqual(report['summary']['matched_initialized_bytes'],7)
        self.assertEqual(report['summary']['static_exact_allocations'],1)

    def test_only_the_complete_physical_span_can_overlap_a_smaller_typed_extent(self):
        for options in [dict(full_kind='source'), dict(full_size=7)]:
            rows,bindings,_=self.copies(**options)
            self.assertEqual({p['status'] for p in data_match.enroll(rows,bindings)}, {'binding-conflict'})
        rows,bindings,_=self.copies()
        bindings[1]['emission_physical_size']=9
        with self.assertRaisesRegex(ValueError,'raw allocation'):
            data_match.enroll(rows,bindings)

    def test_competing_copy_placements_withhold_the_otherwise_passing_copy(self):
        _,bindings,_=self.copies()
        bindings[1].update(rva=0x2010,status='source-extent-unproved')
        rows,_,_=self.copies()
        records=[dict(allocation_pairs=[dict(source_id=rows[0]['id'],copy_id=rows[1]['id'],
                                             identity='same',physical_size=8)])]
        emissions.attach(records,bindings)
        self.assertEqual(bindings[0]['status'],'conflicting-emission-placement')
        self.assertEqual(bindings[1]['status'],'source-extent-unproved')

    def pointer_copies(self, *, addend=0, target_binding=True, proof=True, both_pointers=False):
        symbols=[_symbol('_pointer',0,1,0,3),_symbol('_target',4,1,0,3)]
        a=coff(raw=struct.pack('<I',addend)+b'abcd',symbols=symbols,relocations=((0,1,6),))
        b=coff(raw=struct.pack('<I',0)+b'abcd',symbols=symbols,relocations=((0,1,6),))
        rows=candidate_data.inventory('a',a,'a')+candidate_data.inventory('copy',b,'b')
        bindings=[binding(rows[0],0x2000,4,macro='CODE')]
        if target_binding:bindings.append(binding(rows[3],0x2010,4,macro='VENDOR'))
        if both_pointers:bindings.append(binding(rows[2],0x2000,4,macro='VENDOR'))
        record=self.prove(a,b)
        if proof:
            record['allocation_pairs']=[dict(source_id=rows[i]['id'],copy_id=rows[i+2]['id'],
                                            identity=record['identity']+str(i),physical_size=4) for i in (0,1)]
        emissions.attach([record],bindings,rows)
        image=fixture();layout=Layout(image)
        at=layout.raw(0x2000,4);image[at:at+4]=struct.pack('<I',0x402010)
        at=layout.raw(0x2010,4);image[at:at+4]=b'abcd'
        return data_match.compare(Layout(image),rows,bindings,{'a':a,'copy':b})

    def test_copy_pointer_requires_an_independently_placed_owner(self):
        report=self.pointer_copies()
        self.assertEqual(report['relocations'][0]['status'],'pointer-match')
        anchor=report['relocations'][0]['anchors'][0]
        self.assertEqual(anchor['candidate_id'],'copy:1:4')
        self.assertEqual(anchor['emission_candidate_id'],'a:1:4')
        for options in [dict(target_binding=False),dict(proof=False),dict(addend=5)]:
            self.assertEqual(self.pointer_copies(**options)['relocations'][0]['status'],'pointer-unresolved')

    def test_wrong_copy_pointer_addend_still_differs_when_another_copy_matches(self):
        report=self.pointer_copies(addend=1,both_pointers=True)
        self.assertEqual({r['status'] for r in report['relocations']},{'pointer-match','pointer-mismatch'})
        self.assertEqual(report['summary']['bytes_by_status']['pointer-mismatch'],4)


if __name__ == '__main__':
    unittest.main()
