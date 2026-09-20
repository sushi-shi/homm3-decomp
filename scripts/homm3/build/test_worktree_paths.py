"""Entry-point worktree selection and Windows response-file paths."""
import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from homm3.build import link


class WorktreePathsTest(unittest.TestCase):
    def test_shell_root_resolves_subdirectories_without_creating_build_artifacts(self):
        resolver = Path(__file__).resolve().parents[2] / 'project-root.sh'
        with tempfile.TemporaryDirectory(prefix='homm3 shell root ') as raw:
            root = Path(raw)
            (root / 'src/nested').mkdir(parents=True)
            (root / 'scripts/homm3').mkdir(parents=True)
            (root / 'config').mkdir()
            for name in ('flake.nix', 'config/project.toml', 'config/units.toml'):
                (root / name).touch()
            # A linked worktree has a .git file, not a .git directory.
            (root / '.git').write_text('gitdir: /unused/test/worktree\n')
            for start in (root, root / 'src', root / 'src/nested'):
                result = subprocess.run(['sh', str(resolver), str(start)],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), str(root))
                self.assertFalse((start / 'build').exists())
            # A stale environment variable cannot override the supplied path.
            result = subprocess.run(['sh', str(resolver), str(root / 'src')],
                                    env=dict(os.environ, HOMM3_DIR='/wrong/checkout'),
                                    capture_output=True, text=True)
            self.assertEqual(result.stdout.strip(), str(root))

    def test_shell_root_fails_without_project_markers(self):
        resolver = Path(__file__).resolve().parents[2] / 'project-root.sh'
        with tempfile.TemporaryDirectory() as raw:
            result = subprocess.run(['sh', str(resolver), raw], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, '')
            self.assertIn('no project root', result.stderr)
            self.assertEqual(list(Path(raw).iterdir()), [])

    def test_configure_and_compiler_honor_requested_worktree(self):
        scripts = Path(__file__).resolve().parents[2]
        with tempfile.TemporaryDirectory(prefix="homm3 root ") as raw:
            root = Path(raw)
            (root / "config").mkdir()
            (root / "config/units.toml").write_text("[build]\n[flags]\n")
            project_config = Path(__file__).resolve().parents[3] / "config/project.toml"
            (root / "config/project.toml").write_bytes(project_config.read_bytes())
            env = dict(os.environ, HOMM3_DIR=raw, PYTHONPATH=str(scripts))
            result = subprocess.run([sys.executable, "-m", "homm3", "configure"],
                                    env=env, cwd=scripts, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("0 VC6 units", result.stdout)
            self.assertTrue((root / "build.ninja").is_file())
            self.assertTrue((root / "build/objdiff/objdiff.json").is_file())
            self.assertEqual(json.loads((root / "compile_commands.json").read_text()), [])
            result = subprocess.run(
                [sys.executable, "-c", "from homm3.core.cc_wrap import HOMM3_DIR; print(HOMM3_DIR)"],
                env=env, cwd=scripts, capture_output=True, text=True, check=True)
            self.assertEqual(result.stdout.strip(), raw)

    def test_link_quotes_output_map_objects_and_libraries(self):
        with tempfile.TemporaryDirectory(prefix="homm3 link ") as raw:
            root = Path(raw)
            out, mapfile, obj, lib = [root / name for name in
                                    ("output file.exe", "output file.map", "input file.obj", "import file.lib")]
            obj.touch()
            lib.touch()
            def run_wine(cmd, cwd, produced):
                produced.touch()
                return "", 0
            def windows(path):
                return "Z:" + str(path).replace("/", "\\")
            with patch.object(link.shutil, "which", return_value="wine"), \
                    patch.object(link, "msvc_dir", return_value=root), \
                    patch.object(link, "find_ci", return_value=root / "link.exe"), \
                    patch.object(link, "ensure_wineserver"), \
                    patch.object(link, "winepath_w", side_effect=windows), \
                    patch.object(link, "run_wine", side_effect=run_wine), \
                    patch.dict(os.environ, {"WINEPREFIX": raw}), \
                    contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(link.main(["--out", str(out), "--map", str(mapfile),
                                            "--obj", str(obj), "--lib", str(lib)]), 0)
            lines = (root / "output file.objs.rsp").read_text().splitlines()
            self.assertIn(f'/OUT:"{windows(out)}"', lines)
            self.assertIn(f'/MAP:"{windows(mapfile)}"', lines)
            for path in (obj, lib):
                self.assertIn(f'"{windows(path)}"', lines)
