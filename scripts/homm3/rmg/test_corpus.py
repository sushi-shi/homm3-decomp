"""Reference ownership, resume safety, extension and comparison contracts."""
import argparse
import contextlib
import copy
import io
import json
from pathlib import Path
import random
import tempfile
import unittest

from .cases import load_cases, validate_case
from .corpus import (check_batch_protocol, database, digest, extend, extension_cases, packed,
                     put, replay, retail_reference, status, sync_requests, verdict, writer)


def rainbow(root, cases):
    """A minimal table directory: requests and manifest, no native binaries."""
    (root / 'cases.json').write_text(packed(cases) + '\n')
    info = dict(schema=1, cases=len(cases), cases_sha256=digest(root / 'cases.json'))
    (root / 'manifest.json').write_text(json.dumps(info, indent=2) + '\n')
    with database(root) as db:
        sync_requests(db, root, info)
    return info


def quietly(command, root, **options):
    with contextlib.redirect_stdout(io.StringIO()):
        return command(argparse.Namespace(out=root, **options))


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

    def test_harness_failures_are_refused_not_stored(self):
        with tempfile.TemporaryDirectory() as tmp:
            batch = Path(tmp)
            two = [dict(status='crash'), dict(status='not-run')]
            check_batch_protocol(batch, two)
            check_batch_protocol(batch, [dict(status='process-error', exitCode=3)])
            with self.assertRaisesRegex(ValueError, 'clean exit'):
                check_batch_protocol(batch, [dict(status='process-error', exitCode=0)])
            (batch / 'result.bin').write_bytes(b'')  # a single-job driver's output
            check_batch_protocol(batch, two[:1])
            with self.assertRaisesRegex(ValueError, 'ignored the batch'):
                check_batch_protocol(batch, two)

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


class ExtensionTests(unittest.TestCase):
    def test_extension_is_pinned_and_independent_of_chunking(self):
        first = extension_cases([], 1)[0]
        # Pins the sampler against generator or draw-order drift (sample2-extend-v1).
        self.assertEqual((first['name'], first['seed'], first['stackWord'], first['heapByte']),
                         ('sample2-000000', 4095670707, 2926988064, 93))
        whole = extension_cases([], 12)
        self.assertEqual(whole[:5], extension_cases([], 5))
        self.assertEqual(whole[5:], extension_cases(whole[:5], 7))
        self.assertEqual([c['name'] for c in whole], [f'sample2-{i:06d}' for i in range(12)])
        self.assertEqual([c['width'] for c in whole[:4]], [36, 72, 108, 144])
        self.assertEqual(extension_cases([], 3, master_seed=1)[0]['name'], 'sample2-000000')
        self.assertNotEqual(extension_cases([], 3, master_seed=1), whole[:3])

    def test_sampled_requests_are_valid_and_seeds_unique(self):
        cases = extension_cases([], 480)
        self.assertEqual(len({c['seed'] for c in cases}), 480)
        for case in cases:
            self.assertEqual(validate_case(case), case)
            self.assertEqual(sum(case['isHumanSeat']), case['humanPlayerCount'])
            fixed = sum(town >= 0 for town in case['townType'])
            self.assertLessEqual(fixed, case['humanPlayerCount'] + case['computerPlayerCount'])
        # Index-cycled settings cover every combination once per 480 cases.
        self.assertEqual(len({(c['width'], c['levels'], c['mapVersion'], c['waterContent'],
                               c['monsterStrength']) for c in cases}), 480)

    def test_seed_collision_is_redrawn(self):
        natural = extension_cases([], 2)
        base = [dict(natural[0], seed=natural[1]['seed'])]
        generator = random.Random((0x524d4733 << 32) | 1)
        self.assertEqual(generator.getrandbits(32), natural[1]['seed'])
        self.assertEqual(extension_cases(base, 1)[0]['seed'], generator.getrandbits(32))

    def test_sequence_must_be_sample2(self):
        with self.assertRaisesRegex(ValueError, 'breaks the sample2 sequence'):
            extension_cases([dict(extension_cases([], 1)[0], name='other')], 1)

    def test_extend_appends_without_touching_existing_entries(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            base = extension_cases([], 10)
            rainbow(root, base)
            old = (root / 'cases.json').read_text()
            with database(root) as db:
                put(db, 'retail', 3, dict(status='generated', rng=1))
                db.commit()
            quietly(extend, root, count=4, seed=0x524d4733)
            quietly(extend, root, count=2, seed=0x524d4733)
            cases = load_cases(root / 'cases.json')
            self.assertEqual(cases, extension_cases([], 16))
            self.assertEqual(cases[:10], base)
            self.assertTrue((root / 'cases.json').read_text().startswith(old[:-2] + ','))
            info = json.loads((root / 'manifest.json').read_text())
            self.assertEqual((info['cases'], info['cases_sha256']), (16, digest(root / 'cases.json')))
            self.assertEqual([(e['sampler'], e['first'], e['count']) for e in info['extensions']],
                             [('sample2-extend-v1', 10, 4), ('sample2-extend-v1', 14, 2)])
            with database(root) as db:
                rows = db.execute('SELECT ordinal,name,input_json FROM requests ORDER BY ordinal').fetchall()
                self.assertEqual(rows, [(i, c['name'], packed(c)) for i, c in enumerate(cases)])
                self.assertEqual(db.execute('SELECT ordinal FROM results').fetchall(), [(3,)])
            value = quietly(status, root)
            self.assertEqual((value['requests_per_mode'], value['stored_requests']), (16, 16))
            self.assertEqual(value['missing'], dict(retail=15, hotfix=16))
            self.assertEqual([(s['first'], s['count'], s['missing']) for s in value['segments']],
                             [(0, 10, dict(retail=9, hotfix=10)), (10, 4, dict(retail=4, hotfix=4)),
                              (14, 2, dict(retail=2, hotfix=2))])

    def test_interrupted_extension_is_completed_and_tampering_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rainbow(root, extension_cases([], 3))
            info = rainbow(root, extension_cases([], 5))  # files ahead of the table
            with database(root) as db:
                self.assertEqual(db.execute('SELECT count(*) FROM requests').fetchone()[0], 5)
                self.assertEqual(sync_requests(db, root, info), 0)
            (root / 'cases.json').write_text(packed(extension_cases([], 6)) + '\n')
            with self.assertRaisesRegex(ValueError, 'differs from manifest'):
                quietly(extend, root, count=1, seed=0x524d4733)


if __name__ == '__main__':
    unittest.main()
