"""One mutating homm3 command per worktree: wait with a clear message."""
import io
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import threading
import time
import unittest

from homm3.core import worktree_lock

HOLDER = """
import sys, time
from pathlib import Path
from homm3.core import worktree_lock
with worktree_lock.hold("homm3 build --fast cursor", lock=Path(sys.argv[1])):
    print("held", flush=True)
    sys.stdin.readline()
"""


class WorktreeLockTest(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.lock = self.root / "build" / worktree_lock.LOCK_NAME
        self.lock.parent.mkdir()
        os.environ.pop(worktree_lock.ENV, None)

    def holder(self, env=None):
        process = subprocess.Popen(
            [sys.executable, "-c", HOLDER, str(self.lock)], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, text=True,
            env=env or {k: v for k, v in os.environ.items() if k != worktree_lock.ENV})
        def reap():
            if process.poll() is None:
                process.kill()
            process.wait()
            process.stdin.close()
            process.stdout.close()
        self.addCleanup(reap)
        self.assertEqual(process.stdout.readline().strip(), "held")
        return process

    def test_second_command_waits_and_names_the_holder(self):
        process = self.holder()
        stream = io.StringIO()
        acquired = threading.Event()

        def wait():
            with worktree_lock.hold("homm3 delink", lock=self.lock, stream=stream, poll=0.05):
                acquired.set()
        thread = threading.Thread(target=wait)
        thread.start()
        time.sleep(0.3)
        self.assertFalse(acquired.is_set())
        message = stream.getvalue()
        self.assertIn("another `homm3 build --fast cursor`", message)
        self.assertIn(f"pid {process.pid}", message)
        self.assertIn("is running in this worktree; waiting", message)
        process.stdin.write("\n")
        process.stdin.flush()
        thread.join(10)
        self.assertTrue(acquired.is_set())
        self.assertIn("worktree lock acquired", stream.getvalue())

    def test_a_killed_holder_releases_the_lock(self):
        process = self.holder()
        process.send_signal(signal.SIGKILL)
        process.wait()
        stream = io.StringIO()
        started = time.monotonic()
        with worktree_lock.hold("homm3 build", lock=self.lock, stream=stream, poll=0.05):
            pass
        self.assertLess(time.monotonic() - started, 2)
        self.assertEqual(stream.getvalue(), "")

    def test_stale_record_of_a_dead_pid_is_ignored(self):
        dead = subprocess.Popen([sys.executable, "-c", "pass"])
        dead.wait()
        self.lock.with_name(self.lock.name + ".json").write_text(
            json.dumps({"pid": dead.pid, "command": "homm3 build", "started": ""}))
        self.assertIsNone(worktree_lock.holder(self.lock))
        stream = io.StringIO()
        with worktree_lock.hold("homm3 build", lock=self.lock, stream=stream):
            self.assertEqual(worktree_lock.holder(self.lock)["pid"], os.getpid())
        self.assertEqual(stream.getvalue(), "")
        self.assertIsNone(worktree_lock.holder(self.lock))

    def test_reentrant_in_process_and_for_children(self):
        stream = io.StringIO()
        with worktree_lock.hold("homm3 build", lock=self.lock, stream=stream):
            with worktree_lock.hold("homm3 delink", lock=self.lock, stream=stream):
                pass
            # A child (e.g. the module process the CLI spawns) inherits it.
            self.assertEqual(os.environ[worktree_lock.ENV], str(os.getpid()))
            child = subprocess.run(
                [sys.executable, "-c",
                 "import sys; from pathlib import Path; from homm3.core import worktree_lock\n"
                 "with worktree_lock.hold('homm3 build', lock=Path(sys.argv[1]), poll=0.05):\n"
                 "    print('entered')", str(self.lock)],
                capture_output=True, text=True, timeout=10)
            self.assertEqual(child.stdout.strip(), "entered")
            self.assertEqual(child.stderr, "")
        self.assertNotIn(worktree_lock.ENV, os.environ)
        self.assertEqual(stream.getvalue(), "")


if __name__ == "__main__":
    unittest.main()
