"""Tests for source-map caching, rendering, and post-alignment diffing."""
from __future__ import annotations

import tempfile
from pathlib import Path
import unittest

from homm3.sema import diff, source


def _asm(*instructions: str) -> str:
    lines = ["00000000 <func>:"]
    for offset, instruction in enumerate(instructions):
        mnemonic, *operand = instruction.split(None, 1)
        lines.append(f" {offset:x}: 90\t{mnemonic}\t"
                     f"{operand[0] if operand else ''}")
    return "\n".join(lines) + "\n"


class SourceMapTest(unittest.TestCase):
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
