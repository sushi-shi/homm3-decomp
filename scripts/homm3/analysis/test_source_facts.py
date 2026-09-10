"""Source-fact audit controls, including a real authored C++ AST fixture."""
from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.analysis import dreamcast, source_facts as facts


class TypeFactsTest(unittest.TestCase):
    def test_string_defaults_preserve_custom_arguments_and_cv_layers(self):
        full = "std::basic_string<char, std::char_traits<char>, std::allocator<char> >"
        for abbreviated in ("std::basic_string<char>",
                            "std::basic_string<char, std::char_traits<char> >"):
            self.assertEqual(facts.type_differences(full, abbreviated), ([], []))
            self.assertEqual(facts.type_differences("Box<" + full + ">",
                                                  "Box<" + abbreviated + ">"), ([], []))
        for different in ("std::basic_string<wchar_t>",
                          "std::basic_string<char, OtherTraits>",
                          "std::basic_string<char, std::char_traits<char>, OtherAllocator>",
                          "basic_string<char>"):
            self.assertIn("base-type", facts.type_differences(full, different)[0])
        self.assertEqual(facts.type_differences("const " + full + "&",
                                              "std::basic_string<char>*")[0],
                         ["qualifiers", "reference/pointer"])

    def test_cv_layers_references_arrays_and_template_qualifiers(self):
        for a, b in (("const T &", "T const&"), ("const int [8][18]", "int const[8][18]"),
                     ("std::vector<const T *> &", "std::vector< const T* >&")):
            self.assertEqual(facts.type_differences(a, b), ([], []))
        self.assertIn("qualifiers", facts.type_differences("const T*", "T* const")[0])
        self.assertIn("reference/pointer", facts.type_differences("T&", "T*")[0])
        self.assertIn("array-extent", facts.type_differences("const int[8][18]", "const int[9][18]")[0])
        self.assertEqual(facts.type_differences("T (*)(int)", "T (*)(int)")[0], [])
        self.assertTrue(facts.type_differences("T (*)(int)", "T (*)(int)")[1])

    def test_shadowed_and_missing_locals_are_unchecked_not_clean(self):
        expected = {"locals": [{"name": "x", "type": "const int"}]}
        for locals_ in ([], [{"name": "x", "type": "int"}] * 2):
            result = facts.compare_facts(expected, {"locals": locals_})
            self.assertTrue(result["coverage_gaps"])
            self.assertFalse(result["findings"])

    def test_order_uses_distinct_source_statements_not_argument_or_machine_order(self):
        dc = lambda name, line: {"name": name, "line": line, "file": "unit.cpp"}
        cpp = lambda name, offset, statement: {"name": name, "line": offset, "offset": offset, "statement": statement}
        expected = {"calls": [dc("b", 20), dc("a", 10)]}  # reversed machine address order
        candidate = {"calls": [cpp("a", 1, 1), cpp("b", 2, 2)]}
        self.assertFalse(facts.compare_facts(expected, candidate)["findings"])
        candidate["calls"].reverse()
        candidate["calls"][0]["offset"] = 0
        self.assertEqual(facts.compare_facts(expected, candidate)["findings"][0]["kind"], "source-order")
        candidate["calls"][0]["statement"] = 1
        self.assertFalse(facts.compare_facts(expected, candidate)["findings"])
        candidate["calls"][0]["statement"] = 2
        expected["calls"].append(dc("a", 30))
        self.assertFalse(facts.compare_facts(expected, candidate)["findings"])

    def test_return_reference_member_const_and_parameter_order(self):
        expected = {"return": "const T&", "method_cv": ["const"],
                    "parameters": [{"name": "a", "type": "int"}, {"name": "b", "type": "int"}]}
        candidate = {"return": "T*", "method_cv": [], "parameters": list(reversed(expected["parameters"]))}
        result = facts.compare_facts(expected, candidate)
        self.assertEqual({r["kind"] for r in result["findings"]}, {"type", "member-qualifiers", "parameter-order"})

    def test_formal_signature_survives_partial_optimized_parameter_inventory(self):
        types = SimpleNamespace(get=lambda i: {
            1: {"kind": "function", "arguments": 2, "returns": 4},
            2: {"types": [3, 4]}, 0: {"kind": "primitive"}}[i],
            declaration=lambda i: {3: "const int&", 4: "int"}[i])
        proc = SimpleNamespace(type_index=1, name="f", variables=[SimpleNamespace(kind="param", name="second")])
        dossier = SimpleNamespace(shape=SimpleNamespace(source_file="u.cpp", statements=[], locals=[], line_map=None))
        result = facts.expected_facts(dossier, proc, types)
        self.assertEqual(result["parameters"], [{"name": "", "type": "const int&"}, {"name": "", "type": "int"}])
        self.assertTrue(result["gaps"])

    def test_explicit_aliases_prevent_swapped_local_names_from_correlating(self):
        expected = {"locals": [{"name": "player", "type": "Player*"}, {"name": "iThisPlayer", "type": "int"}]}
        candidate = {"locals": [{"name": "thisPlayer", "type": "Player*", "aliases": ["player"]},
                                 {"name": "player", "type": "int", "aliases": ["iThisPlayer"]}]}
        result = facts.compare_facts(expected, candidate)
        self.assertFalse(result["findings"])
        self.assertFalse(result["coverage_gaps"])
        self.assertEqual(result["checked"]["local types"], 2)

    def test_minimal_dc_body_does_not_certify_the_authored_implementation(self):
        types = SimpleNamespace(get=lambda i: {1: {"kind": "function", "arguments": 2, "returns": 3},
                                               2: {"types": []}, 0: {"kind": "primitive"}}[i],
                                declaration=lambda i: "void")
        proc = SimpleNamespace(type_index=1, name="f", variables=[])
        shape = SimpleNamespace(source_file="u.cpp", statements=[], locals=[], line_map=SimpleNamespace(bodyless=True))
        result = facts.expected_facts(SimpleNamespace(shape=shape), proc, types)
        self.assertTrue(any("bodyless" in gap for gap in result["gaps"]))


@unittest.skipUnless(shutil.which("clang"), "Clang is required for authored AST integration")
class AuthoredAstTest(unittest.TestCase):
    def extract(self, source):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "unit.cpp"
            path.write_text(source)
            run = subprocess.run([shutil.which("clang"), "--target=i686-pc-windows-msvc", "-fsyntax-only",
                                  "-Xclang", "-ast-dump=json", "-Xclang", "-ast-dump-filter=Window::Window", str(path)],
                                 capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            docs = facts.json_documents(run.stdout)
            root = next(d for d in docs if any(c["kind"] == "CompoundStmt" for c in d.get("inner", [])))
            result = facts.extract_function(docs, root["mangledName"], path, source)
            with self.assertRaisesRegex(ValueError, "found 0"):
                facts.extract_function(docs, "missing", path, source)
            return result

    def test_const_arrays_and_flattened_helpers_negative_control(self):
        prefix = '''struct Widget { void setVisible(int); void sendMessage(int); };
struct Button { void setHotkey(int); void push(int); };
struct Text { const char* operator[](int); const char* getText(int); };
struct Timer { static int get(); };
struct Window { Window(Widget& w, Button& b, Text& t); };
Window::Window(Widget& w, Button& b, Text& t) {
'''
        before = prefix + '''int hallX[8][18], hallY[8][18], slotX[3], slotY[3];
w.sendMessage(0); b.push(1); t.getText(4); Timer::get();
}'''
        after = prefix + '''const int hallX[8][18] = {}, hallY[8][18] = {}, slotX[3] = {}, slotY[3] = {};
w.setVisible(0); b.setHotkey(1); t[4]; Timer::get();
}'''
        expected = {"parameters": [{"name": n, "type": t + "&"} for n, t in (("w", "Widget"), ("b", "Button"), ("t", "Text"))],
                    "locals": [{"name": n, "type": "const int" + dim} for n, dim in (("hallX", "[8][18]"), ("hallY", "[8][18]"), ("slotX", "[3]"), ("slotY", "[3]"))],
                    "calls": [{"name": n, "line": 10, "file": "unit.cpp"} for n in ("Widget::set_visible", "Button::set_hotkey", "Text::operator[]", "Timer::Get")]}
        failed = facts.compare_facts(expected, self.extract(before))
        self.assertFalse(failed["coverage_gaps"])
        self.assertEqual(sum(r["kind"] == "type" for r in failed["findings"]), 4)
        self.assertEqual(sum(r["kind"] == "helper" for r in failed["findings"]), 3)
        fixed = facts.compare_facts(expected, self.extract(after))
        self.assertFalse(fixed["findings"])
        self.assertFalse(fixed["coverage_gaps"])

    def test_clang_string_alias_uses_default_template_arguments(self):
        source = '''namespace std {
template<class T> struct char_traits {};
template<class T> struct allocator {};
template<class C, class T = char_traits<C>, class A = allocator<C> > struct basic_string {};
typedef basic_string<char> string;
}
struct OtherTraits {};
struct Window { Window(); };
Window::Window() {
 std::string result;
 std::basic_string<char, OtherTraits> custom;
}'''
        expected_type = "std::basic_string<char,std::char_traits<char>,std::allocator<char> >"
        candidate = self.extract(source)
        result = facts.compare_facts({"locals": [{"name": name, "type": expected_type}
                                                  for name in ("result", "custom")]}, candidate)
        self.assertFalse(result["coverage_gaps"])
        self.assertEqual(len(result["findings"]), 1)
        self.assertEqual(result["findings"][0]["subject"], "local custom")
        self.assertEqual(result["findings"][0]["aspects"], ["base-type"])

    def test_owning_alias_comment_matches_normalized_local(self):
        source = '''struct Window { Window(); };
Window::Window() {
 // Before normalization: bb.
 const int background = 1;
}'''
        result = facts.compare_facts({"locals": [{"name": "bb", "type": "const int"}]}, self.extract(source))
        self.assertFalse(result["findings"])
        self.assertFalse(result["coverage_gaps"])
        self.assertEqual(result["checked"]["local types"], 1)

    def test_standard_selectors_preserve_namespace_and_wrapper_boundary(self):
        prefix = '''namespace std {
template<class T> const T& _cpp_max(const T& a, const T& b) { return a < b ? b : a; }
template<class T> const T& _cpp_min(const T& a, const T& b) { return a < b ? a : b; }
}
int max(int a, int b) { return std::_cpp_max(a, b); }
int min(int a, int b) { return std::_cpp_min(a, b); }
struct Window { Window(int a, int b); };
Window::Window(int a, int b) {
'''
        expected = {"parameters": [{"name": n, "type": "int"} for n in ("a", "b")],
                    "calls": [{"name": "std::" + n, "file": "unit.cpp", "line": 10}
                              for n in ("min", "max")]}
        for calls in ("std::_cpp_min(a, b); std::_cpp_max(a, b);",
                      "::std::_cpp_min<int>(a, b); std::_cpp_max<int>(a, b);"):
            result = facts.compare_facts(expected, self.extract(prefix + calls + "\n}"))
            self.assertFalse(result["findings"])
            self.assertFalse(result["coverage_gaps"])
            self.assertEqual(result["checked"]["named helper groups"], 2)
        # Nested selector calls in the by-value wrapper do not satisfy the
        # caller's direct reference-selector boundary. Flattening also fails.
        for calls in ("min(a, b); max(a, b);", "int lo = a < b ? a : b; int hi = a < b ? b : a;"):
            result = facts.compare_facts(expected, self.extract(prefix + calls + "\n}"))
            self.assertEqual({row["subject"] for row in result["findings"]}, {"std::min", "std::max"})
            self.assertTrue(all(row["kind"] == "helper" for row in result["findings"]))

    def test_automatic_objects_supply_destructor_boundaries_but_heap_pointers_do_not(self):
        prefix = '''struct Guard { Guard(); ~Guard(); };
struct Window { Window(); };
Window::Window() {
'''
        expected = {"calls": [{"name": "Guard::~Guard", "line": 10, "file": "unit.cpp"}]}
        for declaration in ("Guard local;", "Guard();"):
            result = facts.compare_facts(expected, self.extract(prefix + declaration + "\n}"))
            self.assertFalse(result["findings"])
            self.assertEqual(result["checked"]["automatic destructor boundaries"], 1)
            self.assertNotIn("source-order pairs", result["checked"])
        for declaration in ("Guard* pointer = new Guard;", "static Guard local;"):
            result = facts.compare_facts(expected, self.extract(prefix + declaration + "\n}"))
            self.assertEqual(result["findings"][0]["kind"], "helper")


class CommandTest(unittest.TestCase):
    def test_audit_is_a_dispatched_command(self):
        dreamcast._redirect(["audit"])
        args = dreamcast._build_parser().parse_args(["audit", "--module", "townmgr", "--json"])
        self.assertEqual(args.module, "townmgr")
        self.assertTrue(args.json)

    def test_exit_codes_do_not_hide_gaps_or_findings(self):
        for findings, gaps, code in (([], [], 0), ([{}], [], 1), ([], ["parse failure"], 2)):
            row = {"findings": findings, "coverage_gaps": gaps}
            with patch("homm3.analysis.dc_lines.load_symbols"), patch("homm3.core.inputs.read_dreamcast_exe"), \
                 patch("homm3.core.nb11_types.Types.from_symbols"), patch.object(facts, "audit", return_value=row), \
                 contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(facts.run(None, [{}], as_json=True), code)
            self.assertEqual(json.loads(out.getvalue())["summary"]["coverage_gaps"], len(gaps))


if __name__ == "__main__":
    unittest.main()
