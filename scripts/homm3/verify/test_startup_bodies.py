import struct
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import patch

from homm3.verify.startup_bodies import Body, Candidate, match, compare


class StartupBodyTests(unittest.TestCase):
    def fixture(self, *, scalar=7, pointer=0x300, child_pointer=0x300,
                child_call=0x200, sites=(0x102, 0x201), external=False):
        root = b'\xc7\x05'+bytes(4)+struct.pack('<I', 7)+b'\xe8'+bytes(4)+b'\xc3'
        child = b'\xa1'+bytes(4)+b'\xc3'
        bodies = dict(root=Body('root', root, ((2,'object',6),(11,'cleanup',20)),b'',1),
                      cleanup=Body('cleanup',child,((1,'object',6),),b'',1))
        if external:
            del bodies['cleanup']
        actual = bytearray(root)
        struct.pack_into('<I', actual, 2, 0x400000+pointer)
        struct.pack_into('<I', actual, 6, scalar)
        struct.pack_into('<i', actual, 11, child_call-0x10f)
        actual_child = b'\xa1'+struct.pack('<I',0x400000+child_pointer)+b'\xc3'
        payloads = {0x100:bytes(actual), 0x200:actual_child}
        image = SimpleNamespace(image_base=0x400000,
            pe=SimpleNamespace(read=lambda a,n:payloads.get(a,b'')[:n]),
            relocs_in=lambda a,b:[s for s in sites if a<=s<b])
        return SimpleNamespace(body=bodies.get), image, {0x100:len(root),0x200:len(child)}

    def check(self, *, targets=None, **kwargs):
        candidate,image,sizes = self.fixture(**kwargs)
        return match(candidate,'root',0x100,image,sizes,
                     {'object':{0x300}} if targets is None else targets)

    def test_complete_body_and_local_cleanup(self):
        verdict,dependencies,reason = self.check()
        self.assertEqual((verdict,reason),('exact',''))
        self.assertEqual(dependencies,[('cleanup',0x200,6)])

    def test_scalar_pointer_and_relocation_changes_rejected(self):
        for kwargs in (dict(scalar=8),dict(pointer=0x304),dict(child_pointer=0x304),
                       dict(sites=(0x102,)),dict(sites=(0x102,0x105,0x201)),
                       dict(child_call=0x201)):
            with self.subTest(kwargs=kwargs):
                self.assertNotEqual(self.check(**kwargs)[0],'exact')
                self.assertFalse(self.check(**kwargs)[1])

    def test_unknown_data_or_external_code_never_masked(self):
        self.assertEqual(self.check(targets={})[0],'unresolved')
        self.assertEqual(self.check(external=True)[0],'unresolved')

    def test_known_identity_cannot_be_overridden_by_exact_other_body(self):
        self.assertEqual(self.check(targets={'object':{0x300},'cleanup':{0x250}})[0],
                         'mismatch')

    def test_pointer_addend_checked(self):
        candidate,image,sizes = self.fixture(pointer=0x304)
        body=candidate.body('root')
        raw=bytearray(body.payload);struct.pack_into('<i',raw,2,4)
        changed=Body(body.name,bytes(raw),body.relocations,b'',1)
        original=candidate.body
        candidate.body=lambda name:changed if name=='root' else original(name)
        self.assertEqual(match(candidate,'root',0x100,image,sizes,{'object':{0x300}})[0],
                         'exact')
        self.assertNotEqual(match(candidate,'root',0x100,image,sizes,{'object':{0x304}})[0],
                            'exact')

    def test_repeated_symbol_cannot_take_two_addresses(self):
        candidate,image,sizes=self.fixture()
        self.assertEqual(match(candidate,'root',0x100,image,sizes,{'object':{0x300}},
                               assigned={'cleanup':0x250})[0], 'mismatch')

    def test_recursive_unpaired_graph_is_explicitly_unresolved(self):
        candidate,image,sizes=self.fixture()
        self.assertEqual(match(candidate,'root',0x100,image,sizes,{'object':{0x300}},
                               trail=('root',))[0], 'unresolved')

    def integrated(self, *, stale=False, edited_during=False, wrong_scalar=False):
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.model import Model, Binding
        body=b'\xc7\x05'+bytes(4)+struct.pack('<I',7)+b'\xc3'
        raw=_coff((FixtureSection('.text',body+b'\x90'*5,((2,1,6),)),
                   FixtureSection('.data',bytes(4),()),
                   FixtureSection('.CRT$XCU',bytes(4),((0,0,6),))),
                  (_symbol('init',0,1,0x20,2),_symbol('object',0,2,0,2)))
        actual=bytearray(body);struct.pack_into('<I',actual,2,0x400300)
        if wrong_scalar:actual[6]=8
        pe=SimpleNamespace(image_base=0x400000,read=lambda a,n:bytes(actual)[:n])
        image=SimpleNamespace(pe=pe,image_base=pe.image_base,u32=lambda site:0x400300,
                              relocs_in=lambda a,b:[0x102] if a<=0x102<b else [])
        with TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'build/objdiff/base').mkdir(parents=True)
            (root/'tools/bin').mkdir(parents=True)
            path=root/'build/objdiff/base/example.obj';path.write_bytes(raw)
            project=SimpleNamespace(root=root,toolchain=root/'tools',includes=[],
                manifest={'unit':[dict(unit='example',source='source.cpp',flags='f')],
                          'flags':{'f':['/O2']}})
            model=Model([],[Binding(0x300,4,'','data','object','example','src',())],[])
            answers=[None] if stale else [{str(root/'source.cpp'):'hash'},
                                          None if edited_during else {str(root/'source.cpp'):'hash'}]
            with patch('homm3.verify.startup_bodies.read',return_value=([],[],[])), \
                 patch('homm3.verify.startup_bodies.verified_roots',return_value=[0x100]), \
                 patch('homm3.delink.image.Image',return_value=image), \
                 patch('homm3.retail_labels.censuses.functions',return_value=[dict(rva=0x100,size=11)]), \
                 patch('homm3.core.cc_wrap.scan_header_deps',return_value=[]), \
                 patch('homm3.core.compile_receipt.current',side_effect=answers):
                return compare(project,pe,model,[])

    def test_source_anchored_crt_comparison_and_mismatch_report(self):
        result=self.integrated()
        self.assertEqual(len(result['matches']),1)
        self.assertEqual(result['matches'][0]['owners'],
                         [dict(name='object',rva=0x300,size=4)])
        bad=self.integrated(wrong_scalar=True)
        self.assertFalse(bad['matches'])
        self.assertEqual(bad['comparisons'][0]['verdict'],'mismatch')

    def test_stale_or_concurrently_edited_source_cannot_credit_bytes(self):
        for kwargs in (dict(stale=True),dict(edited_during=True)):
            with self.subTest(kwargs=kwargs):
                result=self.integrated(**kwargs)
                self.assertFalse(result['matches'])
                self.assertFalse(result['dependencies'])
                self.assertTrue(result['gaps'])

    def test_shared_cleanup_counted_once_without_new_data_claim(self):
        from homm3.verify.byte_accounting import startup_ranges
        rows=startup_ranges(dict(matches=[dict(rva=0x100,size=16,source='s.cpp')],
                dependencies=[dict(rva=0x200,size=6),dict(rva=0x200,size=6)]))
        self.assertEqual([(r.start,r.end,r.category) for r in rows],
                         [(0x100,0x110,'source-initializer-exact'),
                          (0x200,0x206,'source-cleanup-exact')])


if __name__=='__main__':
    unittest.main()
