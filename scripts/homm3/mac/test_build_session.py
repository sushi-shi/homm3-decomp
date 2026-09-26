"""Shared build work cannot turn stale inputs or unknown calls into verdicts."""
from contextlib import ExitStack
from pathlib import Path
from types import SimpleNamespace
import os
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import build, call_report, queue
from homm3.mac.build_session import BuildSession
from homm3.mac import test_profiles


class BuildSessionTests(unittest.TestCase):
    def fixture(self, stack):
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        pairs = test_profiles.TestMacProfiles().fixture(root)
        stack.enter_context(patch.object(build, 'ROOT', root))
        stack.enter_context(patch.object(build, '_wine_version', return_value='control'))
        stack.enter_context(patch.object(build.toolchain, 'specification', return_value={
            'flags': ['-O1', '-nolink'], 'files': {}, 'collapse_reloads': True}))
        context = call_report.InspectionContext('analysis', {}, {})
        contexts = stack.enter_context(patch.object(call_report, 'inspection_context', return_value=context))
        source = stack.enter_context(patch.object(build, 'candidate_source', return_value='one shared unit'))
        parses = stack.enter_context(patch.object(build, 'parse_code_hunks', wraps=build.parse_code_hunks))
        commands = []

        def run(command, cwd, env):
            commands.append(command)
            if command[1].endswith('MWCPPC.exe'):
                (cwd / 'candidate.o').write_bytes(b'MWOBPPC fixture')
                return ''
            return '\n'.join(
                f'Hunk: Kind=HUNK_GLOBAL_CODE Align=4 Class=PR Name=".{name}"(1) Size=4\n'
                '00000000: 4E800020 blr' for name in ('first', 'second'))

        stack.enter_context(patch.object(build, '_run', side_effect=run))
        pef = SimpleNamespace(code=lambda *_: bytes.fromhex('4e800020'))
        return root, pairs, pef, contexts, source, parses, commands

    def test_inspection_and_comparison_reuse_one_unit_and_context(self):
        with ExitStack() as stack:
            root, pairs, pef, contexts, source, parses, commands = self.fixture(stack)
            session = BuildSession(root, pef, root / 'tools')
            for pair in pairs:
                observation = call_report.inspect(root, pair, pef, root / 'tools',
                                                   context=session.context, session=session)
                result = build.compare_pair(pair, pef, root / 'tools', session=session)
                self.assertEqual(observation['object_sha256'], result.object_sha256)
                self.assertTrue(result.exact)
            session.verify()
            self.assertEqual(contexts.call_count, 1)
            self.assertEqual(source.call_count, 1)
            self.assertEqual(parses.call_count, 1)
            self.assertEqual(len(commands), 2)  # one compiler, one disassembler
            self.assertEqual(len(session.compiled), 2)

    def test_fresh_session_invalidates_same_size_source_and_header_edits(self):
        with ExitStack() as stack:
            root, pairs, pef, _, source, _, commands = self.fixture(stack)
            first = BuildSession(root, pef, root / 'tools')
            before = first.compile(pairs[0])
            path = root / 'include/value.h'
            st = path.stat()
            path.write_text('#define VALUE 2\n')
            os.utime(path, ns=(st.st_atime_ns, st.st_mtime_ns))
            with self.assertRaisesRegex(build.MacBuildError, 'inputs changed'):
                first.verify()
            source.return_value = 'changed shared unit'
            second = BuildSession(root, pef, root / 'tools')
            after = second.compile(pairs[0])
            second.verify()
            self.assertNotEqual(before.build_hash, after.build_hash)
            self.assertNotEqual(before.source_hash, after.source_hash)
            self.assertEqual(len(commands), 4)

    def test_input_additions_removals_and_reference_edits_block_publication(self):
        with ExitStack() as stack:
            root, _, pef, *_ = self.fixture(stack)
            for relative in ('src/new.cpp', 'config/mac/aliases.tsv', 'build/mac/sdk/new.h',
                             'build/mac/toolchain/MWCPPC.exe', 'include/new.h'):
                with self.subTest(relative=relative):
                    session = BuildSession(root, pef, root / 'tools')
                    path = root / relative
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text('new input')
                    with self.assertRaises(build.MacBuildError):
                        session.verify()
                    another = BuildSession(root, pef, root / 'tools')
                    path.unlink()
                    with self.assertRaises(build.MacBuildError):
                        another.verify()

    def test_artifact_tampering_blocks_publication(self):
        with ExitStack() as stack:
            root, pairs, pef, *_ = self.fixture(stack)
            session = BuildSession(root, pef, root / 'tools')
            session.compile(pairs[0])
            (build.object_directory(root, pairs[0]) / 'candidate.o').write_bytes(b'MWOBPPC changed')
            with self.assertRaisesRegex(build.MacBuildError, 'artifacts changed'):
                session.verify()

    def test_failed_unit_is_attempted_once_and_does_not_hide_other_units(self):
        with ExitStack() as stack:
            root, pairs, pef, *_ = self.fixture(stack)
            session = BuildSession(root, pef, root / 'tools')
            with patch.object(build, '_compile_locked', side_effect=build.MacBuildError('compiler failed')) as compile:
                for pair in pairs:
                    row = call_report.inspect(root, pair, pef, root / 'tools',
                                              context=session.context, session=session)
                    self.assertIsNone(row['calls']['candidate'])
                self.assertEqual(compile.call_count, 1)
            self.assertEqual(len(session.failures), 1)
            session.verify()

    def test_failed_compile_keeps_current_provenance_for_queue_routing(self):
        with ExitStack() as stack:
            root, pairs, pef, *_ = self.fixture(stack)
            session = BuildSession(root, pef, root / 'tools')
            with patch.object(build, '_run', side_effect=build.MacBuildError('compiler failed')) as compiler:
                rows = [call_report.inspect(root, pair, pef, root / 'tools',
                                           context=session.context, session=session) for pair in pairs]
            self.assertEqual(compiler.call_count, 1)
            for pair, row in zip(pairs, rows):
                self.assertTrue(row['source_hash'])
                self.assertTrue(row['build_hash'])
                self.assertIsNone(row['calls']['candidate'])
                self.assertIsNone(queue.observation_problem(root, pair, row, {}, session=session))
            session.verify()

    def test_changed_input_prevents_report_publication(self):
        with ExitStack() as stack:
            root, pairs, pef, *_ = self.fixture(stack)
            stack.enter_context(patch.object(build.inputs, 'stage_executable', return_value=root / 'game.pef'))
            stack.enter_context(patch.object(build.inputs, 'read_verified', return_value=b'fixture'))
            stack.enter_context(patch.object(build.toolchain, 'stage', return_value=root / 'tools'))
            stack.enter_context(patch.object(build, 'PEF', return_value=pef))
            publish = stack.enter_context(patch.object(build.reports, 'publish'))
            compare = build.compare_pair

            def edit_after_compare(*args, **kwargs):
                result = compare(*args, **kwargs)
                (root / 'include/value.h').write_text('#define VALUE 2\n')
                return result

            stack.enter_context(patch.object(build, 'compare_pair', side_effect=edit_after_compare))
            with self.assertRaisesRegex(build.MacBuildError, 'inputs changed'):
                build.run(checkpoint=True)
            publish.assert_not_called()

    def test_reference_map_and_labels_share_one_validated_inventory(self):
        with patch.object(call_report, 'load_pairs', return_value=[]) as pairs, \
             patch.object(call_report.references, 'load', return_value=[]) as refs, \
             patch.object(call_report.symbols, 'targets', return_value={}) as targets, \
             patch.object(call_report, 'analysis_hash', return_value='hash'):
            context = call_report.inspection_context(Path('.'), object())
            self.assertEqual(context.analysis, 'hash')
            self.assertEqual((pairs.call_count, refs.call_count, targets.call_count), (1, 1, 1))
            self.assertEqual(targets.call_args.kwargs, {'pairs': [], 'refs': []})
