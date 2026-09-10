"""Line geometry must retain evidence without manufacturing original text."""
import contextlib
import io
import json
import unittest
from unittest.mock import patch

from homm3.analysis import dc_source_layout as layout, dreamcast, dc_structure
from homm3.analysis.test_dreamcast import _fn
from homm3.analysis.test_dc_structure import payload
from homm3.core.nb11_types import Types


class LayoutTest(unittest.TestCase):
    def recover(self, records, **kwargs):
        return layout.recover(records, source="unit.cpp", boundary_line=10,
                              boundary_reliable=True, bodyless=False, **kwargs)

    def test_span_counts_include_holes_but_never_claim_blank_lines_or_total(self):
        result = self.recover([("unit.cpp", 10, 0x100), ("unit.cpp", 14, 0x104),
                               ("unit.cpp", 14, 0x108), ("unit.cpp", 20, 0x10c)])
        source = result["files"][0]
        self.assertEqual(source["observed_span_lines"], 11)
        self.assertEqual(source["recorded_line_count"], 3)
        self.assertEqual(source["line_row_count"], 4)
        self.assertEqual(source["unrecorded_lines_within_span"], 8)
        self.assertEqual(source["gaps"], [
            {"first_line": 11, "last_line": 13, "line_count": 3},
            {"first_line": 15, "last_line": 19, "line_count": 5}])
        for key in ("function_line_count", "blank_line_count", "trailing_line_count"):
            self.assertIsNone(result[key])
        rendered = layout.render(result, rows=True)
        self.assertIn("11..13 | 3 unrecorded line(s), contents unknown", rendered)
        self.assertIn("11 lines; 3 recorded; 8 unrecorded; 4 rows", rendered)

    def test_equal_addresses_file_switches_repeated_rows_and_backwards_lines_survive(self):
        rows = [("unit.cpp", 14, 0x108), ("unit.cpp", 10, 0x100),
                ("helper.h", 700, 0x104), ("unit.cpp", 3, 0x104),
                ("unit.cpp", 3, 0x104)]
        result = self.recover(rows)
        self.assertEqual([row["line"] for row in result["rows"]], [10, 700, 3, 3, 14])
        self.assertEqual([row["attribution"] for row in result["rows"]], [
            "owning-source", "foreign-source", "earlier-owning-source",
            "earlier-owning-source", "owning-source"])
        self.assertEqual(result["files"][0]["observed_span_lines"], 12)
        self.assertEqual(result["files"][1]["observed_span_lines"], 1)
        self.assertIn("inline attributions can extend", result["caution"])

    def test_unknown_missing_and_minimal_boundaries_do_not_gain_a_function_length(self):
        for records, boundary, reliable, bodyless in [
            ([], None, False, False),
            ([("unit.cpp", 20, 0x100), ("unit.cpp", 70, 0x108)], 20, False, False),
            ([("unit.cpp", 10, 0x100), ("unit.cpp", 90, 0x100)], 10, True, True),
        ]:
            result = layout.recover(records, source="unit.cpp", boundary_line=boundary,
                                    boundary_reliable=reliable, bodyless=bodyless)
            self.assertIsNone(result["function_line_count"])
            self.assertIsNone(result["blank_line_count"])
            if records and not reliable:
                self.assertEqual(result["rows"][0]["attribution"], "unreliable-boundary")
            self.assertIn("unknown", layout.render(result))

    def test_source_paths_are_normalized_without_merging_different_directories(self):
        result = self.recover([("C:/src/a.h", 1, 0x100), ("c:\\SRC\\a.h", 2, 0x102),
                               ("C:/other/a.h", 90, 0x104)])
        self.assertEqual(len(result["files"]), 2)
        self.assertEqual(result["files"][0]["recorded_line_count"], 2)

    def test_function_extent_excludes_next_procedure_and_iterator_is_reusable(self):
        row = _fn("0x100", "Function", cb="16")
        rows = [(row["file"], 10, 0x100), (row["file"], 12, 0x108),
                (row["file"], 500, 0x110)]
        result = dreamcast._source_line_shape(row, iter(rows))["source_layout"]
        self.assertEqual([r["line"] for r in result["rows"]], [10, 12])

    def test_structure_output_uses_same_layout_evidence(self):
        value = payload()
        value["debug_shape"]["line_map"]["source_layout"] = self.recover(
            [("unit.cpp", 10, 0x100), ("unit.cpp", 14, 0x108)])
        rendered = dc_structure.render_function(value, Types({}))
        self.assertIn("gap 11..13: 3 unrecorded line(s), contents unknown", rendered)
        self.assertIn("Function total / empty / trailing lines: unknown", rendered)

    def test_index_keeps_previous_boundary_evidence_and_identical_attribution_order(self):
        previous = _fn("0xe0", "Previous", cb="30")
        current = _fn("0x100", "Current", cb="32")
        current["line"] = "20"
        records = [(current["file"], 10, 0xe0), (current["file"], 18, 0xfc),
                   (current["file"], 20, 0x100), (current["file"], 70, 0x108),
                   ("helper.h", 8, 0x108), (current["file"], 70, 0x108),
                   (current["file"], 99, 0x120)]
        corpus = dreamcast.Corpus(functions=[previous, current], variables=[], bridges=[], claims=[])
        with patch.object(dreamcast.dc_srclines, "_load_srclines", return_value={"unit.obj": records}):
            indexed = dreamcast._line_payloads(corpus, [current])[0]
            direct = dreamcast._gap_payload(corpus, current)
        self.assertEqual(indexed, direct)
        self.assertTrue(indexed["borrowed_boundary_line"])
        self.assertEqual([r["line"] for r in indexed["source_layout"]["rows"]], [20, 70, 8, 70])


class BatchTest(unittest.TestCase):
    def setUp(self):
        self.rows = [_fn(hex(0x100 + 0x10 * i), f"Function{i}") for i in range(12)]
        self.rows.append(_fn("0x300", "Other", "other.obj"))
        self.corpus = dreamcast.Corpus(functions=self.rows, variables=[], bridges=[], claims=[])

    def select(self, *args):
        return dreamcast._batch_matches(self.corpus,
                                        dreamcast._build_parser().parse_args(list(args)))

    def test_whole_modules_and_corpus_are_not_limited_to_eight_functions(self):
        self.assertEqual(len(self.select("show", "--module", "UNIT")), 12)
        self.assertEqual(len(self.select("lines", "--all")), 13)
        self.assertEqual(len(self.select("lines", "--module", "unit", "--module", "other.obj")), 13)

    def test_repeated_selectors_deduplicate_and_module_filters_apply(self):
        self.assertEqual(self.select("lines", "dc:100", "dc:100", "dc:110"), self.rows[:2])
        self.assertEqual(self.select("lines", "dc:100", "dc:300", "--module", "unit"), self.rows[:1])

    def test_no_silent_fallback_to_all_on_invalid_selection(self):
        for args in [("lines",), ("show", "--module", "unknown"),
                     ("lines", "dc:100", "--all"), ("show", "dc:100", "--module", "other")]:
            with self.assertRaises(dreamcast.DreamcastError):
                self.select(*args)

    def test_lines_cli_needs_no_msvc_or_sh4_disassembly(self):
        row = self.rows[0]
        records = [(row["file"], 10, 0x100), (row["file"], 14, 0x104)]
        with patch.object(dreamcast, "Corpus", return_value=self.corpus), \
             patch.object(dreamcast.dc_srclines, "_load_srclines", return_value={"unit.obj": records}), \
             patch.object(dreamcast, "build_dossier", side_effect=AssertionError("unexpected disassembly")), \
             patch.object(dreamcast, "_log"), contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(dreamcast._dispatch(["lines", "dc:100", "--json"]), 0)
        data = json.loads(out.getvalue())
        self.assertEqual(len(data["functions"]), 1)
        self.assertEqual(data["functions"][0]["source_layout"]["files"][0]["observed_span_lines"], 5)


if __name__ == "__main__":
    unittest.main()
