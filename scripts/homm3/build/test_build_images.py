"""A full `homm3 build` never silently skips a pinned editor image."""
from __future__ import annotations

import contextlib
import io
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.build import build
from homm3.init import mfc_sp3

_PINS = """
[inputs.retail]
name = "retail executable"
path = "build/orig/HEROES3.EXE"
size = 1
sha256 = "0"
env = "HOMM3_EXE"
option = "--exe"

[inputs.h3maped]
name = "GOG Complete map editor"
image = true
path = "build/orig/h3maped.exe"
size = 1
sha256 = "0"
env = "HOMM3_H3MAPED_EXE"
option = "--exe"

[inputs.h3ccmped]
name = "GOG Complete campaign editor"
image = true
path = "build/orig/h3ccmped.exe"
size = 1
sha256 = "0"
env = "HOMM3_H3CCMPED_EXE"
option = "--exe"
"""


class ImageGateTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory())).resolve()
        (self.root / "config").mkdir()
        (self.root / "config/project.toml").write_text(_PINS)
        (self.root / "build/orig").mkdir(parents=True)
        self.enterContext(patch.object(build, "ROOT", self.root))
        self.enterContext(patch.dict(os.environ, {}, clear=False))
        for name in ("HOMM3_H3MAPED_EXE", "HOMM3_H3CCMPED_EXE", "HOMM3_MFC_SP3",
                     "HOMM3_IMAGE"):
            os.environ.pop(name, None)
        self.mfc = self.enterContext(patch.object(mfc_sp3, "staged", return_value=True))

    def _stage(self, name: str) -> None:
        (self.root / "build/orig" / name).write_bytes(b"x")

    def test_missing_executable_is_a_problem_for_each_unskipped_image(self):
        self._stage("h3maped.exe")
        problems = build._image_problems(set())
        self.assertEqual(len(problems), 1)
        self.assertIn("image h3ccmped", problems[0])
        self.assertIn("$HOMM3_H3CCMPED_EXE", problems[0])
        self.assertEqual(build._image_problems({"h3ccmped"}), [])

    def test_environment_override_counts_as_staged(self):
        self._stage("h3maped.exe")
        os.environ["HOMM3_H3CCMPED_EXE"] = "/elsewhere/h3ccmped.exe"
        self.assertEqual(build._image_problems(set()), [])

    def test_missing_mfc_overlay_is_a_problem_unless_named_by_environment(self):
        self._stage("h3maped.exe")
        self._stage("h3ccmped.exe")
        self.mfc.return_value = False
        self.assertEqual(len(build._image_problems(set())), 2)
        os.environ["HOMM3_MFC_SP3"] = "/elsewhere/mfc"
        self.assertEqual(build._image_problems(set()), [])

    def test_skip_image_parsing_rejects_unknown_keys(self):
        skipped, rest = build._skipped_images(
            ["--skip-image", "h3ccmped", "-j4", "--skip-image=h3maped"])
        self.assertEqual(skipped, {"h3ccmped", "h3maped"})
        self.assertEqual(rest, ["-j4"])
        for argv in (["--skip-image", "game"], ["--skip-image", "nope"], ["--skip-image"]):
            with self.assertRaises(ValueError):
                build._skipped_images(argv)

    def test_full_build_fails_before_compiling_when_an_image_is_missing(self):
        stderr = io.StringIO()
        with patch.object(build, "_run") as run, \
                patch("homm3.core.worktree_lock.hold", return_value=contextlib.nullcontext()), \
                contextlib.redirect_stderr(stderr), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(build.main([]), 1)
        run.assert_not_called()
        self.assertIn("image h3maped", stderr.getvalue())
        self.assertIn("image h3ccmped", stderr.getvalue())
        self.assertIn("--skip-image", stderr.getvalue())

    def test_skipped_image_is_named_and_never_built(self):
        self._stage("h3maped.exe")
        stdout = io.StringIO()
        with patch.object(build, "_run", return_value=0) as run, \
                contextlib.redirect_stdout(stdout):
            self.assertEqual(build._build_images(["-j4"], {"h3ccmped"}), [])
        self.assertIn("image h3ccmped: SKIPPED", stdout.getvalue())
        commands = [call.args for call in run.call_args_list]
        self.assertEqual(len(commands), 1)
        self.assertIn("h3maped", commands[0])


if __name__ == "__main__":
    unittest.main()
