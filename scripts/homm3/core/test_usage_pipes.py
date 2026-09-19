"""Real early-closing readers must not hide child failures or stop builds."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


class ClosedPipeTest(unittest.TestCase):
    def test_inline_and_forwarded_output_finish_and_preserve_exit_status(self):
        scripts = str(Path(__file__).resolve().parents[2])
        for forwarded in (False, True, "unwrapped"):
            for expected in (0, 7):
                with self.subTest(forwarded=forwarded, expected=expected), tempfile.TemporaryDirectory() as tmp:
                    marker = Path(tmp) / "completed"
                    work = ("from pathlib import Path\n"
                            "for i in range(100000): print('line', flush=True)\n"
                            f"Path({str(marker)!r}).write_text('done')\n")
                    body = ("return run_process([sys.executable, '-c', "
                            + repr(work + f"raise SystemExit({expected})") + "], cwd='.')"
                            if forwarded else "exec(" + repr(work) + f"); return {expected}")
                    code = ("import sys\nfrom homm3.core.usage import run_logged, run_process\n"
                            f"def dispatch(argv):\n    {body}\n"
                            + ("raise SystemExit(dispatch([]))" if forwarded == "unwrapped"
                               else "raise SystemExit(run_logged(dispatch, [], lambda *a, **k: None))"))
                    process = subprocess.Popen([sys.executable, "-c", code],
                        env=dict(os.environ, PYTHONPATH=scripts),
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    try:
                        self.assertEqual(process.stdout.readline(), b"line\n")
                        process.stdout.close()
                        error = process.stderr.read()
                        self.assertEqual(process.wait(timeout=30), expected, error.decode())
                        self.assertEqual(error, b"")
                        self.assertEqual(marker.read_text(), "done")
                    finally:
                        if process.poll() is None:
                            process.kill()
                            process.wait()
                        process.stderr.close()
