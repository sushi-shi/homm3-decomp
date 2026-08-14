#!/usr/bin/env python3
"""Hermetic tests for the solver body locator (homm3.vc6._source).

Run: `python3 -m homm3.vc6.test_locator` (rc != 0 on any failure); also
wired as the `locator` gate of `homm3 vc6 check`.

Every POSITIVE case is a shape the locator must FIND (these are the shapes
whose absence kept `why-reg` / `why-branch` off the tree's whole
constructor and member-function population). Every NEGATIVE case is a shape
it must REFUSE - the repo's negative-control rule: a gate that cannot fail
proves nothing, and the two defects fixed here were both *false positives*
in disguise:

  * the `#if 0  // @carcass` stub (measured: `sum_mobility` located to the
    stub at line 155, not the real body at line 1269) - the old locator
    happily returned a `{ /* @stub */ }` body, so every mutation was a
    no-op and the solver reported a CAPPED verdict it had not measured;
  * a declaration or a call site (`new Foo(...)`) mistaken for a definition.

If either negative control ever starts passing as "located", this gate goes
red.
"""
from __future__ import annotations

import unittest

from homm3.vc6 import _source


def _body(text: str, fn: str) -> str | None:
    span = _source.body_span(text, fn)
    return None if span is None else text[span[0]:span[1] + 1]


class DemangleTest(unittest.TestCase):
    def test_constructor(self):
        self.assertEqual(
            _source.source_names("??0TBottomViewKingdom@@QAE@PAVheroWindow@@@Z"),
            ["TBottomViewKingdom::TBottomViewKingdom", "TBottomViewKingdom"])

    def test_destructor(self):
        self.assertEqual(
            _source.source_names("??1type_bottom_view_window@@UAE@XZ"),
            ["type_bottom_view_window::~type_bottom_view_window",
             "~type_bottom_view_window"])

    def test_member_function(self):
        self.assertEqual(
            _source.source_names("?animate@TBottomViewNewTurn@@UAEXXZ"),
            ["TBottomViewNewTurn::animate", "animate"])

    def test_free_function(self):
        self.assertEqual(_source.source_names("?format_string@@YAXXZ"),
                         ["format_string"])

    def test_nested_scope_is_offered_both_ways(self):
        self.assertEqual(_source.source_names("?foo@Inner@Outer@@QAEXXZ"),
                         ["Outer::Inner::foo", "Inner::foo", "foo"])

    def test_operator(self):
        self.assertEqual(_source.source_names("??4Point@@QAEAAV0@ABV0@@Z"),
                         ["Point::operator=", "operator="])

    def test_undecorated_name_passes_through(self):
        self.assertEqual(_source.source_names("pump"), ["pump"])

    # --- negative controls: names with NO source body ------------------------------
    def test_scalar_deleting_dtor_is_reported_compiler_generated(self):
        m = _source.demangle("??_GTBottomViewKingdom@@UAEPAXI@Z")
        self.assertEqual(m.compgen, "scalar deleting destructor")
        self.assertEqual(_source.source_names("??_GTBottomViewKingdom@@UAEPAXI@Z"),
                         [])

    def test_vftable_is_reported_compiler_generated(self):
        self.assertEqual(_source.demangle("??_7Foo@@6B@").compgen, "vftable")

    def test_template_id_scope_is_declined_not_guessed(self):
        m = _source.demangle("??0?$vector@PAVwidget@@@std@@QAE@XZ")
        self.assertIsNotNone(m.note)
        self.assertEqual(_source.source_names("??0?$vector@PAVwidget@@@std@@QAE@XZ"),
                         [])


class BodyLocationTest(unittest.TestCase):
    def test_qualified_member_definition(self):
        src = "int Foo::bar(int a)\n{\n    return a;\n}\n"
        self.assertEqual(_body(src, "?bar@Foo@@QAEHH@Z"),
                         "{\n    return a;\n}")

    def test_constructor_with_member_initialiser_list(self):
        src = ("Foo::Foo(int a)\n"
               "    : Base(a), m_x(a), m_y(0)\n"
               "{\n    hit();\n}\n")
        self.assertEqual(_body(src, "??0Foo@@QAE@H@Z"), "{\n    hit();\n}")

    def test_initialiser_list_with_nested_parens_and_comments(self):
        src = ("Foo::Foo(int a)\n"
               "    : Base(f(a, g(1))),  // note ) and { in a comment\n"
               "      m_s(\"a){b\")\n"
               "{\n    hit();\n}\n")
        self.assertEqual(_body(src, "??0Foo@@QAE@H@Z"), "{\n    hit();\n}")

    def test_destructor(self):
        src = "Foo::~Foo()\n{\n    drop();\n}\n"
        self.assertEqual(_body(src, "??1Foo@@UAE@XZ"), "{\n    drop();\n}")

    def test_operator_definition(self):
        src = "Foo& Foo::operator=(const Foo& o)\n{\n    return *this;\n}\n"
        self.assertEqual(_body(src, "??4Foo@@QAEAAV0@ABV0@@Z"),
                         "{\n    return *this;\n}")

    def test_const_and_throw_suffixes(self):
        src = "int Foo::bar(int a) const throw()\n{\n    return a;\n}\n"
        self.assertEqual(_body(src, "?bar@Foo@@QBEHH@Z"), "{\n    return a;\n}")

    def test_parameters_come_from_the_parameter_list_not_the_init_list(self):
        src = ("Foo::Foo(heroWindow* parent, int res)\n"
               "    : Base(parent)\n"
               "{\n}\n")
        self.assertEqual(_source.params(src, "??0Foo@@QAE@PAVheroWindow@@H@Z"),
                         ["parent", "res"])

    def test_qualified_definition_wins_over_a_same_named_free_function(self):
        src = ("void bar()\n{\n    free_one();\n}\n\n"
               "void Foo::bar()\n{\n    member_one();\n}\n")
        self.assertEqual(_body(src, "?bar@Foo@@QAEXXZ"),
                         "{\n    member_one();\n}")

    # --- NEGATIVE CONTROLS ---------------------------------------------------------
    def test_carcass_stub_is_refused_and_the_real_body_wins(self):
        """The defect that made solver verdicts silently wrong.

        The tree fences unreconstructed rows in `#if 0  // @carcass` near
        the TOP of the file, so a first-match locator returns the stub. If
        this test ever returns the stub again, every solver run on such a
        row is meaningless.
        """
        src = ("#if 0  // @carcass\n"
               "void Foo::bar()\n{\n    // @stub\n}\n"
               "#endif  // @carcass\n\n"
               "void Foo::bar()\n{\n    real();\n}\n")
        self.assertEqual(_body(src, "?bar@Foo@@QAEXXZ"), "{\n    real();\n}")

    def test_carcass_only_row_reports_fenced_not_located(self):
        src = ("#if 0  // @carcass\n"
               "void Foo::bar()\n{\n    // @stub\n}\n"
               "#endif  // @carcass\n")
        self.assertIsNone(_source.body_span(src, "?bar@Foo@@QAEXXZ"))
        self.assertIn("INACTIVE", _source.explain_miss(src, "?bar@Foo@@QAEXXZ"))

    def test_nested_conditional_does_not_end_the_carcass_early(self):
        src = ("#if 0  // @carcass\n"
               "#ifdef WIN32\n"
               "void Foo::bar()\n{\n    // @stub\n}\n"
               "#endif\n"
               "#endif  // @carcass\n"
               "void Foo::bar()\n{\n    real();\n}\n")
        self.assertEqual(_body(src, "?bar@Foo@@QAEXXZ"), "{\n    real();\n}")

    def test_if_one_else_branch_is_dead(self):
        src = ("#if 1\n"
               "void Foo::bar()\n{\n    real();\n}\n"
               "#else\n"
               "void Foo::bar()\n{\n    dead();\n}\n"
               "#endif\n")
        self.assertEqual(_body(src, "?bar@Foo@@QAEXXZ"), "{\n    real();\n}")

    def test_declaration_is_not_a_definition(self):
        src = "class Foo { void bar(int a); };\nvoid other() { }\n"
        self.assertIsNone(_source.body_span(src, "?bar@Foo@@QAEXH@Z"))

    def test_call_site_is_not_a_definition(self):
        src = ("void mk()\n{\n    Widgets.push_back(new Foo(1, 2));\n}\n")
        self.assertIsNone(_source.body_span(src, "??0Foo@@QAE@HH@Z"))

    def test_definition_inside_a_comment_is_not_found(self):
        src = ("/*\nvoid Foo::bar()\n{\n    commented();\n}\n*/\n"
               "void keep() { }\n")
        self.assertIsNone(_source.body_span(src, "?bar@Foo@@QAEXXZ"))

    def test_brace_carrying_macro_does_not_capture_the_body(self):
        src = ("#define GUARD(x) if (x) { fail(); }\n"
               "void Foo::bar()\n{\n    real();\n}\n")
        self.assertEqual(_body(src, "?bar@Foo@@QAEXXZ"), "{\n    real();\n}")

    def test_compiler_generated_row_is_not_located(self):
        src = "VA_COMPGEN(0x004531a0, 0x21, SCALAR_DELETING_DTOR, Foo)\n"
        self.assertIsNone(_source.body_span(src, "??_GFoo@@UAEPAXI@Z"))
        self.assertIn("compiler-generated",
                      _source.explain_miss(src, "??_GFoo@@UAEPAXI@Z"))


class MaskTest(unittest.TestCase):
    def test_mask_is_length_preserving(self):
        src = ("#if 0\nvoid dead() { \"a\" }\n#endif\n"
               "// c\nvoid live() { 'x'; /* b */ }\n")
        self.assertEqual(len(_source.mask(src)), len(src))

    def test_mask_keeps_active_code(self):
        src = "#if 0\nvoid dead() { }\n#endif\nvoid live() { }\n"
        self.assertIn("void live() { }", _source.mask(src))
        self.assertNotIn("dead", _source.mask(src))


if __name__ == "__main__":
    unittest.main()
