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


class UsageTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
