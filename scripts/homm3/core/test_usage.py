"""Every dispatcher records exceptions and returns an error exit status."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.analysis import dreamcast
from homm3.sema import __main__ as sema, _common as sema_common
from homm3.vc6 import __main__ as vc6, _common as vc6_common


class LoggedTest(unittest.TestCase):
    def setUp(self):
        import os
        marker = patch.dict(os.environ, {"HOMM3_USAGE_TEST": "usage-unittest"})
        marker.start()
        self.addCleanup(marker.stop)


class UsageTest(LoggedTest):
    def test_dispatchers_log_all_exit_paths(self):
        for module, owner in ((sema, sema_common), (vc6, vc6_common), (dreamcast, dreamcast)):
            for outcome in (0, 1, 2, SystemExit(2), RuntimeError("injected failure")):
                with self.subTest(module=module.__name__, outcome=outcome), tempfile.TemporaryDirectory() as tmp:
                    path = Path(tmp) / "usage.log"
                    def dispatch(argv):
                        print("diagnostic", file=__import__("sys").stderr)
                        if isinstance(outcome, BaseException):
                            raise outcome
                        return outcome
                    with patch.object(owner, "LOG", path), patch.object(module, "_dispatch", dispatch), \
                            contextlib.redirect_stderr(io.StringIO()), contextlib.redirect_stdout(io.StringIO()) as output:
                        if isinstance(outcome, BaseException):
                            with self.assertRaises(SystemExit) as caught:
                                module.main(["probe", "argument with spaces"])
                            self.assertEqual(caught.exception.code, 2)
                        else:
                            self.assertEqual(module.main(["probe"]), outcome)
                    rows = path.with_suffix(".jsonl").read_text().splitlines()
                    self.assertEqual(len(rows), 1)
                    record = json.loads(rows[0])
                    self.assertEqual(record["rc"], 2 if isinstance(outcome, BaseException) else outcome)
                    self.assertGreaterEqual(record["duration_seconds"], 0)
                    self.assertEqual(len(record["revision"]), 40)
                    if isinstance(outcome, RuntimeError):
                        self.assertIn("RuntimeError: injected failure", record["error"])
                    elif record["rc"] >= 2:
                        self.assertIn("diagnostic", record["error"])
                    else:
                        self.assertIsNone(record["error"])
                    self.assertEqual(output.getvalue(), "")
                    self.assertEqual(len(path.read_text().splitlines()), 1)


class CliUsageTest(LoggedTest):
    def test_build_init_capture_child_errors_and_preserve_streams(self):
        import os
        import sys
        from homm3 import cli
        from homm3.core import common, usage
        for command, diagnostic, category in (
            ("build", "source.cpp(12) : error C2065: x", "compile.cpp"),
            ("init", "wineserver: bind: Operation not permitted", "environment.permission"),
            ("build", "winepath returned non-zero exit status 1", "environment.wine"),
        ):
            with self.subTest(command=command, category=category), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                child = ("import sys; print(" + repr(diagnostic) + "); "
                         "print('child stderr', file=sys.stderr); sys.exit(1)")
                def dispatch(argv):
                    return cli.run(sys.executable, "-c", child)
                with patch.object(cli, "ROOT", root), patch.object(common, "HOMM3_DIR", root), \
                        patch.object(cli, "_dispatch", side_effect=dispatch), \
                        patch.dict(os.environ, {"HOMM3_USAGE_TEST": "child-error-control"}), \
                        contextlib.redirect_stdout(io.StringIO()) as out, \
                        contextlib.redirect_stderr(io.StringIO()) as err:
                    self.assertEqual(cli.main([command]), 1)
                self.assertEqual(out.getvalue(), diagnostic + "\n")
                self.assertEqual(err.getvalue(), "child stderr\n")
                record = json.loads((root / "build/homm3_usage.jsonl").read_text())
                self.assertEqual(record["error_category"], category)
                self.assertEqual(record["outcome"], "error")
                self.assertIn(diagnostic, record["error"])
                self.assertIn("child stderr", record["error"])
                self.assertEqual(record["scope"], "cli")
                self.assertEqual(record["worktree"], str(root.resolve()))
                self.assertEqual(record["cwd"], str(Path.cwd()))
                self.assertTrue(record["is_test"])
                self.assertEqual(record["test_marker"], "child-error-control")
                self.assertGreater(record["duration_seconds"], 0)
                self.assertLessEqual(record["started_at"], record["time"])

    def test_nested_process_events_correlate_without_reusing_ids(self):
        import os
        import sys
        from homm3 import cli
        from homm3.core import usage, common
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            child = "\n".join([
                "from pathlib import Path",
                "import sys",
                f"sys.path.insert(0, {str(Path(__file__).resolve().parents[2])!r})",
                "from homm3.core import usage",
                "usage.run_logged(lambda argv: 1, [],",
                f"    lambda rc, **meta: usage.append(Path({str(root / 'child.log')!r}), 'child', rc, **meta))",
                "raise SystemExit(1)",
            ])
            def dispatch(argv):
                return cli.run(sys.executable, "-c", child)
            with patch.object(cli, "ROOT", root), patch.object(common, "HOMM3_DIR", root), \
                    patch.object(cli, "_dispatch", side_effect=dispatch), \
                    patch.dict(os.environ, {"HOMM3_USAGE_TEST": "nested-control"}):
                self.assertEqual(cli.main(["sema", "diff", "example"]), 1)
            parent = json.loads((root / "build/homm3_usage.jsonl").read_text())
            nested = json.loads((root / "child.jsonl").read_text())
            self.assertEqual(nested["parent_event_id"], parent["event_id"])
            self.assertNotEqual(nested["event_id"], parent["event_id"])
            for record in (parent, nested):
                self.assertEqual(record["outcome"], "difference")
                self.assertIsNone(record["error_category"])
                self.assertIsNone(record["error"])
                self.assertEqual(record["test_marker"], "nested-control")

    def test_uuid_survives_copy_and_new_invocation_is_distinct(self):
        import shutil
        from homm3.core import usage
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "usage.log"
            usage.append(path, "same command", 0)
            copied = Path(tmp) / "copied.jsonl"
            shutil.copyfile(path.with_suffix(".jsonl"), copied)
            usage.append(path, "same command", 0)
            records = [json.loads(line) for line in path.with_suffix(".jsonl").read_text().splitlines()]
            self.assertEqual(records[0]["event_id"], json.loads(copied.read_text())["event_id"])
            self.assertNotEqual(records[0]["event_id"], records[1]["event_id"])

    def test_classification_prefers_environment_over_wrapping_traceback(self):
        from homm3.core.usage import classify_error
        prefix = "Traceback (most recent call last):\n"
        self.assertEqual(classify_error(prefix + "winepath failed"), "environment.wine")
        self.assertEqual(classify_error(prefix + "RuntimeError: bug"), "tool.exception")
        self.assertEqual(classify_error("ninja: build stopped: subcommand failed."), "command.failed")


if __name__ == "__main__":
    unittest.main()
