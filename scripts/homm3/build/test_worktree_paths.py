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
from homm3.core import inputs


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

    def test_shell_environment_drops_foreign_project_state(self):
        script = Path(__file__).resolve().parents[2] / 'project-env.sh'
        environ = {k: v for k, v in os.environ.items()
                   if not k.startswith(('HOMM', 'MSVC_DIR', 'PYTHONPATH'))}
        environ.update(HOMM3_DIR='/lane', HOMM3_MSVC5_DIR='/vc5', HOMM1_DIR='/homm1',
                       HOMM2_DIR='/homm2', MSVC_DIR='/homm1/build/toolchains/vc40',
                       PYTHONPATH='/homm1/scripts')
        result = subprocess.run(['sh', '-c', f'. "{script}" && env'], env=environ,
                                capture_output=True, text=True, check=True)
        values = dict(line.split('=', 1) for line in result.stdout.splitlines() if '=' in line)
        self.assertNotIn('HOMM1_DIR', values)
        self.assertNotIn('HOMM2_DIR', values)
        self.assertEqual(values['HOMM3_MSVC5_DIR'], '/vc5')
        self.assertEqual(values['HOMM3_TOOLCHAIN'], '/lane/build/homm3-toolchain-vc6-sp3')
        self.assertEqual(values['MSVC_DIR'], '/lane/build/homm3-toolchain-vc6-sp3/msvc')
        self.assertEqual(values['PYTHONPATH'], '/lane/scripts')

    @staticmethod
    def _fake_checkout(root: Path) -> Path:
        (root / 'src').mkdir(parents=True)
        (root / 'scripts/homm3').mkdir(parents=True)
        (root / 'config').mkdir()
        for name in ('flake.nix', 'config/project.toml', 'config/units.toml'):
            (root / name).touch()
        return root.resolve()

    def test_wrapper_prefers_current_checkout_over_inherited_homm3_dir(self):
        resolver = Path(__file__).resolve().parents[2] / 'project-root.sh'
        with tempfile.TemporaryDirectory(prefix='homm3 select ') as raw:
            main = self._fake_checkout(Path(raw) / 'main')
            lane = self._fake_checkout(Path(raw) / 'lane')
            outside = Path(raw) / 'outside'
            outside.mkdir()

            def select(cwd, **env):
                environ = {k: v for k, v in os.environ.items()
                           if k not in ('HOMM3_DIR', 'HOMM3_DIR_FORCE')}
                environ.update(env)
                return subprocess.run(['sh', str(resolver), '--select'], cwd=cwd,
                                      env=environ, capture_output=True, text=True)

            result = select(lane / 'src', HOMM3_DIR=str(main))
            self.assertEqual((result.returncode, result.stdout.strip()), (0, str(lane)))
            self.assertIn('HOMM3_DIR_FORCE=1', result.stderr)
            result = select(lane / 'src', HOMM3_DIR=str(main), HOMM3_DIR_FORCE='1')
            self.assertEqual((result.stdout.strip(), result.stderr), (str(main), ''))
            result = select(lane, HOMM3_DIR=str(lane / 'src'))
            self.assertEqual((result.stdout.strip(), result.stderr), (str(lane), ''))
            result = select(lane / 'src')
            self.assertEqual((result.stdout.strip(), result.stderr), (str(lane), ''))
            result = select(outside, HOMM3_DIR=str(main / 'src'))
            self.assertEqual((result.stdout.strip(), result.stderr), (str(main), ''))
            result = select(outside)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('no project root', result.stderr)

    def test_python_root_policy_matches_wrapper(self):
        from homm3.core import root
        with tempfile.TemporaryDirectory(prefix='homm3 select ') as raw:
            main = self._fake_checkout(Path(raw) / 'main')
            lane = self._fake_checkout(Path(raw) / 'lane')
            fixture = Path(raw) / 'fixture'
            fixture.mkdir()
            fallback = Path(raw) / 'code'
            self.assertEqual(root.project_root(lane / 'src'), lane)
            selected, warning = root.select({'HOMM3_DIR': str(main)}, lane / 'src', fallback)
            self.assertEqual(selected, lane)
            self.assertIn(str(lane), warning)
            self.assertEqual(root.select({'HOMM3_DIR': str(main), 'HOMM3_DIR_FORCE': '1'},
                                         lane, fallback), (main, None))
            self.assertEqual(root.select({'HOMM3_DIR': str(lane)}, lane / 'src', fallback),
                             (lane, None))
            self.assertEqual(root.select({}, lane / 'src', fallback), (lane, None))
            self.assertEqual(root.select({}, fixture, fallback), (fallback, None))
            self.assertEqual(root.select({'HOMM3_DIR': str(main)}, fixture, fallback),
                             (main, None))
            # Test fixtures that are not checkouts remain explicit roots.
            self.assertEqual(root.select({'HOMM3_DIR': str(fixture)}, lane, fallback),
                             (fixture, None))
            selected, warning = root.select({'HOMM3_DIR': str(Path(raw) / 'gone')}, lane, fallback)
            self.assertEqual(selected, lane)
            self.assertIn('does not exist', warning)

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

    # link.main reads the retail image for its base and entry point.
    @unittest.skipUnless(inputs.is_staged(inputs.RETAIL), inputs.requires_staged(inputs.RETAIL))
    def test_link_quotes_output_map_objects_and_libraries(self):
        with tempfile.TemporaryDirectory(prefix="homm3 link ") as raw:
            root = Path(raw)
            out, mapfile, obj, lib = [root / name for name in
                                    ("output file.exe", "output file.map", "input file.obj", "import file.lib")]
            obj.touch()
            lib.touch()
            def run_wine(cmd, cwd):
                # link.main checks the output itself; the mock links it.
                out.touch()
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
