"""Reference rendering and regeneration boundaries, without proprietary inputs."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.analysis import dc_structure, debug_shape, dreamcast
from homm3.core.nb11_types import Types
from homm3.core.test_nb11 import fixture


def payload():
    shape = debug_shape.DebugFunctionShape(
        producer="CodeView NB11", target="Dreamcast WinCE SH4",
        address_space="Dreamcast .text offset", name="Function", module="unit.obj",
        linkage="global", address=0x100, emitted_size=32, source_file="unit.cpp",
        boundary_line=10, debug_start=4, debug_end=28,
        statements=(debug_shape.DebugStatement(0x108, 0, "helper.h", 20),
                    debug_shape.DebugStatement(0x108, 24, "unit.cpp", 12)),
        line_map=debug_shape.DebugLineMap(
            procedure_line=10, procedure_line_reliable=True, bodyless=False,
            first_body_line=12, first_body_address=0x108,
            gaps=(debug_shape.DebugSourceGap(10, 12, 11, 11, leading=True),)))
    return {
        "signature": "void __shcall Function(int value)", "type_index": 0x1000,
        "procedure_record": 4, "procedure_flags": 0, "has_endarg_record": True,
        "debug_shape": shape.to_dict(), "variables": [
            {"declaration": "int value", "name": "value", "kind": "param",
             "storage": "sp+0x4", "scope_id": "function", "type_index": 0x74},
            {"declaration": "unsigned int value", "name": "value", "kind": "local",
             "storage": "sp+0x8", "scope_id": "S1", "type_index": 0x75}],
        "local_symbols": [], "scopes": [
            {"id": "S0", "parent_id": "function", "record_offset": 20, "end_record": 72,
             "address": 0x108, "size": 8, "name": ""},
            {"id": "S1", "parent_id": "S0", "record_offset": 44, "end_record": 68,
             "address": 0x108, "size": 8, "name": ""}],
        "inline_evidence": [], "cfg": [],
    }


def corpus():
    row = {"offset": "0x100", "cb": "32", "name": "Function", "module": "unit.obj",
           "file": "C:\\unit.cpp", "line": "10", "kind": "global", "debug_start": "0",
           "debug_end": "32", "params": "0", "locals": "1"}
    return dreamcast.Corpus(functions=[row], variables=[], bridges=[], claims=[])


class RenderTest(unittest.TestCase):
    def test_repeated_line_addresses_are_preserved_and_scopes_keep_recorded_nesting(self):
        text = dc_structure.render_function(payload(), Types({}))
        self.assertIn("|0x008|+0x000:'20' // helper.h", text)
        self.assertIn("|0x008|+0x018:'12' // unit.cpp", text)
        self.assertIn("//   S1 {", text)
        self.assertIn("//     unsigned int value; // sp+0x8", text)
        self.assertLess(text.index("SCOPE BEGIN S0"), text.index("SCOPE BEGIN S1"))
        self.assertLess(text.index("SCOPE END S1"), text.index("SCOPE END S0"))
        self.assertIn("LEADING GAP: 11..11 (1 lines in unit.cpp)", text)
        self.assertNotIn("VA(", text)

    def test_inline_evidence_retains_uncertainty_and_standalone_helper(self):
        value = payload()
        value["inline_evidence"] = [{
            "confidence": "positive", "kind": "foreign-source", "address": 0x108,
            "end_address": 0x110, "source": "helper.h", "source_lines": [20],
            "helper_candidates": [{"dc_offset": 0x300, "signature": "void __shcall Helper()"}],
            "statements": [{"address": 0x108, "line": 20, "source": "helper.h"}],
        }]
        text = dc_structure.render_function(value, Types({}))
        self.assertIn("helper identity is a candidate", text)
        self.assertIn("helper candidate dc:0x00000300: void __shcall Helper()", text)
        self.assertIn("INLINE I0", text)
        self.assertNotIn("inline void", text)

    def test_zero_length_scope_still_opens_and_closes(self):
        value = payload()
        value["scopes"][1]["size"] = 0
        text = dc_structure.render_function(value, Types({}))
        self.assertLess(text.index("SCOPE BEGIN S1"), text.index("SCOPE END S1"))


class ExportTest(unittest.TestCase):
    def run_export(self, output, **kwargs):
        with patch.object(dc_structure.inputs, "read_dreamcast_exe", return_value=fixture()), \
             patch.object(dc_structure, "function_payload", return_value=payload()):
            return dc_structure.export(corpus(), output=output, **kwargs)

    def test_repeatable_export_and_cleanup_only_of_previously_generated_files(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "out"
            index = self.run_export(output, modules=["unit"])
            before = {name: (output / name).read_bytes() for name in index["files"]}
            self.assertEqual(index["summary"]["functions"], 1)
            self.assertIn("sources/unit.cpp", index["files"])
            self.run_export(output, modules=["unit.obj"])
            self.assertEqual(before, {name: (output / name).read_bytes() for name in index["files"]})
            (output / "notes.txt").write_text("keep me")
            (output / "sources/stale.cpp").write_text("old generated file")
            index["files"].append("sources/stale.cpp")
            (output / "index.json").write_text(json.dumps(index))
            self.run_export(output)
            self.assertFalse((output / "sources/stale.cpp").exists())
            self.assertEqual((output / "notes.txt").read_text(), "keep me")

    def test_bad_module_or_unowned_destination_does_not_write(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "out"
            with self.assertRaises(dreamcast.NoMatch):
                self.run_export(output, modules=["missing"])
            self.assertFalse(output.exists())
            output.mkdir()
            (output / "handwritten.cpp").write_text("keep")
            with self.assertRaisesRegex(dreamcast.DreamcastError, "not a structure export"):
                self.run_export(output)
            self.assertEqual(list(output.iterdir()), [output / "handwritten.cpp"])

    def test_failed_extraction_keeps_previous_export(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "out"
            self.run_export(output)
            before = (output / "sources/unit.cpp").read_bytes()
            with patch.object(dc_structure, "render_types", side_effect=ValueError("broken type")):
                with self.assertRaisesRegex(ValueError, "broken type"):
                    self.run_export(output)
            self.assertEqual((output / "sources/unit.cpp").read_bytes(), before)

    def test_manifest_cannot_remove_files_outside_export(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "out"
            output.mkdir()
            (output / "index.json").write_text(json.dumps({
                "schema": dc_structure.SCHEMA, "generator": dc_structure.GENERATOR,
                "files": ["../keep.cpp"]}))
            with self.assertRaisesRegex(dreamcast.DreamcastError, "invalid generated file paths"):
                self.run_export(output)


if __name__ == "__main__":
    unittest.main()
