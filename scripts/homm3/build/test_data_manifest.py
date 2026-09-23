"""Bind actual candidate sections without claiming retail topology/equality."""
from pathlib import Path
import tempfile
import unittest

from homm3.build import data_manifest as manifest
from homm3.build.canonicalize_data_symbols import load_compgen_data_claims
from homm3.core import tsv


class BindingManifestTest(unittest.TestCase):
    def sample(self, macro='DATA_COMPGEN'):
        candidate = dict(id='a:4:0', unit='a', symbols=['$SG1'], section_ordinal=4,
                         section_offset=0, alignment=4, scopes=['local'], storage='rdata')
        binding = dict(id=0, status='bound', candidate_ids=['a:4:0'], macro=macro,
                       rva=0x1000, size=4, source='src/a.cpp:4')
        return dict(candidate_data=[candidate], data_bindings=[binding])

    def test_shared_literal_uses_model_identity_and_coalesces_repeated_sites(self):
        report = self.sample()
        report['data_bindings'].append(dict(report['data_bindings'][0], id=1, source='src/a.cpp:8'))
        rows, issues = manifest.binding_rows(report, {0x1000: '__h3cg$first$data$message'})
        self.assertFalse(issues)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]['name'], '__h3cg$first$data$message')
        self.assertEqual(rows[0]['section_ordinal'], '4')

    def test_ambiguous_bindings_and_unknown_topology_cannot_enter_canonicalizer(self):
        report = self.sample()
        self.assertEqual(manifest.binding_rows(report, {})[0], [])
        self.assertEqual(len(manifest.binding_rows(report, {})[1]), 1)
        report['data_bindings'][0]['status'] = 'ambiguous-pool'
        self.assertEqual(manifest.binding_rows(report, {0x1000: 'name'}), ([], []))
        report['data_bindings'][0]['status'] = 'bound'
        report['candidate_data'][0]['section_ordinal'] = 0
        self.assertFalse(manifest.binding_rows(report, {0x1000: 'name'})[0])

    def test_guard_uses_one_byte_even_when_physical_alignment_is_four(self):
        report = self.sample('DATA_COMPGEN_GUARD')
        report['data_bindings'][0]['size'] = 1
        rows, _ = manifest.binding_rows(report, {0x1000: '__h3cg$a$static_init_guard$table'})
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'bindings.tsv'
            tsv.write(path, [], manifest.BINDINGS_HEADER.split('\t'), rows)
            claims = load_compgen_data_claims(path, 'a')
            self.assertEqual(len(claims), 1)
            self.assertEqual((claims[0].size, claims[0].section), (1, 4))


if __name__ == '__main__':
    unittest.main()
