"""The measured Python call total must include worker and child work."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from homm3.core.perf import profile_environment, summarize_profiles


class ProfileCoverageTest(unittest.TestCase):
    def test_worker_and_child_calls_are_counted_once(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            child = root / 'child.py'
            child.write_text('def measured_work(): pass\nfor _ in range(5): measured_work()\n')
            script = ('import threading,subprocess,sys\n'
                      'def measured_work(): pass\n'
                      'def worker():\n    for _ in range(3): measured_work()\n'
                      'thread=threading.Thread(target=worker)\nthread.start()\nthread.join()\n'
                      f'subprocess.run([sys.executable, {str(child)!r}],check=True)\n')
            profiles = root / 'profiles'
            env = profile_environment(profiles, dict(os.environ, PYTHONPATH=''))
            result = subprocess.run([sys.executable, '-c', script], env=env,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            summary = summarize_profiles(profiles)
            import json
            detail = json.loads((profiles / 'summary.json').read_text())
            self.assertEqual(sum(r['calls'] for r in detail['functions']
                                 if r['function']=='measured_work'), 8)
            self.assertEqual(len(summary['processes']), 2)
            self.assertEqual(sum(summary['launches'].values()), 1)
