"""A partial checkpoint must prove its scope and leave all other evidence alone."""
import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.build.normalized_freshness import write_stamp
from homm3.core import compile_receipt
from homm3.match import scoped_status as scoped, status


class ScopedFreshnessTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.directory = self.root / "build/objdiff"
        self.directory.mkdir(parents=True)
        self.enterContext(patch.object(status, "OBJDIFF_DIR", self.directory))
        self.enterContext(patch.object(status, "SYMBOL_NAMES", self.root / "names.csv"))
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))
        manifest = {"flags": {"default": ["/O2"]}, "unit": [
            {"unit": "unit", "source": "src/unit.cpp", "flags": "default"},
            {"unit": "other", "source": "src/other.cpp", "flags": "default"}]}
        self.project = SimpleNamespace(root=self.root, manifest=manifest,
                                       toolchain=self.root / "compiler", includes=[self.root / "include"])
        inputs = {"src/unit.cpp": '#include "header.h"\nint function() { return 1; }',
                  "include/header.h": "// header", "config/project.toml": "project",
                  "config/units.toml": "units", "compiler/bin/cl.exe": "compiler",
                  "scripts/homm3/core/cc_wrap.py": "wrapper",
                  "scripts/homm3/core/compile_receipt.py": "receipt", "names.csv": "names"}
        for name, text in inputs.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        self.raw, self.target = self.directory / "base/unit.obj", self.directory / "target/unit.c.obj"
        for path in (self.raw, self.target):
            path.parent.mkdir(parents=True)
            path.write_bytes(b"object")
        compile_receipt.publish(self.raw, compile_receipt.snapshot(
            [self.root / name for name in inputs if name != "names.csv"]), ["/O2"])
        self.paths = [self.directory / "normalized/base/unit.obj",
                      self.directory / "normalized/target/unit.c.obj"]
        for path, roles in zip(self.paths, ({"raw": self.raw, "target": self.target},
                                           {"raw": self.target, "base": self.raw})):
            path.parent.mkdir(parents=True)
            path.write_bytes(b"normalized")
            write_stamp(path, {**roles, "project": self.root / "config/project.toml",
                               "symbol_names": status.SYMBOL_NAMES})
        self.config = {"options": {"functionRelocDiffs": "all"}, "units": [{
            "name": "unit", "base_path": str(self.paths[0]), "target_path": str(self.paths[1])}]}
        (self.directory / "objdiff.json").write_text(json.dumps(self.config))
        self.specs = {"unit": manifest["unit"][0]}
        self.ninja = self.enterContext(patch.object(status, "require_built_sources"))

    def test_valid_scope_does_not_need_unselected_object(self):
        specs, config = scoped.selection(self.project, {"unit"})
        scoped.require_fresh(self.project, specs, config)
        self.ninja.assert_called_once_with({"unit"})

    def test_unknown_missing_duplicate_and_non_strict_selections_rejected(self):
        for units in ({"typo"}, {"other"}, set()):
            with self.subTest(units=units), self.assertRaises(SystemExit):
                scoped.selection(self.project, units)
        self.config["units"] *= 2
        (self.directory / "objdiff.json").write_text(json.dumps(self.config))
        with self.assertRaises(SystemExit):
            scoped.selection(self.project, {"unit"})
        self.config["units"] = self.config["units"][:1]
        self.config["options"]["functionRelocDiffs"] = "none"
        (self.directory / "objdiff.json").write_text(json.dumps(self.config))
        with self.assertRaises(SystemExit):
            scoped.selection(self.project, {"unit"})

    def test_receipt_rejects_header_edit_even_with_restored_timestamp(self):
        path = self.root / "include/header.h"
        old = path.stat()
        path.write_text("// edited")
        os.utime(path, ns=(old.st_atime_ns, old.st_mtime_ns))
        with self.assertRaises(SystemExit):
            scoped.require_fresh(self.project, self.specs, self.config)

    def test_missing_receipt_or_changed_raw_cannot_be_banked(self):
        receipt = Path(str(self.raw) + ".inputs.json")
        text = receipt.read_text()
        receipt.unlink()
        with self.assertRaises(SystemExit):
            scoped.require_fresh(self.project, self.specs, self.config)
        receipt.write_text(text)
        self.raw.write_bytes(b"new object")
        with self.assertRaises(SystemExit):
            scoped.require_fresh(self.project, self.specs, self.config)

    def test_fresh_raw_cannot_hide_stale_normalized_or_unpaired_stamp(self):
        self.target.write_bytes(b"new target")
        with self.assertRaises(SystemExit):
            scoped.require_fresh(self.project, self.specs, self.config)
        self.target.write_bytes(b"object")
        write_stamp(self.paths[0], {"raw": self.raw})
        with self.assertRaises(SystemExit):
            scoped.require_fresh(self.project, self.specs, self.config)

    def test_report_project_contains_only_selected_absolute_paths(self):
        def run(command, **kwargs):
            directory = Path(command[2])
            config = json.loads((directory / "objdiff.json").read_text())
            self.assertEqual([u["name"] for u in config["units"]], ["unit"])
            self.assertTrue(Path(config["units"][0]["base_path"]).is_absolute())
            (directory / "report.json").write_text(json.dumps({"units": [{
                "name": "unit", "functions": [{"name": "function", "fuzzy_match_percent": 100}]}]}))
            return subprocess.CompletedProcess(command, 0, "", "")
        with patch.object(scoped.subprocess, "run", side_effect=run):
            self.assertEqual(scoped.measure(self.config)["units"][0]["name"], "unit")

    def checkpoint_setup(self):
        ledger = self.root / "baseline.tsv"
        original = (f"# score_policy={status.SCORE_POLICY}\r\n"
                    "unit\told\t60\t70\t90\t0x1\ttokens1:old\n"
                    "other\tuntouched\t50.000000\t80\t100\t0x2\ttokens1:banked\r\n")
        ledger.write_bytes(original.encode())
        self.enterContext(patch.object(status, "BASELINE", ledger))
        self.enterContext(patch.object(scoped, "Project", return_value=self.project))
        hashes = self.enterContext(patch.object(status, "source_hash_pair", return_value=(
            {("unit", "function"): "tokens1:new"}, {})))
        self.enterContext(patch.object(status, "function_rvas", return_value={
            ("unit", "function"): 1, ("other", "stale"): 2}))
        readme = self.enterContext(patch.object(status, "write_readme"))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))
        return ledger, original, hashes, readme

    def test_update_preserves_unselected_records_and_only_scans_selected_source(self):
        ledger, original, hashes, readme = self.checkpoint_setup()
        report = {"units": [{"name": "unit", "functions": [
            {"name": "function", "fuzzy_match_percent": 75}]}]}
        with patch.object(scoped, "measure", return_value=report):
            scoped.update({"unit"}, readme=True)
        self.assertTrue(ledger.read_bytes().endswith(original.splitlines(keepends=True)[2].encode()))
        hashes.assert_called_once_with(only_units={"unit"})
        rows = readme.call_args.kwargs["checkpoint_rows"]
        self.assertEqual(rows[("other", "untouched")].cur, 50)
        self.assertEqual(rows[("unit", "function")].max, 75)
        self.assertEqual(self.ninja.call_count, 2)

    def test_source_change_during_measurement_rejected_before_any_checkpoint_write(self):
        ledger, original, hashes, readme = self.checkpoint_setup()
        def measure(config):
            (self.root / "include/header.h").write_text("// changed during objdiff")
            return {"units": [{"name": "unit", "functions": [
                {"name": "function", "fuzzy_match_percent": 75}]}]}
        with patch.object(scoped, "measure", side_effect=measure), self.assertRaises(SystemExit):
            scoped.update({"unit"}, readme=True)
        self.assertEqual(ledger.read_bytes(), original.encode())
        readme.assert_not_called()


class ScopedAccountingTest(unittest.TestCase):
    def setUp(self):
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))
        self.old = {("unit", "old"): status.MatchRow(60, 70, 90, 1, "tokens1:old"),
                    ("other", "untouched"): status.MatchRow(50, 80, 100, 2, "tokens1:banked")}
        self.report = {"units": [{"name": "unit", "functions": [
            {"name": "new", "fuzzy_match_percent": 75}]}]}

    def test_rva_rename_resets_only_selected_source_and_preserves_other_cur(self):
        rows, stats = scoped.merge_rows(self.report, self.old, {"unit"},
            {("unit", "new"): "tokens1:new"}, {}, {("unit", "new"): 1})
        self.assertEqual(rows[("unit", "new")], status.MatchRow(75, 75, 90, 1, "tokens1:new"))
        self.assertIs(rows[("other", "untouched")], self.old[("other", "untouched")])
        self.assertNotIn(("unit", "old"), rows)

    def test_missing_body_and_cross_scope_move_rejected(self):
        for rva in (2, 3):
            with self.subTest(rva=rva), self.assertRaises(SystemExit):
                scoped.merge_rows(self.report, self.old, {"unit"}, {}, {}, {("unit", "new"): rva})

    def test_unselected_rows_and_comments_remain_byte_identical(self):
        original = "# header\r\nunit\told\t60\t70\t90\t0x1\ttokens1:old\n" \
                   "# retained note\nother\tuntouched\t50.000000\t80\t100\t0x2\ttokens1:banked\n"
        result = scoped.ledger_text(original, self.old, {"unit"})
        self.assertTrue(result.startswith("# header\r\n"))
        self.assertTrue(result.endswith(original.split("# retained note")[1]))
        self.assertIn("# retained note\n", result)

    def test_rationale_stays_with_renamed_body_and_unselected_neighbor(self):
        original = ("# header\n"
                    "other\tuntouched\t50\t80\t100\t0x2\ttokens1:banked\n"
                    "# This helper preserves the original argument order.\n"
                    "unit\told\t60\t70\t90\t0x1\ttokens1:old\n"
                    "# Neighbor rationale.\n"
                    "neighbor\tf\t100\t100\t100\t0x3\ttokens1:neighbor\n")
        rows = {("unit", "renamed"): status.MatchRow(75, 75, 90, 1, "tokens1:new")}
        result = scoped.ledger_text(original, rows, {"unit"})
        self.assertEqual(result, original.replace(
            "unit\told\t60\t70\t90\t0x1\ttokens1:old\n",
            "unit\trenamed\t75.0000\t75.0000\t90.0000\t0x1\ttokens1:new\n"))

    def test_policy_change_rejected_before_measurement(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "baseline.tsv"
            path.write_text("# score_policy=old\n")
            with patch.object(status, "BASELINE", path), patch.object(scoped, "measure") as measure:
                with self.assertRaises(SystemExit):
                    scoped.update({"unit"})
                measure.assert_not_called()

    def test_scoped_readme_uses_banked_scores_and_retail_sizes(self):
        rows = dict(self.old)
        rows[("unit", "old")] = status.MatchRow(100, 100, 100, 1, "tokens1:new")
        summary = ({1: "target", 2: "target"}, {1: 20, 2: 80}, {"target": (2, 100)})
        units = [{"unit": u, "source": f"src/{u}.cpp"} for u in ("unit", "other")]
        with tempfile.TemporaryDirectory() as tmp:
            readme = Path(tmp) / "README.md"
            readme.write_text(status.RM_START + "\n" + status.RM_END)
            with patch.object(status, "README_PATH", readme), \
                 patch.object(status, "projected_rows", side_effect=AssertionError("unselected projection")), \
                 patch.object(status, "function_rvas", side_effect=AssertionError("unselected live names")), \
                 patch("homm3.match.universe.summary", return_value=summary), \
                 patch("homm3.build.configure.load_manifest", return_value=({}, {}, units)), \
                 contextlib.redirect_stdout(io.StringIO()):
                status.write_readme({"units": [{"name": "other", "functions": [
                    {"name": "untouched", "size": 999999, "fuzzy_match_percent": 100}]}]},
                    checkpoint_rows=rows)
            text = readme.read_text()
            self.assertIn("84.00% matched (MAX)", text)
            self.assertRegex(text, r"\| CUR +\| +1 \| +60\.00% \| each unit's last measured checkpoint")
            self.assertRegex(text, r"\| MAX +\| +1 \| +84\.00% \|")
            self.assertRegex(text, r"\| HIST +\| +2 \| +100\.00% \|")

    def test_ninja_checks_only_selected_targets_and_still_rejects_pending_edge(self):
        result = subprocess.CompletedProcess([], 0, "", "")
        with patch.object(status.subprocess, "run", return_value=result) as run:
            status.require_built_sources({"unit"})
            self.assertEqual(run.call_args.args[0], ["ninja", "-n", "unit"])
            result.stdout = run.call_args.kwargs["env"]["NINJA_STATUS"] + "VC6 unit"
            with self.assertRaises(SystemExit):
                status.require_built_sources({"unit"})

    def test_cli_requires_update_and_unit_argument(self):
        self.assertEqual(status.main(["update", "--unit"]), 2)
        self.assertEqual(status.main(["check", "--unit", "unit"]), 2)
        with patch.object(scoped, "update", return_value=0) as update:
            self.assertEqual(status.main(["update", "--unit", "unit", "--unit", "other", "--write-readme"]), 0)
            update.assert_called_once_with({"unit", "other"}, readme=True)


if __name__ == "__main__":
    unittest.main()
