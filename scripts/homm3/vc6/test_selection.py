"""Selector equivalence and wrong-function controls for VC6 matching tools."""
from __future__ import annotations

import contextlib
import io
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.core import undname
from homm3.sema import _asm, context
from homm3.vc6 import _selection, _unit, diagnose, flow_model, inline_model, reg_model
from homm3.vc6.__main__ import _build_parser


FN = "?GetTeam@game@@QBEHH@Z"
OPEN = "?Open@Widget@@QAEXXZ"
LONG = "?Long@" + "X" * 260 + "@@QAEXXZ"
CSV = f"""rva,name,unit,size,kind,provenance
0x1000,{FN},game,0x20,func,src
0x1200,{OPEN},widget,0x10,func,src
0x1300,?Open@Other@@QAEXXZ,other,0x10,func,src
0x1400,?Duplicate@@YAXXZ,game,0x10,func,src
0x1500,?Duplicate@@YAXXZ,game,0x10,func,src
0x1600,unclaimed,seg_0001,0x10,func,working
0x1700,{LONG},game,0x10,func,src
0x2000,global_data,game,0x4,data,src
"""
FORMS = (
    "0x401000", "0x1000", "0X401000", "0x401003", FN,
    "game::GetTeam", "GetTeam", "game::GetTeam(int) const",
    "game:0x401000", "game:0x1000", f"game:{FN}",
    "game:game::GetTeam", "game:GetTeam",
)
SOLVERS = ("predict-inline", "why-reg", "why-branch")


@unittest.skipUnless(undname.available(), "llvm-undname not on PATH")
class SelectionTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.sources = {}
        for unit in ("game", "widget", "other"):
            src = self.root / f"{unit}.cpp"
            src.write_text(f"// {unit}\n")
            self.sources[unit] = src
        csv = self.root / "symbols.csv"
        csv.write_text(CSV)
        self.patch(context, "SYMCSV", csv)
        self.db = context.SymbolDb(extent_of=lambda: 0x300000)
        self.patch(_selection, "get_context", return_value=SimpleNamespace(
            symbols=self.db, image=SimpleNamespace(image_size=0x300000)))
        self.patch(_unit, "source_for_unit", side_effect=self.sources.get)
        self.patch(_unit, "unit_for_source", side_effect=lambda path: next(
            (u for u, src in self.sources.items() if src == Path(path).resolve()), None))
        self.patch(_asm, "TARGET", self.root / "target")
        self.patch(_asm, "_function_text_symbols", return_value={FN})
        self.parser = _build_parser()
        self.out, self.err = io.StringIO(), io.StringIO()
        self.enterContext(contextlib.redirect_stdout(self.out))
        self.enterContext(contextlib.redirect_stderr(self.err))

    def patch(self, obj, name, *args, **kwargs):
        p = patch.object(obj, name, *args, **kwargs)
        self.addCleanup(p.stop)
        return p.start()

    def test_all_selector_forms_infer_the_same_function_for_every_solver(self):
        for command in SOLVERS:
            for selector in FORMS:
                with self.subTest(command=command, selector=selector):
                    args = self.parser.parse_args([command, selector])
                    _selection.prepare(args)
                    self.assertEqual(Path(args.src), self.sources["game"])
                    self.assertEqual(args.fn, FN)
                    self.assertEqual(args.against, "game:0x1000")
                    self.assertEqual(args._fn_ordinal, 0)
        self.assertEqual(self.out.getvalue(), "")

    def test_legacy_source_and_fn_accept_every_selector_for_every_solver(self):
        for command in SOLVERS:
            for selector in FORMS:
                with self.subTest(command=command, selector=selector):
                    args = self.parser.parse_args(
                        [command, str(self.sources["game"]), "--fn", selector])
                    _selection.prepare(args)
                    self.assertEqual(Path(args.src), self.sources["game"])
                    self.assertEqual(_selection.object_symbol(Path("game.obj"), args.fn), FN)
                    self.assertTrue(args.against.startswith("game:"))

    def test_diagnose_accepts_every_selector_including_cpp_scopes(self):
        for selector in FORMS:
            with self.subTest(selector=selector):
                unit, fn, src = diagnose._resolve(selector)
                self.assertEqual((unit, src), ("game", self.sources["game"]))
                self.assertEqual(_selection.retail(fn, unit).name, FN)

    def test_fn_without_a_source_can_infer_it(self):
        args = self.parser.parse_args(["why-reg", "--fn", "0x401000"])
        _selection.prepare(args)
        self.assertEqual(Path(args.src), self.sources["game"])

    def test_long_symbols_are_not_interpreted_as_filesystem_paths(self):
        args = self.parser.parse_args(["predict-inline", LONG])
        _selection.prepare(args)
        self.assertEqual(args.fn, LONG)

    def test_unit_with_fn_infers_the_source_for_every_solver(self):
        for tool in SOLVERS:
            with self.subTest(tool=tool):
                args = self.parser.parse_args([tool, "game", "--fn", "0x401000"])
                _selection.prepare(args)
                self.assertEqual(Path(args.src), self.sources["game"])
                self.assertEqual(args.fn, FN)

    def test_unit_scope_disambiguates_a_bare_method(self):
        selected = _selection.retail("widget:Open")
        self.assertEqual((selected.name, selected.unit), (OPEN, "widget"))
        with self.assertRaises(SystemExit) as stop:
            _selection.retail("Open")
        self.assertEqual(stop.exception.code, 2)
        self.assertIn("ambiguous", self.err.getvalue())

    def test_invalid_selectors_do_not_choose_an_unrelated_body(self):
        for selector in ("0xzz", "0x900000", "0x2000", "0x1190", "NoSuchName",
                         "unknown:GetTeam", "widget:0x401000", "dc:0x1000",
                         "game.obj:0x1000"):
            with self.subTest(selector=selector), self.assertRaises(SystemExit) as stop:
                _selection.prepare(self.parser.parse_args(["predict-inline", selector]))
            self.assertEqual(stop.exception.code, 2)

    def test_unclaimed_function_does_not_infer_a_nonexistent_source(self):
        with self.assertRaises(SystemExit) as stop:
            _selection.prepare(self.parser.parse_args(["predict-inline", "0x401600"]))
        self.assertEqual(stop.exception.code, 2)
        self.assertIn("no manifest source", self.err.getvalue())

    def test_explicit_source_and_address_must_agree(self):
        for fn in ("0x401200", "widget:Open"):
            args = self.parser.parse_args(
                ["predict-inline", str(self.sources["game"]), "--fn", fn])
            with self.subTest(fn=fn), self.assertRaises(SystemExit):
                _selection.prepare(args)

    def test_source_path_requires_fn_and_missing_source_is_reported(self):
        for argv in (["predict-inline", str(self.sources["game"])],
                     ["predict-inline"],
                     ["predict-inline", str(self.root / "missing.cpp"), "--fn", FN]):
            with self.subTest(argv=argv), self.assertRaises(SystemExit):
                _selection.prepare(self.parser.parse_args(argv))

    def test_scratch_source_requires_an_explicit_reference(self):
        src = self.root / "scratch.cpp"
        src.write_text("// experiment\n")
        with self.assertRaises(SystemExit):
            _selection.prepare(self.parser.parse_args(
                ["predict-inline", str(src), "--fn", "GetTeam"]))

    def test_predict_refreshes_before_reading_the_manifest_object(self):
        args = self.parser.parse_args(["predict-inline", "GetTeam", "--json"])
        events = []
        self.patch(_asm, "refresh_unit", side_effect=lambda unit: events.append("refresh") or "rebuilt")
        self.patch(_unit, "base_obj", side_effect=lambda unit: events.append("object") or Path("base.obj"))
        self.patch(reg_model, "_fn_text", side_effect=RuntimeError("stop after refresh"))
        with self.assertRaisesRegex(RuntimeError, "stop after refresh"):
            inline_model.run_predict(args)
        self.assertEqual(events, ["refresh", "object"])
        self.assertIn("rebuilt", self.err.getvalue())

    def test_predict_never_diagnoses_an_old_object_after_a_failed_refresh(self):
        args = self.parser.parse_args(["predict-inline", "GetTeam"])
        self.patch(_asm, "refresh_unit", side_effect=SystemExit(2))
        read = self.patch(reg_model, "_fn_text")
        with self.assertRaises(SystemExit):
            inline_model.run_predict(args)
        read.assert_not_called()

    def test_predict_can_explicitly_inspect_the_last_built_object(self):
        args = self.parser.parse_args(["predict-inline", "GetTeam", "--no-build"])
        refresh = self.patch(_asm, "refresh_unit")
        self.patch(_unit, "base_obj", return_value=Path("base.obj"))
        self.patch(reg_model, "_fn_text", side_effect=RuntimeError("read object"))
        with self.assertRaisesRegex(RuntimeError, "read object"):
            inline_model.run_predict(args)
        refresh.assert_not_called()

    def test_explicit_reference_and_source_overrides_are_preserved(self):
        src = self.root / "scratch.cpp"
        src.write_text("// experiment\n")
        for flag, value in (("--against", "widget:0x1200"),
                            ("--against", "Widget::Open"),
                            ("--against-src", str(self.sources["other"]))):
            args = self.parser.parse_args(
                ["predict-inline", str(src), "--fn", FN, flag, value])
            _selection.prepare(args)
            self.assertEqual(args.src, str(src))
            self.assertEqual(getattr(args, flag[2:].replace("-", "_")), value)

    def test_candidate_profile_comes_from_explicit_source(self):
        args = self.parser.parse_args(["why-reg", str(self.sources["game"]),
                                      "--fn", FN, "--against", "widget:Open"])
        _selection.prepare(args)
        self.assertEqual(_selection.reference_unit(args), "game")

    def test_explicit_scratch_source_can_compare_against_unclaimed_retail(self):
        src = self.root / "scratch.cpp"
        src.write_text("// experiment\n")
        args = self.parser.parse_args(["why-reg", str(src), "--fn", "probe",
                                      "--against", "0x401600"])
        _selection.prepare(args)
        self.patch(_unit, "flags_for_unit", return_value=None)
        self.assertIsNone(_selection.reference_unit(args))
        image = self.patch(_asm, "image_text", return_value="retail asm")
        text, label = _selection.reference_text(args.against)
        self.assertEqual(text, "retail asm")
        self.assertEqual(image.call_args.args[1:], (0x1600, 0x10, "unclaimed"))
        for selector in ("seg_0001:0x401600", "seg_0001:unclaimed"):
            args.against = selector
            self.assertIsNone(_selection.reference_unit(args))
            self.assertEqual(_selection.reference_text(selector)[0], "retail asm")

    def test_compiled_reference_keeps_the_original_source_selector(self):
        # An explicit source experiment may change a signature. Selecting its
        # reference by the candidate's mangled symbol would wrongly reject it.
        args = self.parser.parse_args([
            "why-reg", str(self.sources["game"]), "--fn", "GetTeam",
            "--against-src", str(self.sources["other"])])
        _selection.prepare(args)
        args.fn = FN
        from homm3.vc6 import _solver
        for module in (reg_model, flow_model):
            with self.subTest(module=module.__name__), \
                    patch.object(_solver, "_compile_tu", return_value=(Path("ref.obj"), "")) as compile_tu, \
                    patch.object(_solver, "_wine_dir", return_value=None), \
                    patch.object(_solver, "_fn_text", return_value=("asm", "other symbol")) as fn_text:
                module._reference_side(args, module.SCRATCH)
                self.assertEqual(fn_text.call_args.args[1], "GetTeam")
                self.assertEqual(compile_tu.call_args.args[1], module.SCRATCH / "ref")

    def test_scratch_override_is_compiled_not_replaced_by_the_tree_object(self):
        src = self.root / "scratch.cpp"
        src.write_text("// experiment\n")
        args = self.parser.parse_args(["predict-inline", str(src), "--fn", FN,
                                      "--against", "game:GetTeam"])
        self.patch(_unit, "base_obj", return_value=Path("existing.obj"))
        compile_text = self.patch(_unit, "compile_text", side_effect=RuntimeError("compiled"))
        with self.assertRaisesRegex(RuntimeError, "compiled"):
            inline_model.run_predict(args)
        self.assertEqual(compile_text.call_args.args[:2], (src.read_text(), "game"))

    def test_branch_solver_uses_the_inferred_unit_profile(self):
        self.patch(flow_model, "SCRATCH", self.root / "whybranch")
        compile_text = self.patch(_unit, "compile_text", side_effect=RuntimeError("compiled"))
        args = self.parser.parse_args(["why-branch", "0x401000"])
        with self.assertRaisesRegex(RuntimeError, "compiled"):
            flow_model.run_why(args)
        self.assertEqual(compile_text.call_args.args[:2],
                         (self.sources["game"].read_text(), "game"))

    def test_duplicate_symbol_address_preserves_both_ordinals(self):
        args = self.parser.parse_args(["predict-inline", "0x401500"])
        _selection.prepare(args)
        self.assertEqual(args._fn_ordinal, 1)
        self.assertEqual(args.against, "game:0x1500")
        dump = self.patch(_asm, "objdump", return_value="asm")
        reg_model._fn_text(Path("game.obj"), FN, args._fn_ordinal)
        self.assertEqual(dump.call_args.args[-1], 1)

    def test_reference_overrides_accept_every_selector_form(self):
        dump = self.patch(_asm, "objdump", return_value="asm")
        _asm.TARGET.mkdir()
        (_asm.TARGET / "game.c.obj").touch()
        for selector in FORMS:
            with self.subTest(selector=selector):
                self.assertEqual(_selection.reference_text(selector),
                                 ("asm", f"delinked game.c.obj ({FN})"))
                self.assertEqual(dump.call_args.args[1:], (FN, 0))
        self.assertEqual(self.out.getvalue(), "")

    def test_object_only_symbols_work_without_a_retail_ledger(self):
        self.patch(_selection, "get_context", side_effect=AssertionError("retail read"))
        for resolver in (reg_model._resolve_symbol, flow_model._resolve_symbol):
            self.assertEqual(resolver(Path("probe.obj"), "game::GetTeam"), FN)

    def test_ambiguous_overloads_and_missing_decorated_symbols_are_refused(self):
        self.patch(_asm, "_function_text_symbols", return_value={
            "?Open@Widget@@QAEXXZ", "?Open@Widget@@QAEXH@Z"})
        for fn in ("Widget::Open", "Open", "?Open@Widget@@QAEXD@Z"):
            with self.subTest(fn=fn), self.assertRaises(SystemExit):
                _selection.object_symbol(Path("widget.obj"), fn)


if __name__ == "__main__":
    unittest.main()
