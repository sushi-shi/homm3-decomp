"""Access review controls: overloads, repeated NB11 types and incomplete audits."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.analysis import access_facts as facts


def method(access="private", parameters=(), family="ordinary", this_type="Widget *"):
    return {"display": "Widget::read", "kind": "method", "access": access,
            "parameters": list(parameters), "family": family, "this_type": this_type}


def authored(parameters=(), const=False):
    return {"class_key": "widget", "member_key": "read", "kind": "method",
            "parameters": list(parameters), "is_method": True, "const": const}


class AccessFactsTest(unittest.TestCase):
    def test_owning_alias_survives_access_label(self):
        source = "// Before normalization: OldField.\nprivate:\n    int m_newField;\n"
        self.assertEqual(facts._aliases(source, {"loc": {"offset": source.index("int ")}}),
                         ["OldField"])
        # Access review cannot invent a correspondence by stripping m_.
        field = {"class_key": "widget", "member_key": "mread", "kind": "member",
                 "is_method": False}
        dc = {("widget", "read"): [{"kind": "member", "access": "private"}]}
        self.assertEqual(facts.correlate(field, dc), (None, "name"))
        field["member_keys"] = ["mread", "read"]
        self.assertEqual(facts.correlate(field, dc)[0]["access"], "private")

    def test_shared_owning_keys_reject_neighbor_class_alias(self):
        source = ("// Before normalization: OldField.\n"
                  "// Before normalization (function): Other::unrelated.\n"
                  "private:\n    int m_newField;\n")
        self.assertEqual(facts.owning_member_keys(
            source, source.index("int "), "Widget", "m_newField"),
            ["mnewfield", "oldfield"])

    def test_access_formatter_preserves_nested_and_conditional_access(self):
        import importlib.util
        script = Path(__file__).resolve().parents[2] / "experiments/apply-access-adherence.py"
        spec = importlib.util.spec_from_file_location("access_transform", script)
        transform = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(transform)
        source = ("class Outer {\npublic:\n    int first;\n"
                  "public: // retain evidence\n    int second;\n"
                  "    class Inner {\n    private:\n        int secret;\n    };\n"
                  "public:\n    void method();\n#if FLAG\nprivate:\n    int a;\n"
                  "#else\npublic:\n    int b;\n#endif\npublic:\n    int last;\n"
                  "private:\n};\n")
        result = transform.remove_redundant_access_labels(source)
        self.assertIn("// retain evidence", result)
        self.assertIn("    private:\n        int secret;", result)
        self.assertIn("#else\npublic:\n    int b;\n#endif\npublic:", result)
        self.assertNotIn("private:\n};", result)
        self.assertNotIn("public:\n    void method", result)
        self.assertEqual(transform.remove_redundant_access_labels(result), result)

    def test_overloads_can_have_different_access(self):
        dc = {("widget", "read"): [method(), method("public", ["int"])]}
        self.assertEqual(facts.correlate(authored(), dc)[0]["access"], "private")
        self.assertEqual(facts.correlate(authored(["int"]), dc)[0]["access"], "public")
        # The retail-only one-argument overload must not inherit the DC
        # nullary member's private access (combatManager::isComputerAction).
        self.assertEqual(facts.correlate(authored(["int"]),
                                        {("widget", "read"): [method()]}),
                         (None, "signature"))

    def test_equal_arity_requires_types_and_cv(self):
        dc = {("widget", "read"): [method(parameters=["int"]),
              method("public", ["const Widget &"], this_type="const Widget *")]}
        self.assertEqual(facts.correlate(authored(["const Widget &"], True), dc)[0]["access"],
                         "public")
        self.assertEqual(facts.correlate(authored(["double"]), dc), (None, "ambiguous"))
        self.assertEqual(facts.correlate(authored(["const Widget &"]), dc), (None, "ambiguous"))
        dc = {("widget", "read"): [method(this_type="Widget * const"),
              method("public", this_type="const Widget * const")]}
        self.assertEqual(facts.correlate(authored(), dc)[0]["access"], "private")
        self.assertEqual(facts.correlate(authored(const=True), dc)[0]["access"], "public")

    def test_conflicting_records_do_not_vote_themselves_clean(self):
        dc = {("widget", "read"): [method(), method("public")]}
        self.assertEqual(facts.correlate(authored(), dc), (None, "ambiguous"))
        dc = {("widget", "read"): [method(family=None)]}
        self.assertIsNone(facts.correlate(authored(), dc)[0]["family"])

    def test_repeated_type_records_are_deduplicated(self):
        class Types:
            records = {1: None, 2: None}

            def get(self, index):
                return {1: {"kind": "class", "name": "Widget", "fields": 3},
                        2: {"kind": "class", "name": "Widget", "fields": 3},
                        4: {"kind": "function", "this": 5, "arguments": 6},
                        6: {"types": []}}.get(index, {})

            def fields(self, index):
                return [{"kind": "method", "name": "read", "type": 4,
                         "access": "private", "property": "ordinary"}]

            def declaration(self, index):
                return "Widget *"

        dc = facts.dc_visibility(Types())
        self.assertEqual(dc[("widget", "read")], [method()])

    def test_path_prefix_does_not_admit_neighbor_or_exclude_similar_name(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "repo"
            mirror = root / "mirror"
            self.assertFalse(facts.is_project_file(root.parent / "repo-other/a.h", root, mirror))
            self.assertFalse(facts.is_project_file(root / "vendor/a.h", root, mirror))
            self.assertTrue(facts.is_project_file(root / "vendor_interface/a.h", root, mirror))

    def test_exit_codes_distinguish_findings_from_coverage(self):
        clean = {"parse_failures": 0, "clang_errors": 0, "name_correlation_gaps": 0,
                 "signature_correlation_gaps": 0, "ambiguous_members": 0,
                 "unspecified_access": 0, "coverage_missing_dc_members": 0,
                 "access": {"mismatch": 0}, "property": {"mismatch": 0}}
        self.assertEqual(facts.exit_status(clean), 0)
        self.assertEqual(facts.exit_status({**clean, "access": {"mismatch": 1}}), 1)
        for key in ("parse_failures", "clang_errors", "signature_correlation_gaps",
                    "ambiguous_members", "name_correlation_gaps", "coverage_missing_dc_members"):
            self.assertEqual(facts.exit_status({**clean, key: 1}), 2)

    def test_json_and_text_both_fail_for_incomplete_parse(self):
        with patch.object(facts, "load_cindex"), \
                patch.object(facts.clang, "mirror", return_value=Path("/tmp/mirror")), \
                patch.object(facts.clang, "clang_bin", return_value="clang"), \
                patch.object(facts, "load_symbols"), \
                patch.object(facts.Types, "from_symbols"), \
                patch.object(facts, "dc_visibility", return_value={}), \
                patch.object(facts.compilation_database, "commands", return_value=[]), \
                patch.object(facts, "collect_authored", return_value=({}, {
                    "parsed": 0, "failures": [{"file": "bad.cpp", "error": "failed"}],
                    "diagnostics": []})):
            for arguments in ([], ["--json"]):
                with patch("sys.argv", ["verify-access"] + arguments), \
                        contextlib.redirect_stdout(io.StringIO()), \
                        contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(facts.main(), 2)


if __name__ == "__main__":
    unittest.main()
