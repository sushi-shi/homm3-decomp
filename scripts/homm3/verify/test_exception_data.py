from dataclasses import replace
import struct
from types import SimpleNamespace
import unittest

from homm3.delink.exception_data import Record, supported, resolve


class ExceptionDataTests(unittest.TestCase):
    descriptor = Record('??_R0?AVFoo@@@8', 'unit', bytes(8)+b'.?AVFoo@@\0',
                        ((0, '??_7type_info@@6B@', 6),), 4)
    catchable = Record('__CT??_R0?AVFoo@@@84', 'unit',
                       struct.pack('<IIiiiII', 0, 0, 0, -1, 0, 4, 0),
                       ((4, descriptor.name, 6), (24, 'copy', 6)), 4)
    array = Record('__CTA1?AVFoo@@', 'unit', struct.pack('<II', 1, 0),
                   ((4, catchable.name, 6),), 4)
    info = Record('__TI1?AVFoo@@', 'unit', bytes(16),
                  ((4, 'destroy', 6), (12, array.name, 6)), 4)
    records = (descriptor, catchable, array, info)
    addresses = (0x100, 0x120, 0x140, 0x150)

    def fixture(self, *, extra=(), duplicate_descriptor=False):
        data = bytearray(0x400)
        known = {'??_7type_info@@6B@': {0x900}, 'copy': {0x800}, 'destroy': {0x820}}
        bindings = dict(zip((r.name for r in self.records), self.addresses))
        bindings.update({name: next(iter(values)) for name, values in known.items()})
        sites = set(extra)
        for record, rva in zip(self.records, self.addresses):
            data[rva:rva+len(record.payload)] = record.payload
            for offset, target, _ in record.relocations:
                struct.pack_into('<I', data, rva+offset, 0x400000+bindings[target])
                sites.add(rva+offset)
        if duplicate_descriptor:
            data[0x200:0x200+len(self.descriptor.payload)] = data[0x100:0x100+len(self.descriptor.payload)]
            sites.add(0x200)
        image = SimpleNamespace(image_base=0x400000, reloc_sites=sorted(sites),
            u32=lambda r: struct.unpack_from('<I', data, r)[0],
            relocs_in=lambda a, b: [r for r in sorted(sites) if a <= r < b],
            pe=SimpleNamespace(sections=[dict(name='.data', va=0x100, rsize=0x300)],
                               read=lambda a, n: bytes(data[a:a+n])))
        return image, known, data, sites

    def test_graph_matches_bytes_and_names_without_masking(self):
        image, known, _, _ = self.fixture()
        self.assertTrue(all(supported(r) for r in self.records))
        matched, withheld = resolve(list(reversed(self.records)), image, known)
        self.assertEqual({r.name: a for r, a in matched},
                         dict(zip((r.name for r in self.records), self.addresses)))
        self.assertEqual(withheld, [])

    def test_unknown_copy_constructor_cannot_credit_descendants(self):
        image, known, _, _ = self.fixture()
        del known['copy']
        matched, withheld = resolve(self.records, image, known)
        self.assertEqual([r.name for r, _ in matched], [self.descriptor.name])
        self.assertEqual(len(withheld), 3)

    def test_wrong_addend_and_missing_or_extra_relocations_are_visible(self):
        for mode in ('wrong pointer', 'missing relocation', 'extra relocation', 'scalar'):
            image, known, data, sites = self.fixture()
            if mode == 'wrong pointer':
                struct.pack_into('<I', data, 0x138, 0x400801)
            elif mode == 'missing relocation':
                sites.remove(0x138)
            elif mode == 'extra relocation':
                sites.add(0x130)
            else:
                data[0x134] ^= 1
            image.reloc_sites = sorted(sites)
            matched, withheld = resolve(self.records, image, known)
            with self.subTest(mode=mode):
                self.assertEqual([r.name for r, _ in matched], [self.descriptor.name])

    def test_full_typename_and_unambiguous_address_are_required(self):
        wrong = replace(self.descriptor, payload=bytes(8)+b'.?AVBar@@\0')
        self.assertFalse(supported(wrong))
        image, known, _, _ = self.fixture(duplicate_descriptor=True)
        matched, withheld = resolve(self.records, image, known)
        self.assertFalse(matched)
        self.assertTrue(any('ambiguous' in why for _, _, why in withheld))

    def test_copies_cannot_disagree_but_matching_copies_remain_enrolled(self):
        image, known, _, _ = self.fixture()
        copied = replace(self.descriptor, unit='second')
        rows, withheld = resolve([*self.records, copied], image, known)
        self.assertEqual(len(rows), 5)
        self.assertFalse(withheld)
        copied = replace(copied, payload=copied.payload[:-2]+b'X\0')
        rows, withheld = resolve([*self.records, copied], image, known)
        self.assertFalse(rows)
        self.assertTrue(any('copies disagree' in why for _, _, why in withheld))

    def test_conflicting_identity_cannot_anchor_another_record(self):
        image, known, _, _ = self.fixture()
        alias = replace(self.catchable, name=self.catchable.name+'alias')
        rows, withheld = resolve([*self.records, alias], image, known)
        self.assertEqual([r.name for r, _ in rows], [self.descriptor.name])
        self.assertTrue(any('overlaps' in why for _, _, why in withheld))

    def test_manifest_retains_exception_comdat_topology(self):
        from pathlib import Path
        from tempfile import TemporaryDirectory
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        from homm3.delink.data_manifest import section_rows
        payload = struct.pack('<II', 1, 0)
        obj = _coff((FixtureSection('.xdata$x', payload, ((4, 1, 6),)),),
                    (_symbol('__CTA1_N', 0, 1, 0, 2),
                     _symbol('catch', 0, 0, 0, 2)))
        row = dict(name='__CTA1_N', object='unit.c', rva=0x100, size=8,
                   storage='rdata', alignment=4, section_placed=True,
                   provenance='candidate-COFF-exception-exact')
        with TemporaryDirectory() as tmp:
            Path(tmp, 'unit.obj').write_bytes(obj)
            sections, withheld = section_rows([row], Path(tmp))
        self.assertFalse(withheld)
        self.assertEqual([(s['name'], s['rva'], s['size'], s['storage']) for s in sections],
                         [('.xdata$x', 0x100, 8, 'rdata')])
        self.assertEqual(row['section_ordinal'], 1)


if __name__ == '__main__':
    unittest.main()
