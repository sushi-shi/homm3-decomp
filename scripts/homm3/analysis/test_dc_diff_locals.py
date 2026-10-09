"""Hermetic tests for `homm3 dreamcast diff-locals` and shared callee keys."""
from __future__ import annotations

from collections import Counter
import contextlib
import io
from pathlib import Path
import shutil
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.analysis import dc_callees, dc_diff_locals as diff, dreamcast, source_facts


class CalleeKeyTest(unittest.TestCase):
    def test_platform_spellings_reduce_to_one_key(self):
        spellings = ("searchArray::limit_was_reached", "?limitWasReached@searchArray@@QAE_NXZ",
                     ".limitWasReached__11searchArrayFv", "searchArray::limitWasReached")
        self.assertEqual({dc_callees.callee(name).key for name in spellings}, {"limitwasreached"})

    def test_constructors_destructors_and_operators(self):
        for name in ("type_point::type_point", "??0type_point@@QAE@HH@Z",
                     ".__ct__10type_pointFii", "CAutoArray<X>::CAutoArray<X>"):
            identity = dc_callees.callee(name)
            self.assertEqual(identity.kind, "constructor", name)
        self.assertEqual(dc_callees.callee(".__ct__Q24font9TFontSpecFv").key, "tfontspec")
        self.assertEqual(dc_callees.callee("??1TTextResource@@QAE@XZ").key, "~ttextresource")
        self.assertEqual(dc_callees.callee(".__dt__4fontFv").kind, "destructor")
        self.assertEqual(dc_callees.callee("??4Foo@@QAEAAV0@ABV0@@Z").key, "operator=")
        self.assertEqual(dc_callees.callee(".__eq__10type_pointCFRC10type_point").key, "operator==")
        self.assertEqual(dc_callees.callee("gzseek@12").key, "gzseek")

    def test_plumbing_is_excluded(self):
        for name in (".__nw__FUl", ".__dla__FPv", "??2@YAPAXI@Z", "??_GFoo@@UAEPAXI@Z",
                     "operator new", "std::vector<int,std::allocator<int> >::size",
                     ".__vc__Q23std6vectorFi", "__modls", "_sin", "memset", ".strcat",
                     ".mac_copy_lowercase_ascii_26afe8", None, "mac:0:0x1234"):
            self.assertIsNone(dc_callees.callee(name), name)

    def test_inline_traces_index_candidate_definitions(self):
        groups = [{"source": r"E:\gamedcs\hero.h", "definition_line": 157, "source_lines": [157, 158],
                   "address": 0x10, "end_address": 0x14, "confidence": "positive",
                   "definitions": ["type_obscuring_object::get_location"]}]
        traces = dc_callees.inline_traces(groups)
        self.assertEqual(list(traces), ["getlocation"])
        self.assertIn("hero.h:157 at 1 site(s)", dc_callees.render_traces(traces["getlocation"])[0])


class InventoryTest(unittest.TestCase):
    def test_locals_pair_by_name_alias_then_convention(self):
        dc = [{"name": "pCurrentHero", "type": "hero *"}, {"name": "iCount", "type": "int"},
              {"name": "oldName", "type": "int"}, {"name": "unused", "type": "int"}]
        ours = [{"name": "currentHero", "type": "hero*"}, {"name": "count", "type": "int"},
                {"name": "renamed", "type": "int", "aliases": ["oldName"]},
                {"name": "extra", "type": "int"}]
        ours_only, dc_only, matched = diff.diff_locals(dc, ours)
        self.assertEqual([row["name"] for row in ours_only], ["extra"])
        self.assertEqual([row["name"] for row in dc_only], ["unused"])
        self.assertEqual(len(matched), 3)

    def test_shadowed_locals_pair_once(self):
        ours_only, dc_only, _ = diff.diff_locals(
            [{"name": "i"}], [{"name": "i"}, {"name": "i"}])
        self.assertEqual(len(ours_only), 1)
        self.assertFalse(dc_only)

    def _dossier(self, rows):
        statements = [SimpleNamespace(address=address, source_file=source, source_line=line,
                                      calls=[SimpleNamespace(name=name) for name in names])
                      for address, source, line, names in rows]
        return SimpleNamespace(shape=SimpleNamespace(source_file=r"E:\gamedcs\unit.cpp",
                                                     statements=statements))

    def test_dc_inventory_keeps_header_rows_and_finds_statement_temporaries(self):
        dossier = self._dossier([
            (0x10, r"E:\gamedcs\unit.cpp", 5, ["TText::TText", "Show", "TText::~TText"]),
            (0x20, r"E:\gamedcs\game.h", 9, ["town::HasGarrison", "__modls"]),
        ])
        clues = [{"address": 0x20, "source": r"E:\gamedcs\game.h", "line": 9,
                  "confidence": "positive", "definitions": ["game::GetTown"]}]
        inventory = diff.dc_inventory(dossier, clues)
        self.assertEqual([call["key"] for call in inventory["calls"]],
                         ["ttext", "show", "~ttext", "hasgarrison"])
        self.assertEqual(inventory["calls"][-1]["inline_row"], ["game::GetTown"])
        self.assertEqual(inventory["temporaries"], Counter({"ttext": 1}))

    def test_call_diff_counts_and_inline_traces(self):
        dc = [{"name": "a::Get", "key": "get", "kind": "call", "line": 3, "file": "u.cpp"},
              {"name": "a::~a", "key": "~a", "kind": "destructor", "line": 3, "file": "u.cpp"},
              {"name": "Only", "key": "only", "kind": "call", "line": 4, "file": "u.cpp",
               "inline_row": ["h::Helper"]}]
        ours = diff.our_calls({"calls": [
            {"name": "a::get", "line": 7}, {"name": "a::get", "line": 8},
            {"name": "b::isFlying", "line": 9}, {"name": "a::a", "line": 9, "copy": True}]})
        traces = {"isflying": [{"source": "hero.h", "definition_line": 641, "lines": [641],
                                "dc_start": 0, "dc_end": 2, "confidence": "positive",
                                "definitions": ["hero::IsFlying"]}]}
        ours_only, dc_only = diff.diff_calls(dc, ours, traces)
        self.assertEqual([(c["key"], c["ours"], c["dreamcast"]) for c in ours_only],
                         [("isflying", 1, 0), ("get", 2, 1)])
        self.assertEqual(ours_only[0]["dc_inline"], traces["isflying"])
        self.assertEqual([(c["key"], c["dc_inline_rows"]) for c in dc_only],
                         [("only", ["h::Helper"])])

    def test_temporaries_compare_destructor_carrying_types(self):
        candidate = {"temporaries": [{"type": "TText", "line": 4, "destructor": True},
                                     {"type": "type_point", "line": 5, "destructor": False}]}
        ours_only, dc_only = diff.diff_temporaries(Counter({"string": 1}), candidate)
        self.assertEqual([row["type"] for row in ours_only], ["ttext"])
        self.assertEqual([row["type"] for row in dc_only], ["string"])

    def test_rank_puts_non_exact_first_closest_to_exact(self):
        rows = [{"va": 1, "score": {"cur": 100.0, "max": 100.0}},
                {"va": 2, "score": {"cur": 80.0, "max": 81.0}},
                {"va": 3, "score": None},
                {"va": 4, "score": {"cur": None, "max": 95.0}}]
        self.assertEqual([row["va"] for row in sorted(rows, key=diff.rank_key)], [4, 2, 1, 3])


class ProbeEditTest(unittest.TestCase):
    SOURCE = b"int f(int a)\n{\n    int value = a * 2;\n    return value + value;\n}\n"

    def local(self, **override):
        source = self.SOURCE
        start = source.index(b"int value")
        init = source.index(b"a * 2")
        uses = [i for i in range(len(source)) if source.startswith(b"value", i)][1:]
        local = {"name": "value", "type": "int", "storage_class": None,
                 "declaration": {"begin": start, "end": source.index(b";", start) + 1, "macro": False},
                 "initializer": {"begin": init, "end": init + 5, "macro": False},
                 "uses": [{"begin": u, "end": u + 5, "macro": False, "modifies": False} for u in uses]}
        local.update(override)
        return local

    def test_substitute_inlines_initializer_and_drops_declaration_line(self):
        self.assertIsNone(diff.removable(self.local()))
        self.assertEqual(diff.substitute(self.SOURCE, self.local()),
                         b"int f(int a)\n{\n    return (a * 2) + (a * 2);\n}\n")

    def test_unremovable_locals_name_the_reason(self):
        local = self.local()
        cases = {
            "no initializer": dict(initializer=None),
            "not a sole declaration statement": dict(declaration=None),
            "static storage": dict(storage_class="static"),
            "array": dict(type="int[4]"),
            "unused": dict(uses=[]),
            "reassigned, incremented or address-taken":
                dict(uses=[dict(local["uses"][0], modifies=True)]),
            "macro expansion": dict(initializer=dict(local["initializer"], macro=True)),
        }
        for reason, override in cases.items():
            self.assertEqual(diff.removable(self.local(**override)), reason)


class SelectionTest(unittest.TestCase):
    def setUp(self):
        functions = [{"offset": off, "cb": "12", "kind": "global", "name": name,
                      "module": module, "file": r"E:\gamedcs\unit.cpp", "line": "10",
                      "debug_start": "0", "debug_end": "12", "params": "0", "locals": "0"}
                     for off, name, module in (("0x100", "A::f", "unit.obj"),
                                               ("0x120", "A::g", "unit.obj"),
                                               ("0x140", "B::h", "other.obj"))]
        self.corpus = dreamcast.Corpus(functions=functions, variables=[], bridges=[], claims=[
            dreamcast.Claim(0x401000, "unit.obj", 0x100, "src/unit.cpp", 3),
            dreamcast.Claim(0x402000, "other.obj", 0x140, "src/other.cpp", 3)])

    def test_selectors_units_modules_and_all(self):
        with patch.object(diff, "unit_sources", return_value={"unit": "src/unit.cpp",
                                                              "other": "src/other.cpp"}):
            self.assertEqual([c.va for _, c in diff.paired(self.corpus, units=["other"])], [0x402000])
            with self.assertRaises(dreamcast.NoMatch):
                diff.paired(self.corpus, units=["missing"])
        self.assertEqual([c.va for _, c in diff.paired(self.corpus, modules=["unit"])], [0x401000])
        self.assertEqual(len(diff.paired(self.corpus, all_functions=True)), 2)
        self.assertEqual([c.va for _, c in diff.paired(self.corpus, selectors=["A::f"])], [0x401000])
        with self.assertRaises(dreamcast.NoMatch), \
                contextlib.redirect_stderr(io.StringIO()) as err:
            diff.paired(self.corpus, selectors=["A::g"])  # unclaimed
        self.assertIn("no retail source claim", err.getvalue())
        with self.assertRaisesRegex(dreamcast.DreamcastError, "exactly one"):
            diff.paired(self.corpus)

    def test_scores_prefer_the_owning_unit(self):
        ledger = {0x401000: [{"unit": "carrier", "cur": 90.0, "max": 90.0},
                             {"unit": "unit", "cur": 50.0, "max": 60.0}]}
        score = diff.score_for(0x401000, "src/unit.cpp", ledger, {"unit": "src/unit.cpp"})
        self.assertEqual(score["unit"], "unit")

    def test_command_is_registered(self):
        args = dreamcast._build_parser().parse_args(
            ["diff-locals", "--unit", "cursor", "--non-exact", "--limit", "3", "--probe", "--json"])
        self.assertEqual((args.unit, args.non_exact, args.limit, args.probe, args.json, args.jobs),
                         (["cursor"], True, 3, True, True, 2))
        self.assertIn("diff-locals", dreamcast.COMMANDS)


@unittest.skipUnless(shutil.which("clang"), "Clang is required for authored AST extents")
class AuthoredExtentsTest(unittest.TestCase):
    def test_locals_record_extents_uses_and_temporaries(self):
        source = """struct Text { Text(int); ~Text(); };
struct Point { Point(int, int); int x; };
void show(const Text&); int get(); void take(Point);
struct Window { void run(int a); };
void Window::run(int a) {
    int value = get();
    int total = value + a;
    total += value;
    int* where = &a;
    show(Text(value));
    Point p(1, 2);
    take(p);
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "unit.cpp"
            path.write_text(source)
            run = subprocess.run([shutil.which("clang"), "--target=i686-pc-windows-msvc", "-fsyntax-only",
                                  "-Xclang", "-ast-dump=json", "-Xclang", "-ast-dump-filter=Window::run",
                                  str(path)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            docs = source_facts.json_documents(run.stdout)
            root = next(d for d in docs if any(c["kind"] == "CompoundStmt" for c in d.get("inner", [])))
            result = source_facts.extract_function(docs, root["mangledName"], path, source)
        data = source.encode()
        locals_ = {row["name"]: row for row in result["locals"]}
        value = locals_["value"]
        self.assertEqual(data[value["declaration"]["begin"]:value["declaration"]["end"]],
                         b"int value = get();")
        self.assertEqual(data[value["initializer"]["begin"]:value["initializer"]["end"]], b"get()")
        self.assertEqual(len(value["uses"]), 3)
        self.assertIsNone(diff.removable(value))
        self.assertEqual(diff.removable(locals_["total"]), "reassigned, incremented or address-taken")
        self.assertEqual(diff.removable(locals_["p"]), "no initializer")
        self.assertEqual([(t["type"], t["destructor"]) for t in result["temporaries"]
                          if t["type"] == "Text"], [("Text", True)])
        copies = [call for call in result["calls"] if call.get("copy")]
        self.assertEqual([call["name"] for call in copies], ["Point::Point"])
        rewritten = diff.substitute(data, value).decode()
        self.assertIn("int total = (get()) + a;", rewritten)
        self.assertNotIn("int value", rewritten)


if __name__ == "__main__":
    unittest.main()
