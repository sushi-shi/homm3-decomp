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
from homm3.core.test_nb11 import fixture


class InputsTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.retail_bytes = b"synthetic retail input"
        self.dc_bytes = fixture()
        self.retail = replace(inputs.RETAIL, destination=self.root / "build/orig/HEROES3.EXE",
                              size=len(self.retail_bytes),
                              sha256=hashlib.sha256(self.retail_bytes).hexdigest())
        self.dc = replace(inputs.DREAMCAST, destination=self.root / "build/orig/dreamcast/H3.EXE",
                          size=len(self.dc_bytes), sha256=hashlib.sha256(self.dc_bytes).hexdigest())
        self.retail_source = self.root / "user files/retail.EXE"
        self.dc_source = self.root / "user files/dreamcast.EXE"
        self.retail_source.parent.mkdir()
        self.retail_source.write_bytes(self.retail_bytes)
        self.dc_source.write_bytes(self.dc_bytes)
        self.enterContext(patch.object(inputs, "RETAIL", self.retail))
        self.enterContext(patch.object(inputs, "DREAMCAST", self.dc))
        self.enterContext(patch.dict("os.environ", {}, clear=True))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))

    def test_environment_stages_both_inputs_then_sources_are_unneeded(self):
        with patch.dict("os.environ", {"HOMM3_EXE": str(self.retail_source),
                                        "HOMM3_DREAMCAST_EXE": str(self.dc_source)}):
            inputs.stage_executable(self.retail)
            inputs.stage_executable(self.dc)
        before = self.dc.destination.stat().st_mtime_ns
        self.retail_source.unlink()
        self.dc_source.unlink()
        self.assertEqual(inputs.stage_executable(self.retail).read_bytes(), self.retail_bytes)
        self.assertIn(0x100, inputs.dreamcast_symbols().procedures)
        self.assertEqual(self.dc.destination.stat().st_mtime_ns, before)

    def test_no_implicit_directory_search(self):
        for spec in (self.retail, self.dc):
            with self.subTest(spec=spec.name), self.assertRaisesRegex(inputs.InputError, spec.env_var):
                inputs.stage_executable(spec)

    def test_wrong_size_and_same_size_wrong_hash_leave_no_copy(self):
        for payload, message in [(b"short", "size"), (b"x" * len(self.dc_bytes), "sha256")]:
            self.dc_source.write_bytes(payload)
            with self.subTest(message=message), self.assertRaisesRegex(inputs.InputError, message):
                inputs.stage_executable(self.dc, self.dc_source)
            self.assertFalse(self.dc.destination.exists())

    def test_invalid_explicit_environment_does_not_fall_back_to_good_copy(self):
        inputs.stage_executable(self.dc, self.dc_source)
        with patch.dict("os.environ", {"HOMM3_DREAMCAST_EXE": str(self.root / "missing")}):
            with self.assertRaisesRegex(inputs.InputError, "cannot read"):
                inputs.stage_executable(self.dc)

    def test_tampered_copy_is_rejected_and_explicit_valid_source_repairs_it(self):
        inputs.stage_executable(self.dc, self.dc_source)
        self.dc.destination.write_bytes(b"x" * len(self.dc_bytes))
        with self.assertRaisesRegex(inputs.InputError, "sha256"):
            inputs.stage_executable(self.dc)
        inputs.stage_executable(self.dc, self.dc_source)
        self.assertEqual(self.dc.destination.read_bytes(), self.dc_bytes)

    def test_failed_replacement_preserves_previous_file_and_removes_temp(self):
        self.dc.destination.parent.mkdir(parents=True)
        self.dc.destination.write_bytes(b"previous")
        with patch.object(Path, "replace", side_effect=OSError("interrupted")):
            with self.assertRaisesRegex(inputs.InputError, "cannot stage"):
                inputs.stage_executable(self.dc, self.dc_source)
        self.assertEqual(self.dc.destination.read_bytes(), b"previous")
        self.assertEqual(list(self.dc.destination.parent.iterdir()), [self.dc.destination])

    def test_init_cli_overrides_environment_and_reuses_staged_inputs(self):
        with patch.object(cli, "ROOT", self.root), patch.object(cli, "run_module", return_value=0) as run:
            with patch.dict("os.environ", {"HOMM3_EXE": "missing", "HOMM3_DREAMCAST_EXE": "missing"}):
                self.assertEqual(cli.main(["init", "--exe", str(self.retail_source),
                                           "--dreamcast-exe", str(self.dc_source), "--no-smoke"]), 0)
            self.assertEqual([call.args for call in run.call_args_list], [
                ("homm3.build.configure",), ("homm3.init.toolchain", "--no-smoke")])
            self.retail_source.unlink()
            self.dc_source.unlink()
            self.assertEqual(cli.main(["init", "--no-smoke"]), 0)

    def test_init_rejects_missing_input_before_toolchain_or_configure(self):
        with patch.object(cli, "run_module") as run:
            self.assertEqual(cli.main(["init", "--exe", str(self.retail_source)]), 1)
            run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
