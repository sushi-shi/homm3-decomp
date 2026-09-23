"""Exercise real objdiff: changed bytes and independently linked pointers fail."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from homm3.build import data_objdiff
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build.normalized_freshness import write_stamp
from homm3.sema import test_data_match as fixtures


class DataObjdiffTest(unittest.TestCase):
    def setUp(self):
        self.fixture = fixtures.DataMatchTest()
        self.fixture.setUp()

    def project(self, obj, bindings=None, **kwargs):
        report = self.fixture.compare(obj, bindings, **kwargs)
        return data_objdiff.project(self.fixture.layout, report, {'a': obj})

    def score(self, pair):
        if not shutil.which('objdiff-cli'):
            self.skipTest('objdiff-cli is required for integration controls')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            for side, rows in zip(('base', 'target'), pair):
                (path/(side+'.obj')).write_bytes(data_objdiff.coff(rows))
            (path/'objdiff.json').write_text(json.dumps(dict(units=[dict(
                name='data', base_path='base.obj', target_path='target.obj') ])))
            result = subprocess.run(['objdiff-cli', 'report', 'generate', '-p', directory,
                                     '-o', str(path/'report.json')], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            report = json.loads((path/'report.json').read_text())
            self.assertFalse(report['units'][0].get('functions'))
            return report['units'][0]['sections'][0]['fuzzy_match_percent']

    def test_objdiff_retains_changed_initializer_and_exact_allocation_sizes(self):
        self.fixture.put(0x2000, b'abcdefgh')
        for raw, score in [(b'abcdefgh', 100), (b'abc!efgh', 87.5)]:
            pairs, ledger = self.project(fixtures.coff(raw=raw))
            self.assertEqual(self.score(pairs['a']), score)
            obj = CoffObject(data_objdiff.coff(pairs['a'][0]))
            self.assertEqual(obj.sections[0].raw_size, 8)
            self.assertFalse(obj.relocations)
            self.assertEqual(ledger[0]['status'], 'compared')

    def test_objdiff_wrong_referent_and_addend_are_not_masked(self):
        for options, exact in [({}, True), ({'addend': 1}, False), ({'target_index': 2}, False)]:
            obj, bindings = self.fixture.pointers(**options)
            pairs, ledger = self.project(obj, bindings)
            self.assertEqual(self.score(pairs['a']) == 100, exact)
            # Retail has gaps between all three allocations. Neither those gaps
            # nor candidate linker padding become comparison bytes.
            self.assertEqual(CoffObject(data_objdiff.coff(pairs['a'][0])).sections[0].raw_size, 12)

    def test_unknown_pointer_is_withheld_rather_than_filled_from_retail(self):
        obj, bindings = self.fixture.pointers()
        pairs, ledger = self.project(obj, bindings[:1])
        self.assertFalse(pairs)
        self.assertEqual(ledger[0]['status'], 'withheld')
        self.assertEqual(ledger[0]['reasons'], ['pointer-unresolved'])

    def test_conflicting_bindings_and_missing_relocation_are_withheld(self):
        obj, bindings = self.fixture.pointers()
        bindings[1]['rva'] = 0x2002
        pairs, ledger = self.project(obj, bindings)
        self.assertTrue(all(r['status'] == 'withheld' for r in ledger[:2]))
        self.assertEqual(len(pairs['a'][0]), 1)
        raw = (0x402010).to_bytes(4, 'little') + bytes(4)
        self.fixture.put(0x2000, raw)
        pairs, ledger = self.project(fixtures.coff(raw=raw), retail_pointers=[0x2000])
        self.assertFalse(pairs)
        self.assertEqual(ledger[0]['reasons'], ['missing-relocation'])

    def test_changed_config_or_projection_cannot_produce_a_fresh_report(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            obj = path/'data.obj'
            original = data_objdiff.coff([('data', b'abcd')])
            obj.write_bytes(original)
            write_stamp(obj, {})
            config = path/'objdiff.json'
            config.write_text(json.dumps(dict(units=[dict(
                name='data', base_path='data.obj', target_path='data.obj')])))
            write_stamp(config, {'projection': obj})
            with patch('homm3.build.report.generate') as run:
                data_objdiff.generate_report(path)
                run.assert_called_once_with(path)
                obj.write_bytes(data_objdiff.coff([('data', b'abce')]))
                with self.assertRaisesRegex(ValueError, 'stale objdiff data'):
                    data_objdiff.generate_report(path)
                obj.write_bytes(original)
                config.write_text(json.dumps(dict(units=[])))
                with self.assertRaisesRegex(ValueError, 'stale objdiff data configuration'):
                    data_objdiff.generate_report(path)
                run.assert_called_once()


if __name__ == '__main__':
    unittest.main()
