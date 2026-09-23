"""Raw objects need content proof, not timestamp freshness or a nearby stamp."""
from pathlib import Path
import tempfile
import unittest

from homm3.build import compiled_freshness as freshness


class CompiledFreshnessTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.paths = ['src/a.cpp', 'src/private.inc', 'include/a.h', 'toolchain/bin/CL.EXE',
                      'toolchain/include/STDIO.H', 'config/units.toml', 'config/project.toml',
                      'scripts/homm3/core/cc_wrap.py', 'scripts/homm3/core/project.py',
                      'scripts/homm3/build/compiled_freshness.py']
        for name in self.paths:
            path = self.root/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('before')
        self.output = self.root/'a.obj'
        self.output.write_bytes(b'compiler-output')

    def snapshot(self, flags=('/O2',)):
        return freshness.snapshot(self.root, self.root/'src/a.cpp', flags,
                                  [self.root/'include'], self.root/'toolchain')

    def test_roundtrip_and_no_stamp_is_not_an_empty_success(self):
        expected = self.snapshot()
        with self.assertRaisesRegex(ValueError, 'missing/invalid'):
            freshness.validate(self.output, expected)
        freshness.write(self.output, expected, expected)
        self.assertEqual(freshness.validate(self.output, expected), freshness.digest(self.output))

    def test_source_header_profile_tool_and_flags_changes_invalidate(self):
        previous = self.snapshot()
        freshness.write(self.output, previous, previous)
        for name in self.paths:
            with self.subTest(path=name):
                (self.root/name).write_text('after!')  # same length does not imply fresh
                with self.assertRaisesRegex(ValueError, 'stale'):
                    freshness.validate(self.output, self.snapshot())
                (self.root/name).write_text('before')
        with self.assertRaisesRegex(ValueError, 'stale'):
            freshness.validate(self.output, self.snapshot(['/Od']))

    def test_new_or_removed_header_invalidates_and_corrupt_output_is_rejected(self):
        expected = self.snapshot()
        freshness.write(self.output, expected, expected)
        extra = self.root/'include/new.inc'
        extra.write_text('new')
        with self.assertRaisesRegex(ValueError, 'stale'):
            freshness.validate(self.output, self.snapshot())
        extra.unlink()
        self.output.write_bytes(b'different-output')
        with self.assertRaisesRegex(ValueError, 'bytes differ'):
            freshness.validate(self.output, expected)

    def test_mid_compile_input_change_cannot_get_a_valid_stamp(self):
        before = self.snapshot()
        (self.root/'src/a.cpp').write_text('changed')
        with self.assertRaisesRegex(ValueError, 'during compilation'):
            freshness.write(self.output, before, self.snapshot())
        self.assertFalse(freshness.stamp_path(self.output).exists())
