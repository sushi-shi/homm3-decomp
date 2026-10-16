"""Global address bindings must survive delinker-created data copies."""
import struct
import unittest
from homm3.build import identity_relocations as ir
from homm3.build.test_eh_handler_normalization import FixtureSection,_coff,_symbol
from homm3.compare.canonicalize import CoffObject


def fixture(*, candidate_storage=2, candidate_defined=False, target_storage=2,
            target_code=False, second_addend=4, name="_global"):
    body=b'\xa1'+bytes(4)+b'\xa1'+struct.pack('<I',second_addend)+b'\xc3'
    base=_coff((FixtureSection('.text',body,((1,1,6),(6,1,6))),),
               (_symbol('_f',0,1,0x20,2),
                _symbol(name,0,1 if candidate_defined else 0,0,candidate_storage)))
    target=_coff((FixtureSection('.text',body,((1,1,6),(6,2,6))),
                  FixtureSection('.data',bytes(16),())),
                 (_symbol('_f',0,1,0x20,2),_symbol(name,0,2,0,target_storage),
                  _symbol(name,8,2,0,target_storage)))
    if not target_code:
        c=CoffObject(target);data=bytearray(target)
        struct.pack_into('<I',data,c.sections[1].header_offset+36,0xc0000040)
        target=bytes(data)
    return base,target


class GlobalDataReferenceTests(unittest.TestCase):
    def test_model_bound_external_reference_ignores_delinker_copies(self):
        base,target=fixture()
        after,count=ir.canonicalize(base,target,{'_global':(0x5000,'data')},{})
        self.assertEqual(count,2)
        old,new=CoffObject(target),CoffObject(after)
        self.assertEqual(old.section_bytes(old.sections[1]),new.section_bytes(new.sections[1]))
        for relocation in new.relocations:
            symbol=new.symbols[relocation.symbol_index]
            self.assertEqual((symbol.name,symbol.section),('_global',0))
        self.assertEqual(new.section_bytes(new.sections[0]),old.section_bytes(old.sections[0]))

    def test_defined_global_uses_its_model_address_not_copy_offset(self):
        _base, target = fixture()
        body = b'\xa1' + bytes(4) + b'\xa1' + struct.pack('<I', 4) + b'\xc3'
        base = _coff((FixtureSection('.text', body, ((1, 1, 6), (6, 1, 6))),
                      FixtureSection('.data', bytes(16), ())),
                     (_symbol('_f', 0, 1, 0x20, 2), _symbol('_global', 4, 2, 0, 2)))
        obj = CoffObject(base)
        data = bytearray(base)
        struct.pack_into('<I', data, obj.sections[1].header_offset + 36, 0xc0000040)
        base = bytes(data)
        after, count = ir.canonicalize(base, target, {'_global': (0x5000, 'data')}, {})
        self.assertEqual(count, 2)
        normalized = CoffObject(after)
        self.assertTrue(all(normalized.symbols[r.symbol_index].section == 0
                            for r in normalized.relocations))
        self.assertEqual(ir.canonicalize(base, target, {}, {}), (target, 0))

    def test_unproven_conflicting_or_nondata_binding_stays_visible(self):
        base,target=fixture()
        for authority,identities,unit in (
                ({},{},''),({'_global':(0x5000,'func')},{},''),
                ({'_global':(0x5000,'data')},{('', '_global'):{0x6000}},''),
                ({'_global':(0x5000,'data')},{('u','_global'):{0x6000}},'u'),
                ({'_global':(0x5000,'data')},{('u','_global'):{0x5000,0x6000}},'u')):
            with self.subTest(authority=authority,identities=identities,unit=unit):
                self.assertEqual(ir.canonicalize(base,target,authority,identities,unit=unit),(target,0))

    def test_local_candidate_or_code_definition_is_not_a_global_data_import(self):
        for options in ({'candidate_storage':3},{'candidate_defined':True},
                        {'target_storage':3},{'target_code':True}):
            with self.subTest(options=options):
                base,target=fixture(**options)
                self.assertEqual(ir.canonicalize(base,target,{'_global':(0x5000,'data')},{}),(target,0))

    def test_content_named_literal_copies_need_their_separate_proof(self):
        for name in ('??_C@x', '__real@x'):
            with self.subTest(name=name):
                base, target = fixture(name=name)
                self.assertEqual(ir.canonicalize(
                    base, target, {name: (0x5000, 'data')}, {}), (target, 0))

    def test_different_addend_is_preserved(self):
        base,_=fixture()
        _,target=fixture(second_addend=8)
        after,count=ir.canonicalize(base,target,{'_global':(0x5000,'data')},{})
        self.assertEqual(count,1)
        obj=CoffObject(after)
        relocation=next(r for r in obj.relocations if r.site==6)
        self.assertGreater(obj.symbols[relocation.symbol_index].section,0)
        self.assertEqual(struct.unpack_from('<I',obj.section_bytes(obj.sections[0]),6)[0],8)
