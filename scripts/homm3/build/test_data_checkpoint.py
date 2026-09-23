"""Content-stale objects must be rebuilt even when Ninja's timestamps agree."""
import os
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.build import compiled_freshness, data_checkpoint, data_objdiff
from homm3.build import test_compiled_freshness as fixtures


class DataCheckpointTest(unittest.TestCase):
    def setUp(self):
        fixtures.CompiledFreshnessTest.setUp(self)
        (self.root/'src/b.cpp').write_text('before')
        self.project = SimpleNamespace(root=self.root, includes=[self.root/'include'],
            toolchain=self.root/'toolchain', manifest=dict(flags={'p': ['/O2']}, unit=[
                dict(unit=u, source=f'src/{u}.cpp', flags='p') for u in ('a', 'b')]))
        self.enterContext(patch.object(data_checkpoint, 'Project', return_value=self.project))
        self.outputs = {}
        for unit in ('a', 'b'):
            output = self.root/f'build/objdiff/base/{unit}.obj'
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(b'raw compiled bytes')
            expected = compiled_freshness.snapshot(self.root, self.root/f'src/{unit}.cpp', ['/O2'],
                [self.project.toolchain/'include', *self.project.includes], self.project.toolchain)
            compiled_freshness.write(output, expected, expected)
            self.outputs[unit] = output

    def test_fresh_input_does_not_trigger_recompilation(self):
        self.assertEqual(data_checkpoint.schedule(self.root), [])

    def test_preserved_timestamp_edit_invalidates_stamps_without_deleting_raw_objects(self):
        path = self.root/'src/b.cpp'
        before = path.stat()
        path.write_text('after!')
        os.utime(path, ns=(before.st_atime_ns, before.st_mtime_ns))
        self.assertEqual(data_checkpoint.schedule(self.root, targets=['a']), ['a'])
        self.assertTrue(compiled_freshness.stamp_path(self.outputs['b']).exists())
        self.assertEqual(data_checkpoint.schedule(self.root), ['a', 'b'])
        for output in self.outputs.values():
            self.assertEqual(output.read_bytes(), b'raw compiled bytes')
            self.assertFalse(compiled_freshness.stamp_path(output).exists())

    def test_corrupted_object_is_scheduled_even_with_unchanged_inputs(self):
        self.outputs['b'].write_bytes(b'corrupted')
        self.assertEqual(data_checkpoint.schedule(self.root), ['b'])

    def test_objdiff_revalidates_source_trees_even_if_saved_objects_are_unchanged(self):
        stamps = [compiled_freshness.stamp_path(p) for p in self.outputs.values()]
        data_objdiff.validate_compiler_inputs(stamps)
        (self.root/'src/new.inc').write_text('new compile input')
        with self.assertRaisesRegex(ValueError, 'compiler input tree changed'):
            data_objdiff.validate_compiler_inputs(stamps)


if __name__ == '__main__':
    unittest.main()
