import struct
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import patch

from homm3.verify.startup_bodies import Body, Candidate, match, compare, helper_source_anchors


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

    def test_helper_anchors_do_not_borrow_known_callee_implementation(self):
        owner = SimpleNamespace(rva=0x300)
        root = Body('root', b'\xe8'+bytes(4)+b'\xc3', ((1,'helper',20),), b'', 1)
        helper = Body('helper', b'\xa1'+bytes(4)+b'\xc3', ((1,'object',6),), b'', 1)
        candidate = SimpleNamespace(body={'root':root, 'helper':helper}.get)
        self.assertEqual(helper_source_anchors(candidate,root,{'object':owner},{}),[owner])
        self.assertFalse(helper_source_anchors(candidate,root,{},{}))
        self.assertFalse(helper_source_anchors(candidate,root,{'object':owner},
                                              {'helper':{0x200}}))

    def test_helper_anchor_cycles_and_depth_limit_are_not_partial_proof(self):
        owner = SimpleNamespace(rva=0x300)
        def chain(count, cycle=False):
            bodies = {str(i):Body(str(i), b'\xe8'+bytes(4)+b'\xc3',
                                 ((1,str(i+1),20),), b'',1) for i in range(count-1)}
            last = str(count-1)
            refs = ((1,'object',6),) + (((6,'0',20),) if cycle else ())
            payload = b'\xa1'+bytes(4)+(b'\xe8'+bytes(4) if cycle else b'')+b'\xc3'
            bodies[last] = Body(last,payload,refs,b'',1)
            return helper_source_anchors(SimpleNamespace(body=bodies.get),
                                         bodies['0'],{'object':owner},{})
        self.assertEqual(chain(32),[owner])
        self.assertFalse(chain(33))
        self.assertFalse(chain(2,cycle=True))

    def integrated(self, *, stale=False, edited_during=False, wrong_scalar=False,
                   wrapped=False, extra_root=False, no_owner=False, known_child=False):
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.model import Model, Binding
        body=b'\xc7\x05'+bytes(4)+struct.pack('<I',7)+b'\xc3'
        raw=_coff((FixtureSection('.text',body+b'\x90'*5,((2,1,6),)),
                   FixtureSection('.data',bytes(4),()),
                   FixtureSection('.CRT$XCU',bytes(4),((0,0,6),))),
                  (_symbol('init',0,1,0x20,2),_symbol('object',0,2,0,2)))
        actual=bytearray(body);struct.pack_into('<I',actual,2,0x400300)
        if wrong_scalar:actual[6]=8
        payloads = {0x100: bytes(actual)}
        sizes = {0x100: len(body)}
        sites = [0x102]
        roots = [0x100]
        if wrapped:
            wrapper = b'\xe8' + bytes(4) + b'\xc3'
            raw = _coff((
                FixtureSection('.text', wrapper + b'\x90'*10, ((1, 1, 20),)),
                FixtureSection('.text', body + b'\x90'*5, ((2, 2, 6),)),
                FixtureSection('.data', bytes(4), ()),
                FixtureSection('.CRT$XCU', bytes(4), ((0, 0, 6),))),
                (_symbol('init', 0, 1, 0x20, 2),
                 _symbol('helper', 0, 2, 0x20, 2), _symbol('object', 0, 3, 0, 2)))
            if extra_root:
                roots.append(0x110)
            payloads = {r: b'\xe8' + struct.pack('<i', 0x200-r-5) + b'\xc3'
                        for r in roots}
            payloads[0x200] = bytes(actual)
            sizes = {r: len(b) for r, b in payloads.items()}
            sites = [0x202]
        pe=SimpleNamespace(image_base=0x400000,read=lambda a,n:payloads.get(a,b'')[:n])
        image=SimpleNamespace(pe=pe,image_base=pe.image_base,u32=lambda site:0x400300,
                              relocs_in=lambda a,b:[s for s in sites if a<=s<b])
        with TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'build/objdiff/base').mkdir(parents=True)
            (root/'tools/bin').mkdir(parents=True)
            path=root/'build/objdiff/base/example.obj';path.write_bytes(raw)
            project=SimpleNamespace(root=root,toolchain=root/'tools',includes=[],
                manifest={'unit':[dict(unit='example',source='source.cpp',flags='f')],
                          'flags':{'f':['/O2']}})
            model=Model([Binding(0x210,11,'','func','helper','example','src',())]
                        if known_child else [],
                        [] if no_owner else
                        [Binding(0x300,4,'','data','object','example','src',())],[])
            answers=[None] if stale else [{str(root/'source.cpp'):'hash'},
                                          None if edited_during else {str(root/'source.cpp'):'hash'}]
            with patch('homm3.verify.startup_bodies.read',return_value=([],[],[])), \
                 patch('homm3.verify.startup_bodies.verified_roots',return_value=roots), \
                 patch('homm3.delink.image.Image',return_value=image), \
                 patch('homm3.retail_labels.censuses.functions',return_value=[
                     dict(rva=r,size=n) for r,n in sizes.items()]), \
                 patch('homm3.core.cc_wrap.scan_header_deps',return_value=[]), \
                 patch('homm3.core.compile_receipt.current',side_effect=answers):
                return compare(project,pe,model,[])

    def test_crt_wrapper_is_anchored_by_recursively_compared_owner(self):
        result = self.integrated(wrapped=True)
        self.assertEqual([r['rva'] for r in result['matches']], [0x100])
        self.assertEqual([(r['symbol'], r['rva']) for r in result['dependencies']],
                         [('helper', 0x200)])
        self.assertEqual(result['matches'][0]['owners'],
                         [dict(name='object', rva=0x300, size=4)])

    def test_wrapped_roots_still_require_complete_unique_fresh_proof(self):
        for kwargs in (dict(wrong_scalar=True), dict(extra_root=True),
                       dict(no_owner=True), dict(known_child=True),
                       dict(stale=True), dict(edited_during=True)):
            with self.subTest(kwargs=kwargs):
                result = self.integrated(wrapped=True, **kwargs)
                self.assertFalse(result['matches'])
                self.assertFalse(result['dependencies'])

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
