from dataclasses import replace
from hashlib import sha256
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.model import Binding, Model
from homm3.verify.source_function_aliases import infer, recover
from homm3.verify.startup_bodies import Body
from homm3.verify import test_exception_data as exception_fixture


class SourceFunctionAliasTests(unittest.TestCase):
    alias = '??1Foo@@UAE@XZ'

    def fixture(self, *, extra_anchor=False):
        fixture = exception_fixture.ExceptionDataTests()
        image, known, original, sites = fixture.fixture()
        data = original + bytearray(0xc00)
        records = [replace(r, relocations=tuple(
            (off, self.alias if name == 'destroy' else name, kind)
            for off, name, kind in r.relocations)) for r in fixture.records]
        raw = b'\xa1' + bytes(4) + b'\xc3'
        body = Body('baseDestructor', raw, ((1, 'value', 6),), b'', 1)
        bodies = {'baseDestructor': body, self.alias: replace(body, name=self.alias)}
        functions = [Binding(0x820, 6, '', 'text', 'baseDestructor', 'unit', 'src', ()),
                     Binding(0x800, 1, '', 'text', 'copy', 'unit', 'src', ())]
        anchors = [0x820]
        if extra_anchor:
            functions.append(Binding(0x840, 6, '', 'text', 'other', 'unit', 'src', ()))
            bodies['other'] = replace(body, name='other')
            anchors.append(0x840)
        for rva in anchors:
            data[rva:rva+6] = b'\xa1' + struct.pack('<I', 0x400980) + b'\xc3'
            sites.add(rva+1)
        image.reloc_sites = sorted(sites)
        image.pe.read = lambda a, n: bytes(data[a:a+n])
        image.u32 = lambda a: struct.unpack_from('<I', data, a)[0]
        model = Model(functions, [
            Binding(0x900, 4, '', 'data', '??_7type_info@@6B@', '', 'data_vtables', ()),
            Binding(0x980, 4, '', 'data', 'value', 'unit', 'src', ())], [])
        candidate = SimpleNamespace(symbols=bodies, body=bodies.get)
        return model, records, {'unit': candidate}, image, data

    def test_body_and_type_identified_record_prove_one_alias(self):
        m, records, candidates, image, _ = self.fixture()
        self.assertEqual(infer(m, records, candidates, image),
                         [(self.alias, 0x820, 'unit', '__TI1?AVFoo@@', 0x150)])
        self.assertEqual(len(m.functions), 2)  # no extra body/score entry

    def test_identical_bodies_without_complete_metadata_do_not_prove_alias(self):
        for offset in (0x108, 0x134, 0x154):
            m, records, candidates, image, data = self.fixture()
            data[offset] ^= 1
            with self.subTest(offset=offset):
                self.assertFalse(infer(m, records, candidates, image))

    def test_retail_instruction_and_named_reference_differences_reject_alias(self):
        for offset in (0x820, 0x821):
            m, records, candidates, image, data = self.fixture()
            data[offset] ^= 1
            with self.subTest(offset=offset):
                self.assertFalse(infer(m, records, candidates, image))

    def test_ambiguous_representative_is_not_picked(self):
        m, records, candidates, image, _ = self.fixture(extra_anchor=True)
        self.assertFalse(infer(m, records, candidates, image))

    def test_known_identity_cannot_be_replaced_by_a_different_alias(self):
        m, records, candidates, image, _ = self.fixture()
        m.functions.append(Binding(0x840, 6, '', 'text', self.alias, 'unit', 'src', ()))
        self.assertFalse(infer(m, records, candidates, image))

    def test_source_or_object_change_during_comparison_cannot_publish_alias(self):
        for changed in (None, 'source', 'object'):
            with self.subTest(changed=changed), TemporaryDirectory() as tmp:
                root = Path(tmp)
                (root/'tools/bin').mkdir(parents=True)
                source, obj = root/'source.cpp', root/'unit.obj'
                source.write_bytes(b'class Foo {};')
                obj.write_bytes(b'raw COFF witness')
                inputs = {str(source): sha256(source.read_bytes()).hexdigest()}
                witness = SimpleNamespace(buf=obj.read_bytes())
                m, _, _, _, _ = self.fixture()
                def candidates(project, base_dir, *, witnesses):
                    witnesses['unit'] = (witness, inputs)
                    return [], []
                def proof(*args):
                    if changed == 'source':
                        source.write_bytes(b'class Foo { int added; };')
                    elif changed == 'object':
                        obj.write_bytes(b'recompiled')
                    return [(self.alias, 0x820, 'unit', '__TI1?AVFoo@@', 0x150)]
                project = SimpleNamespace(toolchain=root/'tools')
                with patch('homm3.verify.source_function_aliases.exception_data.candidates', candidates), \
                     patch('homm3.verify.source_function_aliases.Candidate'), \
                     patch('homm3.verify.source_function_aliases.infer', proof), \
                     patch('homm3.delink.image.retail'):
                    result = recover(m, project, root)
                self.assertEqual(bool(result), changed is None)


if __name__ == '__main__':
    unittest.main()
