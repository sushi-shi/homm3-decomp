import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.cleanliness import board


class ContainerHelperMetrics(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "src").mkdir()
        (self.root / "include").mkdir()
        self.baseline = self.root / "baseline.tsv"
        for name, value in (("REPO", self.root), ("BASELINE", self.baseline)):
            replacement = patch.object(board, name, value)
            replacement.start()
            self.addCleanup(replacement.stop)

    def write_source(self, text):
        (self.root / "src/sample.cpp").write_text(text)

    def gate(self):
        with contextlib.redirect_stdout(io.StringIO()):
            return board.check_and_roll(write=True, dc_origins=[])

    def test_embedded_controls(self):
        self.assertEqual(board.selftest(), [])

    def test_counts_code_and_reports_each_site(self):
        self.write_source('''
// std::_MIN(a, b); values.insert(values.end(), value);
const char* description = "std::_Sort insert(end(), value)";
void append() {
    values.insert(
        values.end(), std::_cpp_min(a, b));
    values.insert(where, values.begin(), values.end());
    values.insert(values.end(), first, last);
}
''')
        (self.root / "include/helpers.h").write_text(
            "inline int helper() { return std :: _MAX(a, b); }\n")
        rows, offenders = board.count(per_file=True, dc_origins=[])
        counts = dict(rows)
        self.assertEqual(counts["end-position inserts"], 2)
        self.assertEqual(counts["std internal references"], 2)
        self.assertEqual(sum(label == "end-position inserts"
                             for label, _ in offenders), 2)
        self.assertIn(("end-position inserts", "src/sample.cpp:5: insert("),
                      offenders)

    def test_new_metrics_ratchet_and_reject_regrowth(self):
        self.write_source(
            "values.insert(values.end(), std::_cpp_min(a, b));\n")
        self.assertEqual(self.gate(), [])
        initial = board.load_baseline()
        self.assertEqual(initial["end-position inserts"], 1)
        self.assertEqual(initial["std internal references"], 1)
        self.write_source("values.push_back(min(a, b));\n")
        self.assertEqual(self.gate(), [])
        lowered = self.baseline.read_text()
        self.write_source(
            "values.insert(values.end(), std::_cpp_min(a, b));\n")
        violations = self.gate()
        self.assertTrue(any("end-position inserts rose 0 -> 1" in line
                            for line in violations))
        self.assertTrue(any("std internal references rose 0 -> 1" in line
                            for line in violations))
        self.assertEqual(self.baseline.read_text(), lowered)


if __name__ == "__main__":
    unittest.main()
