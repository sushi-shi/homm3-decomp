"""Negative controls for the scoped static checks, using both real tools."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

from homm3.core import lint


@unittest.skipUnless(shutil.which('ruff') and shutil.which('pyright'),
                     'Ruff and Pyright are required')
class LintTest(unittest.TestCase):
    def test_each_checker_can_fail_the_command(self):
        scripts = Path(__file__).resolve().parents[2]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config").mkdir()
            shutil.copyfile(scripts.parent / "config/project.toml",
                            root / "config/project.toml")
            for name in (*lint.MODULES, *lint.TESTS):
                path = root / 'scripts/homm3' / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('')
            probe = root / 'scripts/homm3' / lint.MODULES[0]
            for source, status, diagnostics in (
                ('value: int = 1\n', 0, ('All checks passed!', '0 errors')),
                ('import os\n', 1, ('F401', '0 errors')),
                ('value: int = "wrong"\n', 1, ('All checks passed!', 'reportAssignmentType')),
            ):
                with self.subTest(source=source):
                    probe.write_text(source)
                    result = subprocess.run(
                        [sys.executable, '-m', 'homm3.core.lint'], cwd=root,
                        env={**os.environ, 'HOMM3_DIR': str(root), 'PYTHONPATH': str(scripts)},
                        capture_output=True, text=True, timeout=60)
                    output = result.stdout + result.stderr
                    self.assertEqual(result.returncode, status, output)
                    for diagnostic in diagnostics:
                        self.assertIn(diagnostic, output)


if __name__ == '__main__':
    unittest.main()
