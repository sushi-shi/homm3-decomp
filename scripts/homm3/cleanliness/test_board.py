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
            return board.check_and_roll(write=True)

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
        rows, offenders = board.count(per_file=True)
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


class LocalClassTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "config/cleanliness").mkdir(parents=True)
        replacement = patch.object(board, "REPO", self.root)
        replacement.start()
        self.addCleanup(replacement.stop)

    def review(self, *rows):
        (self.root / "config/cleanliness/local_classes.tsv").write_text(
            "# reviewed\nfile\tclass\n" + "".join(f"{f}\t{c}\n" for f, c in rows))

    def test_reviewed_unique_local_class_is_admitted_only_in_its_file(self):
        path, other = self.root / "src/dialog.cpp", self.root / "src/other.cpp"
        source = "class LocalDialog { int value; };"
        self.review(("src/dialog.cpp", "LocalDialog"))
        self.assertEqual(board._reviewed_local_classes([(path, source)])[path], {"LocalDialog"})
        self.assertEqual(board._reviewed_local_classes([(other, source)])[other], set())

    def test_missing_inventory_admits_nothing(self):
        path = self.root / "src/dialog.cpp"
        self.assertEqual(board._reviewed_local_classes(
            [(path, "class LocalDialog {};")])[path], set())

    def test_unreviewed_duplicate_view_revokes_admission(self):
        path = self.root / "src/dialog.cpp"
        source = "class Outer { class Saved { int value; }; };"
        self.review(("src/dialog.cpp", "Outer"), ("src/dialog.cpp", "Saved"))
        self.assertEqual(board._reviewed_local_classes([(path, source)])[path], {"Outer", "Saved"})
        duplicate = (self.root / "include/duplicate.h", "class Saved { int other; };")
        self.assertNotIn("Saved", board._reviewed_local_classes([(path, source), duplicate])[path])
        twice = "class Saved { int a; }; class Saved { int b; };"
        self.assertEqual(board._reviewed_local_classes([(path, twice)])[path], set())

    def test_reviewed_private_copies_in_several_modules_are_distinct(self):
        first, second = self.root / "src/first.cpp", self.root / "src/second.cpp"
        source = "namespace { class TAutoStrPtr { char *p; }; }"
        self.review(("src/first.cpp", "TAutoStrPtr"), ("src/second.cpp", "TAutoStrPtr"))
        local = board._reviewed_local_classes([(first, source), (second, source)])
        self.assertEqual((local[first], local[second]), ({"TAutoStrPtr"}, {"TAutoStrPtr"}))

    def test_qualified_nested_class_uses_its_own_name(self):
        source = "class Reader::Failure { enum { CODE = 2 }; };"
        self.assertTrue(board._cpp_local_view_sites(source, {"local_classes": {"Reader"}}))
        self.assertEqual(board._cpp_local_view_sites(source, {"local_classes": {"Failure"}}), [])
        self.assertEqual(board._cpp_local_enum_sites(source, {"local_classes": {"Failure"}}), [])

    def test_enum_admission_is_limited_to_reviewed_class_member_scope(self):
        source = """class LocalDialog {
    enum { OK = 500 };
    void f() { enum { LOCAL = 501 }; }
    class Unproven { enum { NESTED = 502 }; };
};
enum Outside { OTHER = 503 };
"""
        self.assertEqual(len(board._cpp_local_enum_sites(source, {})), 4)
        sites = board._cpp_local_enum_sites(source, {"local_classes": {"LocalDialog"}})
        self.assertEqual(len(sites), 3)
        self.assertTrue(all(source.index("enum { OK") not in range(i, i + 12) for i in sites))


if __name__ == "__main__":
    unittest.main()
