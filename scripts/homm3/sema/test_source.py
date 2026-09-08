"""Tests for source-map caching, rendering, and post-alignment diffing."""
from __future__ import annotations

import tempfile
import struct
from pathlib import Path
import unittest
from unittest.mock import patch

from homm3.core.test_codeview import _fixture
from homm3.sema import diff, source


def _asm(*instructions: str) -> str:
    lines = ["00000000 <func>:"]
    for offset, instruction in enumerate(instructions):
        mnemonic, *operand = instruction.split(None, 1)
        lines.append(f" {offset:x}: 90\t{mnemonic}\t"
                     f"{operand[0] if operand else ''}")
    return "\n".join(lines) + "\n"


class SourceMapTest(unittest.TestCase):
    def test_load_uses_recorded_header_not_same_numbered_tu_lines(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "header.h"
            header.write_text("\n" * 10 + "headerFirst();\nheaderSecond();\nheaderLast();\n")
            src = root / "unit.cpp"
            src.write_text('#include "header.h"\n' + "\n" * 9
                           + "wrongFirst();\nwrongSecond();\nwrongLast();\n")
            for filename in ("Z:" + str(header).replace("/", "\\"),
                             str(header), "header.h"):
                with self.subTest(filename=filename):
                    obj = root / "unit.obj"
                    obj.write_bytes(_fixture(files=(filename,)))
                    with patch.object(source, "_debug_obj", return_value=(
                            obj, src, "unit.cpp")), patch.object(source.common, "HOMM3_DIR", root):
                        mapping = source.load("unit", "func", 0, obj)
                    self.assertEqual(mapping.source, "header.h")
                    self.assertEqual(mapping.heads_at(1), (
                        source.Statement(1, 13, "headerLast();"),))
                    self.assertFalse(any("wrong" in row.text for row in mapping.statements))

    def test_unknown_recorded_file_does_not_fall_back_to_manifest_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            src = root / "unit.cpp"
            src.write_text("\n" * 10 + "wrongFirst();\nwrongSecond();\nwrongLast();\n")
            unrelated = root / "unrelated.h"
            unrelated.write_text(src.read_text())
            for filename in (str(unrelated), str(root / "missing.h"), r"Q:\unknown.h"):
                with self.subTest(filename=filename):
                    obj = root / "unit.obj"
                    obj.write_bytes(_fixture(files=(filename,)))
                    with patch.object(source, "_debug_obj", return_value=(obj, src, "unit.cpp")):
                        with self.assertRaisesRegex(source.SourceError, "not a current TU dependency"):
                            source.load("unit", "func", 0, obj)

    def test_recorded_compiler_header_is_allowed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            compiler = root / "compiler"
            header = compiler / "include" / "vector"
            header.parent.mkdir(parents=True)
            header.write_text("\n" * 10 + "first();\nsecond();\nlast();\n")
            src = root / "unit.cpp"
            src.write_text("#include <vector>\n")
            obj = root / "unit.obj"
            obj.write_bytes(_fixture(files=(str(header),)))
            with patch.object(source, "_debug_obj", return_value=(
                    obj, src, "unit.cpp")), patch.object(source.cc_wrap, "msvc_dir", return_value=compiler):
                mapping = source.load("unit", "func", 0, obj)
            self.assertEqual(mapping.source, str(header))
            self.assertEqual(mapping.heads_at(1)[0].text, "last();")

    def test_load_decodes_return_to_begin_line(self):
        payload = bytearray(_fixture())
        line_offset = struct.unpack_from("<I", payload, 20 + 28)[0]
        struct.pack_into("<H", payload, line_offset + 3 * 6 + 4, 0x7fff)
        with tempfile.TemporaryDirectory() as directory:
            obj = Path(directory) / "unit.obj"
            obj.write_bytes(payload)
            src = Path(directory) / "unit.cpp"
            src.write_text("\n" * 9 + "{\n    work();\n    finish();\n}\n")
            with patch.object(source, "_debug_obj", return_value=(
                    obj, src, "src/unit.cpp")):
                mapping = source.load("unit", "func", 0, obj)
        self.assertEqual(mapping.heads_at(1), (source.Statement(1, 10, "{"),))
        self.assertIn("; src/unit.cpp:10 | {", source.render_disassembly(
            _asm("nop", "ret"), mapping, verbose=False))

    def test_recorded_compiler_header_uses_wine_filename_case(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            compiler = root / "compiler"
            header = compiler / "include" / "XTREE"
            header.parent.mkdir(parents=True)
            header.write_text("\n" * 10 + "first();\nsecond();\nlast();\n")
            src = root / "unit.cpp"
            src.write_text("#include <xtree>\n")
            obj = root / "unit.obj"
            recorded = "Z:" + str(header.with_name("xtree")).replace("/", "\\")
            obj.write_bytes(_fixture(files=(recorded,)))
            with patch.object(source, "_debug_obj", return_value=(
                    obj, src, "unit.cpp")), patch.object(source.cc_wrap, "msvc_dir", return_value=compiler):
                mapping = source.load("unit", "func", 0, obj)
            self.assertEqual(mapping.source, str(header))
            self.assertEqual(mapping.heads_at(1)[0].text, "last();")

    def test_header_case_resolution_rejects_ambiguity_and_unowned_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            compiler = root / "compiler"
            include = compiler / "include"
            include.mkdir(parents=True)
            for name in ("XTREE", "Xtree"):
                (include / name).write_text("template source\n")
            (root / "HEADER.H").write_text("unrelated source\n")
            src = root / "unit.cpp"
            src.write_text("int value;\n")
            for recorded in (include / "xtree", root / "header.h"):
                with self.subTest(recorded=recorded):
                    with patch.object(source.cc_wrap, "msvc_dir", return_value=compiler):
                        with self.assertRaisesRegex(source.SourceError, "not a current TU dependency"):
                            source._recorded_source(str(recorded), src, "unit.cpp")

    def test_load_still_rejects_other_out_of_range_lines(self):
        with tempfile.TemporaryDirectory() as directory:
            obj = Path(directory) / "unit.obj"
            obj.write_bytes(_fixture())
            src = Path(directory) / "unit.cpp"
            src.write_text("\n" * 12)
            with patch.object(source, "_debug_obj", return_value=(
                    obj, src, "src/unit.cpp")):
                with self.assertRaisesRegex(source.SourceError, "/Z7 line 13"):
                    source.load("unit", "func", 0, obj)

    def test_begin_brace_resolves_to_first_executable_line(self):
        lines = ["int f()", "{", "    // comment", "", "    return 4;", "}"]
        self.assertEqual(source._first_body_line(lines, 2), 5)

    def test_debug_code_allows_only_alignment_nops(self):
        self.assertTrue(source._same_logical_code(b"\xc3\x90", b"\xc3"))
        self.assertFalse(source._same_logical_code(b"\xc3\xcc", b"\xc3"))
        self.assertFalse(source._same_logical_code(b"\xc3" + b"\x90" * 16,
                                                   b"\xc3"))

    def test_cache_payload_tracks_source_header_and_flags(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "local.h"
            source_file = root / "unit.cpp"
            header.write_text("#define N 1\n")
            source_file.write_text('#include "local.h"\nint x = N;\n')
            first = source._cache_payload("unit", source_file, ["/O2"])
            header.write_text("#define N 2\n")
            second = source._cache_payload("unit", source_file, ["/O2"])
            third = source._cache_payload("unit", source_file, ["/Od"])
        self.assertNotEqual(first, second)
        self.assertNotEqual(second, third)

    def test_lite_disassembly_labels_each_boundary_once(self):
        mapping = source.SourceMap("src/unit.cpp", (
            source.Statement(0, 10, "int a = 5;"),
            source.Statement(1, 11, "return a;"),
        ))
        rendered = source.render_disassembly(
            _asm("mov eax, ebx", "ret"), mapping, verbose=False)
        self.assertIn("; src/unit.cpp:10 | int a = 5;", rendered)
        self.assertIn("; src/unit.cpp:11 | return a;", rendered)
        self.assertEqual(rendered.count("src/unit.cpp:10"), 1)
        self.assertNotIn("90", rendered)
        self.assertIn("  0: mov eax, ebx", rendered)

    def test_lite_disassembly_folds_call_symbols_like_plain_disasm(self):
        mapping = source.SourceMap("src/unit.cpp", (
            source.Statement(0, 10, "callee();"),))
        listing = ("00000000 <func>:\n"
                   "       0: e8 00 00 00 00\tcall\t0x5 <func+0x5>\n"
                   "\t\t\t00000001:  IMAGE_REL_I386_REL32\t?callee@@YAXXZ\n"
                   "       5: c3\tret\n")
        rendered = source.render_disassembly(listing, mapping, verbose=False)
        self.assertIn("; src/unit.cpp:10 | callee();\n  0: call ?callee@@YAXXZ",
                      rendered)
        self.assertNotIn("IMAGE_REL", rendered)
        verbose = source.render_disassembly(listing, mapping, verbose=True)
        self.assertIn("IMAGE_REL_I386_REL32", verbose)


class StatementDiffTest(unittest.TestCase):
    def setUp(self):
        self.mapping = source.SourceMap("src/unit.cpp", (
            source.Statement(0, 10, "int a = 5;"),
            source.Statement(1, 11, "int b = a + 4;"),
        ))

    def test_first_changed_statement_is_named(self):
        base = _asm("mov eax, ebx", "add ecx, ebx", "ret")
        target = _asm("mov eax, ebx", "sub ecx, ebx", "ret")
        rendered, exact = diff._source_diff(base, target, self.mapping)
        self.assertFalse(exact)
        self.assertIn(
            "first divergent candidate statement: src/unit.cpp:11", rendered)
        self.assertIn("; !! src/unit.cpp:11 | int b = a + 4;", rendered)
        self.assertIn("~ base   add ecx, ebx", rendered)
        self.assertIn("target sub ecx, ebx", rendered)

    def test_source_text_never_changes_the_verdict(self):
        assembly = _asm("mov eax, ebx", "ret")
        altered = source.SourceMap("src/other.cpp", (
            source.Statement(0, 99, "completely different text;"),))
        first_render, first_exact = diff._source_diff(
            assembly, assembly, self.mapping)
        other_render, other_exact = diff._source_diff(
            assembly, assembly, altered)
        self.assertTrue(first_exact)
        self.assertEqual(first_exact, other_exact)
        self.assertNotEqual(first_render, other_render)

    def test_insert_is_grouped_under_nearest_candidate_statement(self):
        base = _asm("mov eax, ebx", "ret")
        target = _asm("mov eax, ebx", "add ecx, ebx", "ret")
        rendered, exact = diff._source_diff(base, target, self.mapping)
        self.assertFalse(exact)
        self.assertIn("+ target add ecx, ebx", rendered)
        self.assertIn("first divergent candidate statement", rendered)


if __name__ == "__main__":
    unittest.main()
