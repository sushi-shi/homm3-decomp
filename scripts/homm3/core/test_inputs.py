"""Input admission and init negative controls, without game bytes or Wine."""
from __future__ import annotations

import contextlib
from dataclasses import replace
import hashlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3 import cli
from homm3.core import inputs


class InputsTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.retail_bytes = b"synthetic retail input"
        self.retail = replace(inputs.RETAIL, destination=self.root / "build/orig/HEROES3.EXE",
                              size=len(self.retail_bytes),
                              sha256=hashlib.sha256(self.retail_bytes).hexdigest())
        self.retail_source = self.root / "user files/retail.EXE"
        self.retail_source.parent.mkdir()
        self.retail_source.write_bytes(self.retail_bytes)
        self.enterContext(patch.object(inputs, "RETAIL", self.retail))
        self.enterContext(patch.dict("os.environ", {}, clear=True))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))

    def test_environment_stages_input_then_source_is_unneeded(self):
        with patch.dict("os.environ", {"HOMM3_EXE": str(self.retail_source)}):
            inputs.stage_executable(self.retail)
        before = self.retail.destination.stat().st_mtime_ns
        self.retail_source.unlink()
        self.assertEqual(inputs.stage_executable(self.retail).read_bytes(), self.retail_bytes)
        self.assertEqual(self.retail.destination.stat().st_mtime_ns, before)

    def test_no_implicit_directory_search(self):
        with self.assertRaisesRegex(inputs.InputError, self.retail.env_var):
            inputs.stage_executable(self.retail)

    def test_wrong_size_and_same_size_wrong_hash_leave_no_copy(self):
        for payload, message in [(b"short", "size"), (b"x" * len(self.retail_bytes), "sha256")]:
            self.retail_source.write_bytes(payload)
            with self.subTest(message=message), self.assertRaisesRegex(inputs.InputError, message):
                inputs.stage_executable(self.retail, self.retail_source)
            self.assertFalse(self.retail.destination.exists())

    def test_invalid_explicit_environment_does_not_fall_back_to_good_copy(self):
        inputs.stage_executable(self.retail, self.retail_source)
        with patch.dict("os.environ", {"HOMM3_EXE": str(self.root / "missing")}):
            with self.assertRaisesRegex(inputs.InputError, "cannot read"):
                inputs.stage_executable(self.retail)

    def test_tampered_copy_is_rejected_and_explicit_valid_source_repairs_it(self):
        inputs.stage_executable(self.retail, self.retail_source)
        self.retail.destination.write_bytes(b"x" * len(self.retail_bytes))
        with self.assertRaisesRegex(inputs.InputError, "sha256"):
            inputs.stage_executable(self.retail)
        inputs.stage_executable(self.retail, self.retail_source)
        self.assertEqual(self.retail.destination.read_bytes(), self.retail_bytes)

    def test_failed_replacement_preserves_previous_file_and_removes_temp(self):
        self.retail.destination.parent.mkdir(parents=True)
        self.retail.destination.write_bytes(b"previous")
        with patch.object(Path, "replace", side_effect=OSError("interrupted")):
            with self.assertRaisesRegex(inputs.InputError, "cannot stage"):
                inputs.stage_executable(self.retail, self.retail_source)
        self.assertEqual(self.retail.destination.read_bytes(), b"previous")
        self.assertEqual(list(self.retail.destination.parent.iterdir()), [self.retail.destination])

    def test_init_cli_overrides_environment_and_reuses_staged_input(self):
        with patch.object(cli, "ROOT", self.root), patch.object(cli, "run_module", return_value=0) as run:
            with patch.dict("os.environ", {"HOMM3_EXE": "missing"}):
                self.assertEqual(cli.main(["init", "--exe", str(self.retail_source), "--no-smoke"]), 0)
            self.assertEqual([call.args for call in run.call_args_list], [
                ("homm3.build.configure",), ("homm3.init.toolchain", "--no-smoke"),
                ("homm3.build.compilation_database",)])
            self.retail_source.unlink()
            self.assertEqual(cli.main(["init", "--no-smoke"]), 0)

    def test_init_rejects_missing_input_before_toolchain_or_configure(self):
        with patch.object(cli, "run_module") as run:
            self.assertEqual(cli.main(["init"]), 1)
            run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
