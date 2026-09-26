"""Identity, freshness and inspection contracts for the full-TU helper target."""
from contextlib import redirect_stdout, redirect_stderr
from dataclasses import replace
import io
import json
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.mac import addresses, build, cc_wrap, emitted, pairs
from homm3.mac import __main__ as cli
from homm3.match.source_ownership import Definition


class TestIdentity(unittest.TestCase):
    def definition(self, spelling="int"):
        return Definition("src/foo.cpp", 2, 0, 1, "Foo::f", "int (int)", 1,
                          True, False, 0x400100, "", argument_types=(spelling,))

    def test_singletons_require_compatible_parameters_and_member_qualifiers(self):
        for spelling, symbol in [("int", ".f__3FooFl"), ("int *", ".f__3FooFPf"),
                                  ("const Foo &", ".f__3FooFR3Foo"),
                                  ("int", ".f__3FooCFi"),
                                  ("int (*)(int)", ".f__3FooFPFi_i"),
                                  ("volatile int *", ".f__3FooFPi")]:
            with self.subTest(spelling=spelling, symbol=symbol):
                definition = self.definition(spelling)
                self.assertIsNone(emitted.select(definition, [symbol]))
                self.assertIsNone(emitted.owner(symbol, [definition]))
        definition = self.definition()
        self.assertEqual(emitted.select(definition, [".f__3FooFi"]), ".f__3FooFi")
        self.assertIs(emitted.owner(".f__3FooFi", [definition]), definition)

    def test_non_emitted_overload_never_becomes_a_scored_pair(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "config").mkdir()
            (root / "config/units.toml").write_text('[[unit]]\nunit="foo"\nsource="src/foo.cpp"\n')
            (root / "src").mkdir()
            text = ("VA(0x00400100, 4) MAC_ADDRESS(0x100, 4)\nint Foo::f(int x) { return x; }\n"
                    "VA(0x00400200, 4) MAC_ADDRESS(0x200, 4)\nint Foo::f(long x) { return x; }\n")
            (root / "src/foo.cpp").write_text(text)
            claims = addresses.scan_text(text, "src/foo.cpp")[0]
            definitions = [self.definition(), replace(self.definition("long"), va=0x400200)]
            obj = root / "build/mac/obj"
            obj.mkdir(parents=True)
            (obj / "foo.o").write_bytes(b"MWOBPPC ")
            (obj / "foo.hunks.json").write_text(json.dumps({"code": [
                {"symbol": ".f__3FooFl", "size": 4, "references": []}]}))
            inventory = pairs.load(root, definitions=definitions, claims=claims)
            self.assertEqual([pair.retail_va for pair in inventory.pairs], [0x400200])
            self.assertEqual(inventory.symbols, {".f__3FooFl": 0x200})
            self.assertEqual(inventory.unscored[0]["retail_va"], "0x00400100")
            # Inspection still finds the non-emitted overload, independently of the join.
            with patch.object(pairs, "_definitions", return_value=definitions):
                self.assertEqual(pairs.select_claim(root, "0x00400100").mac_offset, 0x100)
            (obj / "foo.o").unlink()
            self.assertEqual(pairs._universe(root), set())


class TestRefresh(unittest.TestCase):
    def test_ninja_infrastructure_failure_withholds_comparison_and_checkpoint(self):
        failure = subprocess.CompletedProcess(["ninja"], 1, "", "ninja: error: loading build.ninja")
        with patch.object(build.subprocess, "run", return_value=failure), \
                patch.object(build, "_run") as score:
            with self.assertRaisesRegex(build.MacBuildError, "loading build.ninja"):
                build.run({"hero"}, checkpoint=True)
            score.assert_not_called()

    def compile(self, folder, result, allow=True, stage_error=None):
        root = Path(folder)
        out = root / "build/mac/obj/foo.o"
        out.parent.mkdir(parents=True, exist_ok=True)
        artifacts = [out, out.with_suffix(".dis.txt"), out.with_suffix(".hunks.json")]
        for path in artifacts:
            path.write_text("old output")
        with patch.object(cc_wrap, "ROOT", root), \
                patch("homm3.mac.toolchain.stage", return_value=root, side_effect=stage_error), \
                patch("homm3.mac.sdk.stage"), patch.object(cc_wrap, "_depfile"), \
                patch.object(cc_wrap, "_wine_env", return_value={}), \
                patch.object(cc_wrap, "flags_for", return_value=()), \
                patch.object(cc_wrap.subprocess, "run", return_value=result), \
                redirect_stderr(io.StringIO()):
            try:
                return cc_wrap.compile_unit("foo", root / "src/foo.cpp", out, allow_compile_errors=allow)
            finally:
                self.assertFalse(any(path.exists() for path in artifacts))

    def test_only_diagnosed_source_errors_can_be_nonfatal(self):
        for code, diagnostics, allow, expected in [
                (1, "#   Error: ^\n# undefined identifier", True, 0),
                (1, "#   Error: ^\n# undefined identifier", False, 1),
                (1, "wine: cannot start compiler", True, 1),
                (124, "#   Error: ^\ncompiler timeout", True, 1),
                (0, "no object produced", True, 1)]:
            with self.subTest(code=code, diagnostics=diagnostics, allow=allow), \
                    tempfile.TemporaryDirectory() as folder:
                result = subprocess.CompletedProcess([], code, diagnostics, "")
                self.assertEqual(self.compile(folder, result, allow), expected)

    def test_staged_input_failure_invalidates_old_outputs(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaisesRegex(ValueError, "wrong SDK"):
                self.compile(folder, None, stage_error=ValueError("wrong SDK"))


class TestInspection(unittest.TestCase):
    def target(self):
        return pairs.Pair(0x400100, "foo", cli.common.HOMM3_DIR / "src/foo.cpp", "Foo::f",
                          0, 0x100, 4, "", "va:0x00400100", 2)

    def test_show_without_an_object_does_not_compile(self):
        target = self.target()
        pef = SimpleNamespace(code=lambda *args: b"\x4e\x80\x00\x20")
        with patch.object(cli, "_select", return_value=target), \
                patch.object(cli, "_image", return_value=pef), \
                patch.object(build, "objects") as compile_units, redirect_stdout(io.StringIO()) as output:
            self.assertEqual(cli.main(["show", "0x00400100"]), 0)
            compile_units.assert_not_called()
            self.assertIn("Foo::f", output.getvalue())
            self.assertIn("0x100", output.getvalue())

    def test_compile_precedes_emitted_join_and_uses_refreshed_symbol(self):
        target = self.target()
        emitted_pair = replace(target, mac_symbol=".f__3FooFi")
        compiled = []
        def load(root):
            self.assertEqual(compiled, [{"foo"}])
            return pairs.Inventory((emitted_pair,), (), {}, {}, ())
        with patch.object(build, "objects", side_effect=compiled.append), \
                patch.object(pairs, "load", side_effect=load):
            result, _inventory = cli._compile_target(target)
            self.assertIs(result, emitted_pair)

    def test_mac_section_is_part_of_selector_identity(self):
        target = self.target()
        inventory = pairs.Inventory((target,), (), {}, {}, ())
        with self.assertRaises(pairs.PairError):
            pairs.select(inventory, "mac:1:0x100")
        self.assertIs(pairs.select(inventory, "mac:0:0x100"), target)

    def test_calls_refreshes_then_keeps_retail_when_candidate_is_unavailable(self):
        target = self.target()
        compiled = []
        def load(root):
            self.assertEqual(compiled, [{"foo"}])
            return pairs.Inventory((), (), {}, {}, ())
        pef = SimpleNamespace(code=lambda *args: b"\x4e\x80\x00\x20")
        with patch.object(pairs, "claimed", return_value=(target,)), \
                patch.object(build, "objects", side_effect=compiled.append), \
                patch.object(pairs, "load", side_effect=load), \
                patch.object(cli, "_image", return_value=pef), \
                patch.object(cli.symbols, "targets", return_value={}), \
                redirect_stdout(io.StringIO()) as output:
            self.assertEqual(cli.main(["calls", "foo", "--json"]), 0)
        report = json.loads(output.getvalue())
        self.assertEqual(report["totals"]["retail"]["functions"], 1)
        self.assertEqual(report["totals"]["candidate"]["functions"], 0)
        self.assertEqual(report["pairs"][0]["calls"]["state"], "candidate_unavailable")


if __name__ == "__main__":
    unittest.main()
