from dataclasses import replace
from hashlib import sha256
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.model import Binding, Model
from homm3.core import msvc_names
from homm3.verify.source_static_data import definitions, infer, recover
from homm3.verify.startup_bodies import Body


class SourceStaticDataTests(unittest.TestCase):
    nil = '?_Nil@?$_Tree@H@std@@1PAU_Node@12@A'
    refs = '?_Nilrefs@?$_Tree@H@std@@1IA'

    def fixture(self):
        raw = b'\xa1'+bytes(4)+b'\x8b\x0d'+bytes(4)+b'\xe8'+bytes(4)+b'\xc3'
        body = Body('destroy', raw,
                    ((1,self.nil,6),(7,self.refs,6),(12,'release',20)), b'', 16)
        data = bytearray(0x1000)
        data[0x100:0x111] = raw
        struct.pack_into('<I', data, 0x101, 0x400800)
        struct.pack_into('<I', data, 0x107, 0x400804)
        struct.pack_into('<i', data, 0x10c, 0x200-0x110)
        sites = {0x101,0x107}
        image = SimpleNamespace(image_base=0x400000,
            pe=SimpleNamespace(read=lambda a,n: bytes(data[a:a+n]),
                               data_regions=lambda: {'bss': (0x800,0x900)}),
            payload=lambda a,n: bytes(data[a:a+n]),
            relocs_in=lambda a,b: sorted(s for s in sites if a<=s<b))
        model = Model([
            Binding(0x100,len(raw),'','text','destroy','unit','src_compgen',()),
            Binding(0x200,1,'','text','release','unit','src',())], [], [])
        bodies = {'destroy':body}
        candidate = SimpleNamespace(symbols=bodies, body=bodies.get)
        return model, {'unit':(candidate,{self.nil,self.refs})}, image, data, sites, bodies

    def test_complete_source_body_identifies_typed_statics(self):
        m,c,image,_,_,_ = self.fixture()
        self.assertEqual(infer(m,c,image), [
            (self.nil,0x800,(('unit',0x100),)),
            (self.refs,0x804,(('unit',0x100),))])

    def test_source_anonymous_namespace_keeps_canonical_model_identity(self):
        m,c,image,_,_,bodies = self.fixture()
        raw_name = self.nil.replace('H@std', 'H@?%Z:\\tmp\\lane\\src\\unit.cpp123@std')
        body = bodies['destroy']
        bodies['destroy'] = replace(body, relocations=tuple(
            (off, raw_name if name == self.nil else name, kind)
            for off, name, kind in body.relocations))
        c['unit'][1].remove(self.nil)
        c['unit'][1].add(raw_name)
        names = {name for name, _, _ in infer(m,c,image)}
        self.assertIn(msvc_names.mask(raw_name), names)
        self.assertNotIn(raw_name, names)

    def test_nonrelocation_bytes_known_calls_sites_and_zero_storage_required(self):
        for case in ('instruction','call','missing-site','extra-site','data-site','nonzero','unaligned','outside'):
            m,c,image,data,sites,_ = self.fixture()
            if case=='instruction':data[0x100] ^= 1
            if case=='call':data[0x10c] ^= 1
            if case=='missing-site':sites.remove(0x101)
            if case=='extra-site':sites.add(0x104)
            if case=='data-site':sites.add(0x800)
            if case=='nonzero':data[0x800]=1
            if case=='unaligned':struct.pack_into('<I',data,0x101,0x400801)
            if case=='outside':struct.pack_into('<I',data,0x101,0x400900)
            with self.subTest(case=case):self.assertFalse(infer(m,c,image))

    def test_no_unknown_call_or_unproven_function_identity(self):
        m,c,image,_,_,_ = self.fixture()
        m.functions.pop()
        self.assertFalse(infer(m,c,image))
        m,c,image,_,_,_ = self.fixture()
        m.functions[0] = m.functions[0]._replace(channel='functions_static_libs')
        self.assertFalse(infer(m,c,image))

    def test_known_identity_or_owned_extent_cannot_be_overridden(self):
        for binding in (Binding(0x808,4,'','bss',self.nil,'unit','src',()),
                        Binding(0x7fc,8,'','bss','other','unit','src',())):
            m,c,image,_,_,_ = self.fixture(); m.data.append(binding)
            self.assertFalse(infer(m,c,image))

    def test_two_names_cannot_claim_the_same_storage(self):
        m,c,image,data,_,_ = self.fixture()
        struct.pack_into('<I',data,0x107,0x400800)
        self.assertFalse(infer(m,c,image))

    def test_candidate_addends_are_not_silently_erased(self):
        m,c,image,_,_,bodies = self.fixture()
        body=bodies['destroy'];raw=bytearray(body.payload)
        struct.pack_into('<I',raw,1,4)
        bodies['destroy']=replace(body,payload=bytes(raw))
        self.assertFalse(infer(m,c,image))

    def test_conflicting_whole_body_witnesses_reject_identity(self):
        m,c,image,data,sites,bodies = self.fixture()
        m.functions.append(m.functions[0]._replace(rva=0x300,name='otherDestroy'))
        bodies['otherDestroy']=replace(bodies['destroy'],name='otherDestroy')
        data[0x300:0x311]=data[0x100:0x111]
        struct.pack_into('<I',data,0x301,0x400808)
        struct.pack_into('<i',data,0x30c,0x200-0x310)
        sites.update((0x301,0x307))
        result=infer(m,c,image)
        self.assertFalse(result)

    def test_repeated_symbol_must_refer_to_one_address_within_body(self):
        m,c,image,_,_,bodies = self.fixture()
        b=bodies['destroy']
        bodies['destroy']=replace(b,relocations=((1,self.nil,6),(7,self.nil,6),(12,'release',20)))
        self.assertFalse(infer(m,c,image))

    def test_only_complete_sdk_bss_comdats_supply_extents(self):
        for change in ({},{'size':8},{'comdat':0},{'alignment':1},{'name':'.data'},
                       {'characteristics':0x200010a0}):
            sec=dict(index=1,name='.bss',size=4,comdat=2,alignment=4,
                     characteristics=0xc0301080);sec.update(change)
            obj=SimpleNamespace(section_table=[sec],
                defined_symbols=lambda _: [(0,self.nil)],
                typed_relocations=lambda _: {}, section_payload=lambda _: b'')
            self.assertEqual(bool(definitions(obj)),not change)
        obj.defined_symbols=lambda _: [(0,'ordinaryGlobal')]
        self.assertFalse(definitions(obj))

    def test_freshness_rechecked_after_proof(self):
        for changed in (None,'source','object'):
            with self.subTest(changed=changed), TemporaryDirectory() as tmp:
                root=Path(tmp);(root/'tools/bin').mkdir(parents=True)
                source=root/'source.cpp';source.write_bytes(b'std::map<int,int> table;')
                objpath=root/'unit.obj';objpath.write_bytes(b'raw object')
                obj=SimpleNamespace(buf=objpath.read_bytes())
                expected={str(source):sha256(source.read_bytes()).hexdigest()}
                project=SimpleNamespace(root=root,toolchain=root/'tools',includes=[],
                    manifest={'unit':[{'unit':'unit','source':'source.cpp','flags':'game'}],
                              'flags':{'game':[]}})
                m,_,image,_,_,_=self.fixture()
                def proof(*args):
                    if changed=='source':source.write_bytes(b'changed source')
                    if changed=='object':objpath.write_bytes(b'changed object')
                    return [(self.nil,0x800,(('unit',0x100),))]
                with patch('homm3.verify.source_static_data.coffx.objects',return_value=[('unit',obj)]), \
                     patch('homm3.verify.source_static_data.definitions',return_value={self.nil}), \
                     patch('homm3.verify.source_static_data.Candidate'), \
                     patch('homm3.verify.source_static_data.compile_receipt.current',return_value=expected), \
                     patch('homm3.core.cc_wrap.scan_header_deps',return_value=[]), \
                     patch('homm3.verify.source_static_data.infer',proof), \
                     patch('homm3.delink.image.retail',return_value=image):
                    result=recover(m,project,root)
                self.assertEqual(bool(result),changed is None)


if __name__ == '__main__':
    unittest.main()
