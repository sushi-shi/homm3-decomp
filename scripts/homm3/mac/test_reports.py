"""Concurrent unit reports retain other observations with their original hashes."""
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import tempfile
import unittest

from homm3.mac.reports import publish


class TestMacReportPublication(unittest.TestCase):
    def test_concurrent_units_merge_and_failed_retry_removes_old_success(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "report.json"
            def write(index):
                unit = f"unit{index}"
                return publish(path, dict(
                    analysis_sha256=f"analysis{index}", target_sha256="exe",
                    pairs=[dict(retail_va=f"0x{index:08x}", unit=unit)]), units=[unit])
            with ThreadPoolExecutor(max_workers=6) as pool:
                list(pool.map(write, range(6)))
            merged = json.loads(path.read_text())
            self.assertEqual(len(merged["pairs"]), 6)
            for row in merged["pairs"]:
                expected = "analysis" + row["unit"].removeprefix("unit")
                self.assertEqual(row.get("analysis_sha256", merged["analysis_sha256"]), expected)
            failed = publish(path, dict(analysis_sha256="new", target_sha256="exe", pairs=[]),
                             units=["unit2"])
            self.assertEqual(len(failed["pairs"]), 5)
            self.assertNotIn("unit2", {row["unit"] for row in failed["pairs"]})
            full = publish(path, dict(analysis_sha256="full", pairs=[]))
            self.assertEqual(full["pairs"], [])


if __name__ == "__main__":
    unittest.main()
