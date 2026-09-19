"""Score writes must reject both stale comparison copies and unbuilt sources."""
import contextlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from homm3.build.normalized_freshness import write_stamp
from homm3.match import status


class FreshnessTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.enterContext(patch.object(status, "OBJDIFF_DIR", self.root))
        self.enterContext(patch.object(status.common, "HOMM3_DIR", self.root))
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))
        self.raw = self.root / "raw.obj"
        self.raw.write_bytes(b"old object")
        self.objects = []
        for side in ("base", "target"):
            path = self.root / "normalized" / side / "unit.obj"
            path.parent.mkdir(parents=True)
            path.write_bytes(b"normalized object")
            write_stamp(path, {"raw": self.raw})
            self.objects.append(path)
        (self.root / "objdiff.json").write_text(json.dumps({"units": [{
            "base_path": str(self.objects[0]), "target_path": str(self.objects[1])}]}))

    def test_raw_edit_rejected_before_objdiff_or_ledger_write(self):
        status.require_fresh_comparisons()
        self.raw.write_bytes(b"rebuilt object")
        with patch.object(status.subprocess, "run") as run:
            with self.assertRaises(SystemExit):
                status.refresh_report()
            run.assert_not_called()
        with patch.object(status, "require_built_sources"), \
                patch.object(status, "write_baseline") as write:
            with self.assertRaises(SystemExit):
                status.cmd_update({})
            write.assert_not_called()

    def test_missing_or_unstamped_target_is_not_a_fresh_report(self):
        self.objects[1].with_name("unit.obj.stamp.json").unlink()
        with self.assertRaises(SystemExit):
            status.require_fresh_comparisons()
        self.objects[1].unlink()
        with self.assertRaises(SystemExit):
            status.require_fresh_comparisons()

    @unittest.skipUnless(shutil.which("ninja"), "requires Ninja")
    def test_ninja_detects_source_header_and_command_changes_without_building(self):
        source, header = self.root / "source", self.root / "header"
        source.write_text("source")
        header.write_text("header")
        graph = "rule copy\n  command = cp source object\nbuild object: copy source | header\nbuild objects: phony object\n"
        (self.root / "build.ninja").write_text(graph)
        subprocess.run(["ninja", "objects"], cwd=self.root, check=True, capture_output=True)
        status.require_built_sources()
        for path in (source, header, self.root / "build.ninja"):
            with self.subTest(path=path.name):
                path.write_text(graph.replace("cp source object", "cp -f source object")
                                if path.name == "build.ninja" else "edited")
                with patch.object(status, "write_baseline") as write:
                    with self.assertRaises(SystemExit):
                        status.cmd_update({})
                    write.assert_not_called()
                self.assertEqual((self.root / "object").read_text(), "source")
                if path.name != "build.ninja":
                    path.write_text(path.name)
                    subprocess.run(["ninja", "objects"], cwd=self.root, check=True, capture_output=True)

    def test_invalid_subcommand_does_not_generate_a_report(self):
        with patch.object(status, "refresh_report") as report:
            self.assertEqual(status.main(["show"]), 2)
            report.assert_not_called()

    def test_no_work_does_not_depend_on_ninjas_human_message(self):
        result = subprocess.CompletedProcess([], 0, "notice: using Ninja wrapper\n", "")
        with patch.object(status.subprocess, "run", return_value=result) as run:
            status.require_built_sources()
            marker = run.call_args.kwargs["env"]["NINJA_STATUS"]
            result.stdout = marker + "copy source object\n"
            with self.assertRaises(SystemExit):
                status.require_built_sources()

    @unittest.skipUnless(shutil.which("ninja"), "requires Ninja")
    def test_read_only_views_reject_unbuilt_source_before_loading_scores(self):
        (self.root / "source").write_text("before merge")
        (self.root / "build.ninja").write_text(
            "rule copy\n  command = cp source compiled\n"
            "build compiled: copy source\nbuild objects: phony compiled\n")
        subprocess.run(["ninja", "objects"], cwd=self.root, check=True, capture_output=True)
        (self.root / "source").write_text("after merge")
        # Normalized objects and their stamps still agree with the old raw
        # object. Only checking that chain would silently show pre-merge scores.
        status.require_fresh_comparisons()
        for argv in ([], ["functions", "unit"], ["--write-readme"]):
            with self.subTest(argv=argv), patch.object(status, "refresh_report") as report, \
                    patch.object(status, "write_readme"), patch.object(status, "cmd_summary"), \
                    patch.object(status, "cmd_functions"):
                with self.assertRaises(SystemExit):
                    status.main(argv)
                report.assert_not_called()
