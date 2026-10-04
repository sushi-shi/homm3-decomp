"""Reference ownership, resume safety and comparison contracts."""
import copy
from pathlib import Path
import tempfile
import unittest

from .cases import validate_case
from .corpus import database, put, replay, retail_reference, verdict, writer


class CorpusTests(unittest.TestCase):
    def setUp(self):
        self.case = validate_case(dict(name='fixture', seed=123, stackWord=0xffffffff, heapByte=91))
        self.state = dict(returnCode=0, rngState=42, x87ControlWord=639,
                          x87FinalControlWord=639, request=self.case)
        self.report = dict(runs={'retail': dict(status='ok', state=self.state)}, comparison=dict(
            leftState=dict(self.state, rngState=99), rightState=self.state,
            map=dict(leftSize=4, rightSize=5, leftSha256='candidate', rightSha256='retail')))

    def test_import_uses_retained_native_side_even_when_cpp_differs(self):
        value = retail_reference(self.report)
        self.assertEqual((value['sha256'], value['size'], value['rng']), ('retail', 5, 42))
        self.report['comparison']['rightState'] = dict(self.state, rngState=77)
        with self.assertRaisesRegex(ValueError, 'lacks a map digest'):
            retail_reference(self.report)

    def test_native_crash_never_becomes_a_success(self):
        self.report['runs']['retail'] = dict(status='crash', address='0x1234', seconds=1)
        native = retail_reference(self.report)
        self.assertEqual(native, dict(status='crash', address='0x1234'))
        self.assertEqual(verdict(self.case, native, None, 'retail'), 'native-fault')

    def test_resume_is_idempotent_and_conflicts_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            with database(Path(tmp)) as db:
                value = retail_reference(self.report)
                put(db, 'retail', 0, value)
                put(db, 'retail', 0, copy.deepcopy(value))
                self.assertEqual(db.execute('SELECT count(*) FROM results').fetchone()[0], 1)
                with self.assertRaisesRegex(ValueError, 'conflicting'):
                    put(db, 'retail', 0, dict(value, rng=43))
                put(db, 'hotfix', 0, dict(value, rng=43))

    def test_second_writer_cannot_run(self):
        with tempfile.TemporaryDirectory() as tmp, writer(Path(tmp)):
            with self.assertRaises(BlockingIOError):
                with writer(Path(tmp)):
                    self.fail('second writer acquired the lock')

    def test_replay_records_mode_and_memory_inputs(self):
        self.assertIn('behavior retail 4294967295 91 1 -1 none seats', replay(self.case, 'retail'))
        self.assertIn('behavior hotfix seats', replay(self.case, 'hotfix'))
        self.assertNotEqual(replay(self.case, 'retail'), replay(dict(self.case, seed=124), 'retail'))
        with self.assertRaises(ValueError):
            replay(dict(self.case, heapByte=None), 'retail')

    def test_positive_and_mutation_controls(self):
        native = retail_reference(self.report)
        rust = dict(native, status='ok')
        self.assertEqual(verdict(self.case, native, rust, 'retail'), 'equal')
        for field in ('rng', 'size', 'request', 'sha256'):
            changed = dict(rust, **{field: 'different'})
            self.assertEqual(verdict(self.case, native, changed, 'retail'), 'mismatch')

    def test_rejection_requires_matching_state_and_specific_fault(self):
        native = dict(retail_reference(self.report), status='rejected', code=3)
        rust = dict(status='error', stage='Towns', rng=native['rng'], request=native['request'],
                    message='hotfix requires owned starting towns for all requested players')
        self.assertEqual(verdict(self.case, native, rust, 'hotfix'), 'matching-rejection')
        self.assertEqual(verdict(self.case, native, dict(rust, rng=43), 'hotfix'), 'mismatch')
        self.assertEqual(verdict(self.case, native, dict(rust, message='other'), 'hotfix'), 'mismatch')

    def test_only_documented_coast_fault_is_separate_from_mismatches(self):
        native = retail_reference(self.report)
        rust = dict(status='error', stage='Rivers', message='placement accesses missing cell '
                    'WorldPosition { point: Point { x: 36, y: 35 }, level: Surface }')
        self.assertEqual(verdict(self.case, native, rust, 'retail'), 'typed-coast-one-past')
        self.assertEqual(verdict(self.case, native, rust, 'hotfix'), 'mismatch')
        rust['message'] = rust['message'].replace('x: 36', 'x: 37')
        self.assertEqual(verdict(self.case, native, rust, 'retail'), 'mismatch')


if __name__ == '__main__':
    unittest.main()
