"""Tests for the Loki h3maped pipeline pieces that need no staged inputs, plus
toolchain-backed checks that skip cleanly when GCC 2.95.2 is not staged."""
from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path

from homm3.loki import cmpobj, delink, objwriter, toolchain
from homm3.loki.cmpobj import CodeSection, Function
from homm3.loki.elf import R_386_32, R_386_PC32, SHT_REL, SHT_SYMTAB, Elf


class ComparisonObjectTest(unittest.TestCase):
    def test_round_trip(self):
        code = bytearray(bytes.fromhex("5589e5e8fcffffff68000000005dc3"))
        section = CodeSection(".text", code, [Function("caller__Fv", 0, len(code), True)])
        cmpobj.put(section, 4, R_386_PC32, "callee__Fv", -4)
        cmpobj.put(section, 9, R_386_32, '$s"x"#1', 0)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "unit.o"
            objwriter.write(path, [section])
            elf = Elf(path.read_bytes())
        symtab = next(s for s in elf.sections if s.type == SHT_SYMTAB)
        symbols = elf.symbols(symtab)
        rel = next(s for s in elf.sections if s.type == SHT_REL)
        targets = [(r.offset, r.type, symbols[r.symbol].name) for r in elf.rels(rel)]
        self.assertEqual(targets, [(4, R_386_PC32, "callee__Fv"), (9, R_386_32, '$s"x"#1')])
        self.assertIn("caller__Fv", [s.name for s in symbols])
        self.assertEqual(elf.bytes(elf.section(".text"))[4:8], (-4).to_bytes(4, "little", signed=True))

    def test_anonymous_namespace_is_canonical(self):
        retail = "drawSprite__34_GLOBAL_.N.GUIGameObject.cpppZMFtaPC7CSpriteUi"
        compiled = "drawSprite__39_GLOBAL_.N./src/GUIGameObject.cppaB3dE9PC7CSpriteUi"
        self.assertEqual(cmpobj.canonical_symbol(retail), "drawSprite__5_ANONPC7CSpriteUi")
        self.assertEqual(cmpobj.canonical_symbol(retail), cmpobj.canonical_symbol(compiled))

    def test_literal_names_carry_content(self):
        self.assertTrue(cmpobj.literal_name(b"GzBuf.cpp\0").startswith('$s"GzBuf.cpp"#'))
        self.assertEqual(cmpobj.literal_name(b"\0\0\0\0\0\0\xf0\x3f"), "$d000000000000f03f")


@unittest.skipUnless(toolchain.is_staged(), "GCC 2.95.2 not staged (homm3 loki toolchain)")
class CompiledBaseTest(unittest.TestCase):
    def test_static_call_and_literal_are_canonical(self):
        source = ('static int helper(int x) { return x * 3; }\n'
                  'const char* name() { return "hello"; }\n'
                  'int caller(int k) { return helper(k); }\n')
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory) / "t.cpp"
            obj = Path(directory) / "t.o"
            src.write_text(source)
            subprocess.run(toolchain.driver_command("-c", "-O0", "-mcpu=pentiumpro", str(src), "-o", str(obj)),
                           env=toolchain.environment(), check=True)
            (text,) = [s for s in delink.base_sections(obj.read_bytes()) if s.name == ".text"]
        targets = {r.target for r in text.relocs}
        self.assertIn("helper__Fi", targets)              # GAS resolved it; canonical form relocates it
        self.assertTrue(any(t.startswith('$s"hello"#') for t in targets))


if __name__ == "__main__":
    unittest.main()
