"""Build-mode controls: full checkpoints refresh targets; iteration preserves them."""
from __future__ import annotations

import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.build import build, configure, delink, normalize_objs
from homm3.cleanliness import board
from homm3.match import banked_rows, single_view, status, verify_va_claims


class BuildModeTest(unittest.TestCase):
    def setUp(self):
        directory = self.enterContext(tempfile.TemporaryDirectory())
        self.root = Path(directory)
        self.target = self.root / "build/objdiff/target/cursor.c.obj"
        self.target.parent.mkdir(parents=True)
        self.target.write_bytes(b"existing retail target")
        self.events = []
        self.mocks = {}
        self.enterContext(patch.object(build, "ROOT", self.root))
        self.enterContext(patch.object(status, "REPORT", self.root / "build/objdiff/report.json"))
        self.enterContext(patch.object(status, "overall_line", return_value="report"))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))

        for name, module, function, result in [
            ("configure", configure, "main", None),
            ("compile", build, "_run", 0),
            ("delink", delink, "main", 0),
            ("normalize", normalize_objs, "main", 0),
            ("report", status, "load_report", {}),
            ("check", status, "cmd_check", None),
            ("checkpoint", status, "cmd_update", None),
            ("banked", banked_rows, "run_gate", []),
            ("claims", verify_va_claims, "run_gate", []),
            ("single_view", single_view, "run_gate", []),
            ("cleanliness", board, "check_and_roll", []),
            ("readme", status, "write_readme", None),
        ]:
            def called(*args, _name=name, _result=result, **kwargs):
                self.events.append(_name)
                return _result
            self.mocks[name] = self.enterContext(patch.object(module, function, side_effect=called))

    def test_full_build_refreshes_existing_targets_before_checkpoint(self):
        self.assertEqual(build.main([]), 0)
        self.assertEqual(self.events, ["configure", "compile", "delink", "report",
                                      "check", "checkpoint", "banked", "claims",
                                      "single_view", "cleanliness", "readme"])
        self.mocks["compile"].assert_called_once_with("ninja")
        self.mocks["normalize"].assert_not_called()  # delink already normalizes

    def test_full_build_also_initializes_missing_targets(self):
        self.target.unlink()
        self.assertEqual(build.main([]), 0)
        self.mocks["delink"].assert_called_once_with([])

    def test_fast_build_preserves_targets_and_skips_checkpoint(self):
        self.assertEqual(build.main(["--fast", "cursor"]), 0)
        self.assertEqual(self.events, ["configure", "compile", "normalize", "configure", "report"])
        self.mocks["compile"].assert_called_once_with("ninja", "cursor")
        self.assertEqual(self.target.read_bytes(), b"existing retail target")
        self.mocks["delink"].assert_not_called()
        self.mocks["checkpoint"].assert_not_called()

    def test_fast_build_cannot_silently_bootstrap_a_delink(self):
        self.target.unlink()
        self.assertEqual(build.main(["--fast", "cursor"]), 1)
        self.assertEqual(self.events, [])

    def test_failed_compilation_cannot_delink_from_stale_objects(self):
        self.mocks["compile"].side_effect = lambda *args: 1
        self.assertEqual(build.main([]), 1)
        self.mocks["delink"].assert_not_called()
        self.mocks["report"].assert_not_called()

    def test_failed_delink_cannot_update_checkpoint(self):
        self.mocks["delink"].side_effect = lambda *args: 1
        self.assertEqual(build.main([]), 1)
        self.mocks["report"].assert_not_called()
        self.mocks["checkpoint"].assert_not_called()

    def test_failed_normalization_cannot_report_a_fast_build(self):
        self.mocks["normalize"].side_effect = lambda *args: 1
        self.assertEqual(build.main(["--fast", "cursor"]), 1)
        self.mocks["report"].assert_not_called()


if __name__ == "__main__":
    unittest.main()
