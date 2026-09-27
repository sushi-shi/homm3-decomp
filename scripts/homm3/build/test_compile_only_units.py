"""Compile-only units must stay built without hiding independent retail bodies."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.build import configure


class CompileOnlyUnitsTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.enterContext(patch.object(configure, 'ROOT', self.root))
        self.unit = {'unit': 'hook', 'source': 'src/hook.cpp',
                     'flags': 'game', 'compare': False}
        (self.root / 'src').mkdir()
        (self.root / 'src/hook.cpp').write_text('void hook() {}\n')

    def test_compile_only_stays_in_build_and_link_but_not_comparison(self):
        configure.write_ninja({'game': ['/c']}, [self.unit])
        graph = (self.root / 'build.ninja').read_text()
        self.assertIn('build build/objdiff/base/hook.obj:', graph)
        self.assertIn('build objects: phony build/objdiff/base/hook.obj', graph)
        self.assertIn('build build/exe/HEROES3.candidate.EXE: link build/objdiff/base/hook.obj', graph)
        configure.write_objdiff({}, [self.unit, {'unit': 'ordinary'}])
        rows = json.loads((self.root / 'build/objdiff/objdiff.json').read_text())['units']
        self.assertEqual([row['name'] for row in rows], ['ordinary'])
        self.assertEqual(rows[0]['target_path'], './dummy.obj')

    def test_cannot_hide_existing_retail_target(self):
        for side in ('target', 'normalized/target'):
            with self.subTest(side=side):
                target = self.root / 'build/objdiff' / side / 'hook.c.obj'
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(b'retail')
                with self.assertRaisesRegex(SystemExit, 'compile-only unit has a retail target'):
                    configure.write_objdiff({}, [self.unit])
                target.unlink()

    def test_compare_field_requires_boolean(self):
        for value in ('false', 0, None):
            with self.subTest(value=value), patch.object(configure.units_manifest, 'load',
                return_value={'flags': {'game': ['/c']}, 'unit': [dict(self.unit, compare=value)]}):
                with self.assertRaisesRegex(SystemExit, 'compare must be a boolean'):
                    configure.load_manifest()


if __name__ == '__main__':
    unittest.main()
