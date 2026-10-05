from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from homm3.core import cc_wrap
from homm3.core.cc_wrap import _compile_staged


class WinePrefixAnchorTests(unittest.TestCase):
    """cc_wrap must pin WINEPREFIX to this tree's prefix.

    The guard used to be `if not Path(os.environ.get("WINEPREFIX", "")).is_dir()`.
    Path("") is PosixPath("."), whose is_dir() is True, so an ABSENT WINEPREFIX
    satisfied the guard and the compile silently ran against the shared ~/.wine
    prefix instead of build/wineprefix. Every unit then failed with
    `fatal error C1083: Cannot open include file: 'va.h'` even though INCLUDE
    named the right directories, and it only looked healthy while a correct
    wineserver happened to be running.
    """

    @staticmethod
    def anchor(environ_value, root):
        env = {} if environ_value is None else {"WINEPREFIX": environ_value}
        return cc_wrap.anchor_wine_prefix(env, root=root)

    def test_absent_prefix_is_pinned_to_the_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(self.anchor(None, root),
                             str(root / "build/wineprefix"))
            self.assertEqual(self.anchor("", root),
                             str(root / "build/wineprefix"))

    def test_stale_prefix_is_pinned_to_the_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(self.anchor(str(root / "gone"), root),
                             str(root / "build/wineprefix"))

    def test_existing_prefix_is_respected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            other = root / "other"
            other.mkdir()
            self.assertEqual(self.anchor(str(other), root), str(other))

    def test_required_prefix_reports_a_missing_tree_prefix(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with self.assertRaises(cc_wrap.WineUnavailable) as caught:
                cc_wrap.anchor_wine_prefix({}, root=root, require=True)
            self.assertIn("homm3 init", str(caught.exception.code))
            (root / "build/wineprefix").mkdir(parents=True)
            env = {}
            cc_wrap.anchor_wine_prefix(env, root=root, require=True)
            self.assertEqual(env["WINEPREFIX"], str(root / "build/wineprefix"))

    def test_vc6_tools_share_the_anchor(self):
        # The shim and IL capture used `Path(os.environ.get("WINEPREFIX",
        # "")).is_dir()`, true for an unset variable, so `predict-inline
        # --trace` in a worktree ran on ~/.wine and could not find windows.h.
        from homm3.vc6 import il
        from homm3.vc6.shim import build
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "build/wineprefix").mkdir(parents=True)
            for module in (build, il):
                with self.subTest(module=module.__name__), \
                        patch.dict(os.environ, {}, clear=False), \
                        patch.object(module._common, "REPO", root), \
                        patch.object(module.shutil, "which", return_value="/bin/wine"), \
                        patch.object(cc_wrap, "ensure_wineserver"):
                    os.environ.pop("WINEPREFIX", None)
                    module._ensure_wine_env()
                    self.assertEqual(os.environ["WINEPREFIX"],
                                     str(root / "build/wineprefix"))


class StagedObjectTests(unittest.TestCase):
    def test_failed_compile_preserves_the_previous_object(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "hero.obj"
            out.write_bytes(b"previous")

            def fail(command, staged):
                self.assertEqual(command, [str(staged)])
                return "Wine failed", 1

            self.assertEqual(_compile_staged(out, lambda path: [str(path)],
                                             run=fail),
                             ("Wine failed", 1, False))
            self.assertEqual(out.read_bytes(), b"previous")
            self.assertEqual(list(Path(tmp).glob(".*.tmp.obj")), [])

    def test_success_replaces_the_previous_object(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "hero.obj"
            out.write_bytes(b"previous")

            def compile_object(command, staged):
                self.assertEqual(command, [str(staged)])
                staged.write_bytes(b"new object")
                return "", 0

            self.assertEqual(_compile_staged(out, lambda path: [str(path)],
                                             run=compile_object),
                             ("", 0, True))
            self.assertEqual(out.read_bytes(), b"new object")
            self.assertEqual(list(Path(tmp).glob(".*.tmp.obj")), [])

    def test_failed_compile_with_partial_object_preserves_previous_object(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "hero.obj"
            out.write_bytes(b"previous")

            def fail_after_writing(command, staged):
                self.assertEqual(command, [str(staged)])
                staged.write_bytes(b"partial object")
                return "compiler error", 1

            self.assertEqual(_compile_staged(
                out, lambda path: [str(path)], run=fail_after_writing),
                ("compiler error", 1, False))
            self.assertEqual(out.read_bytes(), b"previous")
            self.assertEqual(list(Path(tmp).glob(".*.tmp.obj")), [])


def _fake_tool(directory: Path, name: str, body: str) -> None:
    tool = directory / name
    tool.write_text("#!/bin/sh\n" + body)
    tool.chmod(0o755)


class WinepathFailureTests(unittest.TestCase):
    """A failed `winepath -w` used to surface as a raw CalledProcessError
    traceback with Wine's own explanation discarded (stderr=DEVNULL)."""

    def test_failure_reports_stderr_and_advice(self):
        failed = subprocess.CompletedProcess(
            ["winepath"], 1, stdout="",
            stderr="wineserver: bind /tmp/.wine-1000: Operation not permitted\n")
        with patch.object(cc_wrap.subprocess, "run", return_value=failed), \
                patch.dict(os.environ, {"WINEPREFIX": "/x/build/wineprefix"}):
            with self.assertRaises(cc_wrap.WineUnavailable) as caught:
                cc_wrap.winepath_w("/x/src/a.cpp")
        message = str(caught.exception.code)
        self.assertIn("Operation not permitted", message)
        self.assertIn("sandbox", message)
        self.assertIn("WINEPREFIX=/x/build/wineprefix", message)
        self.assertIsInstance(caught.exception, SystemExit)

    def test_missing_winepath_names_the_toolchain_shell(self):
        with patch.object(cc_wrap.subprocess, "run", side_effect=FileNotFoundError):
            with self.assertRaises(cc_wrap.WineUnavailable) as caught:
                cc_wrap.winepath_w("/x")
        self.assertIn("nix develop .#build", str(caught.exception.code))

    def test_success_returns_the_translated_path(self):
        ok = subprocess.CompletedProcess(["winepath"], 0, stdout="Z:\\x\n",
                                         stderr="fixme: noise\n")
        with patch.object(cc_wrap.subprocess, "run", return_value=ok):
            self.assertEqual(cc_wrap.winepath_w("/x"), "Z:\\x")

    def test_compiler_wrapper_exits_without_a_traceback(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            bin_dir = root / "bin"
            bin_dir.mkdir()
            _fake_tool(bin_dir, "wine", "exit 0\n")
            _fake_tool(bin_dir, "wineserver", "exit 0\n")
            _fake_tool(bin_dir, "winepath",
                       "echo 'wineserver: bind: Operation not permitted' >&2\nexit 1\n")
            msvc = root / "msvc"
            (msvc / "bin").mkdir(parents=True)
            (msvc / "bin/CL.EXE").write_bytes(b"")
            src = root / "a.cpp"
            src.write_text("int a;\n")
            scripts = Path(__file__).resolve().parents[2]
            env = dict(os.environ, PATH=f"{bin_dir}:{os.environ.get('PATH', '')}",
                       MSVC_DIR=str(msvc), WINEPREFIX=str(root),
                       PYTHONPATH=str(scripts))
            result = subprocess.run(
                [sys.executable, "-m", "homm3.core.cc_wrap", "--out",
                 str(root / "a.obj"), "--src", str(src), "--", "/c"],
                env=env, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("Traceback", result.stderr)
        self.assertIn("Operation not permitted", result.stderr)
        self.assertIn("winepath -w", result.stderr)


if __name__ == "__main__":
    unittest.main()
