from dataclasses import replace
from hashlib import sha256
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.delink import exception_data, exception_identities as identities
from homm3.model import Binding, Model
from homm3.verify.source_function_aliases import infer
from homm3.verify.startup_bodies import Body
from homm3.verify.test_exception_data import ExceptionDataTests


class ExceptionIdentityTests(unittest.TestCase):
    raw_scope = r'?%Z:\checkout\src\unit.cpp123@'
    retail_scope = r'?%C:\Game\unit.cpp456@'
    policy = {('unit', 'unit.cpp'): r'C:\Game\unit.cpp456'}

    def records(self, scope):
        def name(value):
            return value.replace('Foo@@', 'Foo@' + scope + '@').replace(
                'destroy', '??1Foo@' + scope + '@UAE@XZ')
        result = []
        for record in ExceptionDataTests.records:
            payload = record.payload
            renamed = name(record.name)
            if renamed.startswith('??_R0'):
                payload = payload[:8] + b'.' + renamed[5:-2].encode() + b'\0'
            result.append(replace(record, name=renamed, payload=payload,
                relocations=tuple((off, name(target), kind)
                                  for off, target, kind in record.relocations)))
        return result

    def fixture(self):
        fixture = ExceptionDataTests()
        fixture.records = self.records(self.retail_scope)
        fixture.descriptor = fixture.records[0]
        fixture.addresses = (0x100, 0x200, 0x240, 0x250)
        # Fixture's fixed known 'destroy' anchor is renamed in the metadata.
        fixture.records[-1] = replace(fixture.records[-1], relocations=(
            (4, 'destroy', 6), (12, fixture.records[2].name, 6)))
        image, known, data, sites = fixture.fixture()
        data.extend(bytes(0xc00))
        data[0x820:0x826] = b'\xa1' + struct.pack('<I', 0x400980) + b'\xc3'
        sites.add(0x821)
        image.reloc_sites = sorted(sites)
        raw = self.records(self.raw_scope)
        alias = raw[-1].relocations[0][1]
        body = Body('baseDestructor', b'\xa1' + bytes(4) + b'\xc3',
                    ((1, 'value', 6),), b'', 1)
        bodies = {'baseDestructor': body, alias: replace(body, name=alias)}
        candidate = SimpleNamespace(symbols=bodies, body=bodies.get)
        model = Model([
            Binding(0x820, 6, '', 'text', 'baseDestructor', 'unit', 'src', ()),
            Binding(0x800, 1, '', 'text', 'copy', 'unit', 'src', ())], [
            Binding(0x900, 4, '', 'data', '??_7type_info@@6B@', '', 'data_vtables', ()),
            Binding(0x980, 4, '', 'data', 'value', 'unit', 'src', ())], [])
        return model, raw, candidate, image, data

    def proof(self, model, records, candidate, image):
        return infer(model, identities.project_records(records),
                     {'unit': identities.CandidateNames(candidate, 'unit')}, image)

    def test_reviewed_graph_and_exact_body_prove_identity_without_raw_byte_credit(self):
        model, records, candidate, image, _ = self.fixture()
        before = list(records)
        with patch('homm3.compare.canonicalize._load_anon_ns_canonical', return_value=self.policy):
            proved = self.proof(model, records, candidate, image)
            projected = identities.project_records(records)
        self.assertEqual(proved, [(self.records(self.retail_scope)[-1].relocations[0][1],
                                  0x820, 'unit', projected[-1].name, 0x250)])
        self.assertEqual(records, before)
        self.assertNotEqual(records[0].payload, projected[0].payload)
        self.assertNotEqual(len(records[0].payload), len(projected[0].payload))
        known = {b.name: {b.rva} for b in model.data + model.functions}
        known[records[-1].relocations[0][1]] = {0x820}
        self.assertFalse(exception_data.resolve(records, image, known)[0])

    def test_unreviewed_or_wrong_scope_does_not_prove_identity(self):
        for policy in ({}, {('other', 'unit.cpp'): r'C:\Game\unit.cpp456'},
                       {('unit', 'other.cpp'): r'C:\Game\unit.cpp456'}):
            model, records, candidate, image, _ = self.fixture()
            with self.subTest(policy=policy), patch(
                    'homm3.compare.canonicalize._load_anon_ns_canonical', return_value=policy):
                self.assertFalse(self.proof(model, records, candidate, image))

    def test_typename_scalar_pointer_instruction_and_named_target_are_not_masked(self):
        for offset in (0x10c, 0x214, 0x254, 0x820, 0x821):
            model, records, candidate, image, data = self.fixture()
            data[offset] ^= 1
            with self.subTest(offset=offset), patch(
                    'homm3.compare.canonicalize._load_anon_ns_canonical', return_value=self.policy):
                self.assertFalse(self.proof(model, records, candidate, image))

    def test_candidate_name_collision_cannot_select_a_body(self):
        _, records, candidate, _, _ = self.fixture()
        name = records[-1].relocations[0][1]
        other = name.replace(self.raw_scope, self.retail_scope)
        candidate.symbols[other] = replace(candidate.body(name), name=other)
        with patch('homm3.compare.canonicalize._load_anon_ns_canonical', return_value=self.policy):
            view = identities.CandidateNames(candidate, 'unit')
            self.assertIsNone(view.body(other))

    def test_duplicate_retail_descriptor_cannot_anchor_graph(self):
        model, records, candidate, image, data = self.fixture()
        size = len(self.records(self.retail_scope)[0].payload)
        data[0x300:0x300+size] = data[0x100:0x100+size]
        sites = set(image.reloc_sites) | {0x300}
        image.reloc_sites = sorted(sites)
        image.relocs_in = lambda a, b: sorted(r for r in sites if a <= r < b)
        with patch('homm3.compare.canonicalize._load_anon_ns_canonical', return_value=self.policy):
            self.assertFalse(self.proof(model, records, candidate, image))

    def test_published_graph_rows_are_scoped_identities_and_require_fresh_witnesses(self):
        model, records, candidate, image, _ = self.fixture()
        with TemporaryDirectory() as tmp, patch(
                'homm3.compare.canonicalize._load_anon_ns_canonical', return_value=self.policy):
            root = Path(tmp)
            (root/'tools/bin').mkdir(parents=True)
            project = SimpleNamespace(toolchain=root/'tools')
            alias, rva, *_ = self.proof(model, records, candidate, image)[0]
            model.functions[0] = model.functions[0]._replace(aliases=(
                SimpleNamespace(name=alias, channel='source-folded-exact'),))
            with patch.object(exception_data, 'candidates', return_value=(records, [])), \
                 patch('homm3.delink.image.retail', return_value=image), \
                 patch.object(identities, 'witnesses_current', return_value=True) as fresh:
                rows = identities.address_identities(model, project, root, {})
                self.assertEqual({row[0] for row in rows}, {0x100, 0x200, 0x240, 0x250})
                self.assertTrue(all(row[2] == 'exception-identity' and row[4] == 'unit'
                                    for row in rows))
                fresh.return_value = False
                self.assertEqual(identities.address_identities(model, project, root, {}), [])

    def test_malformed_raw_descriptor_cannot_be_repaired_by_projection(self):
        record = self.records(self.raw_scope)[0]
        record = replace(record, payload=record.payload[:-2]+b'X\0')
        with patch('homm3.compare.canonicalize._load_anon_ns_canonical', return_value=self.policy):
            self.assertEqual(identities.project_records([record]), [])

    def test_policy_change_invalidates_witnesses(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, obj, policy = (root/name for name in ('unit.cpp', 'unit.obj', 'paths.tsv'))
            source.write_bytes(b'class Foo {};')
            obj.write_bytes(b'raw COFF')
            policy.write_bytes(b'original scope')
            witnesses = {'unit': (SimpleNamespace(buf=obj.read_bytes()),
                                  {str(source): sha256(source.read_bytes()).hexdigest()})}
            with patch('homm3.delink.exception_identities.anon_ns_stamp_inputs',
                       return_value={'anon_ns_paths': policy}):
                snapshot = identities.policy_snapshot()
                self.assertTrue(identities.witnesses_current(witnesses, root, snapshot))
                policy.write_bytes(b'different scope')
                self.assertFalse(identities.witnesses_current(witnesses, root, snapshot))


if __name__ == '__main__':
    unittest.main()
