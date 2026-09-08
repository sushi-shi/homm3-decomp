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
    def test_configure_and_compiler_honor_requested_worktree(self):
        scripts = Path(__file__).resolve().parents[2]
        with tempfile.TemporaryDirectory(prefix="homm3 root ") as raw:
            root = Path(raw)
            (root / "config").mkdir()
            (root / "config/units.toml").write_text("[flags]\n")
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
