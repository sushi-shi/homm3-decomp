"""Tooling contracts of the context-variant enumerator (no compiler runs)."""
from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path

from homm3.vc6 import context_variants as cv


def _coff(code: bytes, relocs: list[int], symbols: list[tuple[str, int]]) -> bytes:
    """One .text section, IMAGE_REL_I386_DIR32 relocations, short symbols."""
    header_size, section_size = 20, 40
    code_at = header_size + section_size
    reloc_at = code_at + len(code)
    symtab_at = reloc_at + 10 * len(relocs)
    head = struct.pack("<HHIIIHH", 0x14c, 1, 0, symtab_at, len(symbols), 0, 0)
    section = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(code), code_at,
                          reloc_at, 0, len(relocs), 0, 0x60000020)
    rel = b"".join(struct.pack("<IIH", offset, 0, 6) for offset in relocs)
    syms = b"".join(struct.pack("<8sIhHBB", name.encode().ljust(8, b"\0"), value, 1, 0x20, 2, 0)
                    for name, value in symbols)
    return head + section + code + rel + syms + struct.pack("<I", 4)


class FunctionBytesTest(unittest.TestCase):
    def test_split_by_symbol_mask_relocations_strip_padding(self):
        code = bytes([0x55, 0xa1, 1, 2, 3, 4, 0xc3, 0x90, 0x90, 0x33, 0xc0, 0xc3])
        data = _coff(code, [2], [("?f@@Y", 0), ("?g@@Y", 9)])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "t.obj"
            path.write_bytes(data)
            self.assertEqual(cv.function_bytes(path, "?f@@Y"),
                             bytes([0x55, 0xa1, 0, 0, 0, 0, 0xc3]))
            self.assertEqual(cv.function_bytes(path, "?g@@Y"), bytes([0x33, 0xc0, 0xc3]))
            self.assertIsNone(cv.function_bytes(path, "?h@@Y"))


class ChannelTest(unittest.TestCase):
    def test_callee_points_cross_a_test_within_the_window_nearest_first(self):
        sites = [dict(callee="?a", cb=41, budget=36), dict(callee="?b", cb=60, budget=58),
                 dict(callee="?c", cb=200, budget=10)]
        points = cv.callee_points(sites, limit=10)
        self.assertEqual(points[0], ("?a", 40))       # free threshold, margin 1
        self.assertIn(("?b", 58), points)             # budget edge
        self.assertNotIn(("?c", 10), points)          # outside the cost window
        self.assertNotIn(("?a", 41), points)          # the current cost is no variant

    def test_parse_main_reads_cost_and_phase_of_the_root(self):
        log = ("sym 00000010 ?root@@YAXXZ\nmain 00000010 cb=513 phase=0\n"
               "sym 00000020 ?other@@YAXXZ\nmain 00000020 cb=9 phase=1\n")
        self.assertEqual(cv.parse_main(log, "?root@@YAXXZ"), {"cost": 513, "phase": 0})

    def test_context_label_and_key(self):
        ctx = cv.Context(phase=1, root_cost=910, callee_cost={"?getHero": 41})
        self.assertEqual(ctx.label(), "phase=1, root-cost=910, cost[?getHero]=41")
        self.assertEqual(cv.Context().label(), "captured context")
        self.assertNotEqual(ctx.key(), cv.Context(phase=1).key())


class DeclarationOffsetTest(unittest.TestCase):
    SOURCE = "#include <a.h>\nint g;\n// note\nVA(0x00401230, 0x10)\nvoid f() {}\n"

    def test_before_places_k_declarations_ahead_of_the_annotation(self):
        text = cv.padded_source(self.SOURCE, 0x401230, 2, "before")
        self.assertEqual(text, "#include <a.h>\nint g;\n// note\ntypedef int h3ctx_decl0;\n"
                               "typedef int h3ctx_decl1;\nVA(0x00401230, 0x10)\nvoid f() {}\n")

    def test_top_prefixes_the_unit_and_missing_annotation_is_none(self):
        self.assertTrue(cv.padded_source(self.SOURCE, 0x401230, 1, "top")
                        .startswith("typedef int h3ctx_decl0;\n#include"))
        self.assertIsNone(cv.padded_source(self.SOURCE, 0x409999, 1, "before"))
        self.assertEqual(cv.padded_source(self.SOURCE, 0x401230, 0, "before"), self.SOURCE)


if __name__ == "__main__":
    unittest.main()
