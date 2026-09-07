"""Candidate preflight must not compile retail-only owners or hide recovery steps."""
import contextlib
import io
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.sema import _asm, diff, disasm
from homm3.sema.__main__ import _build_parser


class CandidateTest(unittest.TestCase):
    def context(self, unit):
        return SimpleNamespace(symbols=SimpleNamespace(
            resolve_fn=lambda _: ("retail_name", unit, 0x1000, 1, 0)))

    def test_retail_only_candidate_views_never_compile(self):
        for command in (("disasm", "--base"), ("disasm", "--source"),
                        ("diff", "--summary"), ("diff", "--structure"), ("diff", "--source")):
            with self.subTest(command=command), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                # Stale artifacts must not override the manifest admission check.
                (root / "seg_0013.obj").touch()
                (root / "seg_0013.c.obj").touch()
                module = disasm if command[0] == "disasm" else diff
                args = _build_parser().parse_args([command[0], "0x1000", command[1]])
                with patch.object(module, "get_context", return_value=self.context("seg_0013")), \
                        patch.object(_asm.manifest, "by_unit", return_value={"admitted": {}}), \
                        patch.multiple(_asm, BASE=root, TARGET=root), \
                        patch.object(_asm, "refresh_unit") as refresh, \
                        contextlib.redirect_stderr(io.StringIO()) as err:
                    if module is disasm:
                        with self.assertRaises(SystemExit) as stop:
                            module.run(args)
                        self.assertEqual(stop.exception.code, 2)
                    else:
                        self.assertEqual(module.run(args), 2)
                refresh.assert_not_called()
                self.assertIn("no candidate TU", err.getvalue())
                self.assertIn("homm3 sema disasm 0x1000", err.getvalue())
                self.assertNotIn("fix the source", err.getvalue())

    def test_missing_admitted_objects_are_not_called_retail_only(self):
        for command in (("disasm", "--base"), ("diff", "--summary")):
            with self.subTest(command=command), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                module = disasm if command[0] == "disasm" else diff
                args = _build_parser().parse_args([command[0], "0x1000", command[1], "--no-build"])
                with patch.object(module, "get_context", return_value=self.context("admitted")), \
                        patch.object(_asm.manifest, "by_unit", return_value={"admitted": {}}), \
                        patch.multiple(_asm, BASE=root, NORMAL_BASE=root, NORMAL_TARGET=root), \
                        contextlib.redirect_stderr(io.StringIO()) as err:
                    if module is disasm:
                        with self.assertRaises(SystemExit):
                            module.run(args)
                    else:
                        self.assertEqual(module.run(args), 2)
                self.assertIn("object missing", err.getvalue())
                self.assertIn("TU admitted", err.getvalue())
                self.assertIn("homm3 build", err.getvalue())
                self.assertNotIn("no candidate TU", err.getvalue())

    def test_missing_symbol_explains_claim_binding(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            obj = root / "admitted.obj"
            obj.touch()
            with patch.object(_asm.common, "HOMM3_DIR", root), patch.object(_asm, "BASE", root), \
                    patch.object(_asm.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "", "")), \
                    patch.object(_asm, "_public_text_symbols", return_value=set()), \
                    patch.object(_asm, "_function_text_symbols", return_value=set()), \
                    contextlib.redirect_stderr(io.StringIO()) as err:
                with self.assertRaises(SystemExit):
                    _asm.objdump(obj, "missing_name", 0)
            self.assertIn("symbol missing_name not found", err.getvalue())
            self.assertIn("candidate, TU admitted", err.getvalue())
            self.assertIn("VA/VA_COMPGEN", err.getvalue())

    def test_wine_refresh_failure_does_not_blame_cpp(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            ninja = root / "build.ninja"
            ninja.touch()
            def run(*a, **kw):
                return subprocess.CompletedProcess([], 1, "", "wineserver: bind: Operation not permitted")
            with patch.multiple(_asm, NINJA_FILE=ninja, REFRESH_LOCK=root / "lock"), \
                    contextlib.redirect_stderr(io.StringIO()) as err:
                with self.assertRaises(SystemExit):
                    _asm.refresh_unit("admitted", run=run)
            self.assertIn("environment.permission", err.getvalue())
            self.assertIn("execution permissions", err.getvalue())
            self.assertNotIn("fix the source", err.getvalue())

    def test_stale_normalization_names_unit_and_recovery(self):
        from homm3.build import normalized_freshness
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            obj = root / "admitted.obj"
            obj.touch()
            with patch.object(_asm, "NORMAL_BASE", root), \
                    patch.object(normalized_freshness, "freshness_problems", return_value=["raw input changed"]), \
                    contextlib.redirect_stderr(io.StringIO()) as err:
                with self.assertRaises(SystemExit):
                    _asm.objdump(obj, "name", 0)
            self.assertIn("stale normalized comparison object for TU admitted", err.getvalue())
            self.assertIn("homm3 build --fast admitted", err.getvalue())
