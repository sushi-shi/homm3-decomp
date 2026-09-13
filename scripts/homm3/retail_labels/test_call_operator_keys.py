"""Written predicate bodies bind directly; generated enrollments cannot hide them."""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm3.retail_labels import source


class CallOperatorKeysTest(unittest.TestCase):
    def test_equal_size_predicates_bind_by_written_owner(self):
        declarations = (
            "bool First::operator()(int left, int right) const",
            "bool Second::operator () (int left, int right)",
            "bool rules::Third::operator()(int left, int right) const",
        )
        symbols = (
            "??RFirst@@QBE_NHH@Z",
            "??RSecond@@QAE_NHH@Z",
            "??RThird@rules@@QBE_NHH@Z",
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "predicates.cpp"
            path.write_text("\n".join(
                f"VA(0x{0x401000 + i * 0x20:08x}, 0x20)\n{decl} {{ return false; }}"
                for i, decl in enumerate(declarations)))
            rows = source.scan_file(path, {0x1000, 0x1020, 0x1040})
        groups = {source._demangle_key(symbol): [(symbol, 0x20)]
                  for symbol in reversed(symbols)}
        with mock.patch.object(source, "_base_authority_scan",
                               return_value=(groups, {})):
            source.join_unit("predicates", rows)
        self.assertEqual([row.get("joined") for row in rows], list(symbols))
        self.assertTrue(all(row["channel"] == "src-VA+base" for row in rows))

    def test_unrelated_operators_and_template_owners_are_not_swallowed(self):
        for decl in (
            "bool Predicate::operator==(const Predicate& other) const",
            "bool Predicate::operator[](int index) const",
            "bool Predicate<int>::operator()(int value) const",
            "bool ns::Predicate<int>::operator()(int value) const",
        ):
            self.assertIsNone(source.CALL_OPERATOR_RE.search(decl), decl)
        self.assertIsNone(source._demangle_key("??R?$Predicate@H@@QBE_NH@Z"))

    def test_written_call_cannot_be_enrolled_as_compiler_generated(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "predicates.cpp"
            path.write_text("VA_COMPGEN(0x00401000, 0x20, FUNCTOR_CALL, First)\n")
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr), self.assertRaises(SystemExit):
                source.scan_file(path, {0x1000})
        self.assertIn("put VA on its operator() definition", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
