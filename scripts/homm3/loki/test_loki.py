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
        self.assertEqual(
            cmpobj.literal_name(b"Q231_GLOBAL_.N.ObjectType.cppIWVf1b18TTerrainSlotTraits\0"),
            cmpobj.literal_name(b"Q231_GLOBAL_.N.ObjectType.cppULngUc18TTerrainSlotTraits\0"))
        self.assertEqual(cmpobj.literal_name(b"\0\0\0\0\0\0\xf0\x3f"), "$d000000000000f03f")

    def test_string_tables_are_named_by_their_strings(self):
        strings = [b"dirttl.def\0", b"sandtl.def\0"]
        image = {0x100: strings[0], 0x10b: strings[1]}
        read = lambda address, size: image[address][:size]
        table = lambda address: [cmpobj.literal_name(cmpobj.c_string(read, a)) for a in (0x100, 0x10b)]
        named = cmpobj.literal_for(read, 0x200, 0, None, table)
        self.assertTrue(named.startswith('$t2[$s"dirttl.def"#'))
        self.assertEqual(named, cmpobj.table_name([cmpobj.literal_name(s) for s in strings]))

    def test_pointer_tables_are_named_by_their_targets(self):
        targets = {0x200: "aCastle", 0x204: "aRampart"}
        read = lambda address, size: bytes(size)
        named = cmpobj.literal_for(read, 0x200, 0, targets.get)
        self.assertEqual(named, cmpobj.pointer_table_name(["aCastle", "aRampart"]))
        base_list = cmpobj.literal_for(read, 0x200, 0, {0x200: "__ti5THero"}.get)
        self.assertEqual(base_list, "$p__ti5THero+00000000")

    def test_function_local_statics_pair_by_order(self):
        def section(functions, relocs, local_data=()):
            text = CodeSection(".text", bytearray(0x40), functions, local_data=set(local_data))
            for offset, target in relocs:
                cmpobj.put(text, offset, R_386_32, target, 0)
            return text
        base = [section([Function("__tcf_0", 0, 0x10, False), Function("f__Fv", 0x10, 0x30, True)],
                        [(0x14, "a.12"), (0x18, "_.tmp_0.13"), (0x1c, "callee__Fv"),
                         (0x20, "__tcf_0"), (0x24, "g_mask"), (0x28, "b.14")],
                        ["a.12", "_.tmp_0.13", "b.14", "g_mask"])]
        target = [section([Function("sub_00002000", 0, 0x10, False), Function("f__Fv", 0x10, 0x30, True)],
                          [(0x14, "data_00001000"), (0x18, "data_0000100c"), (0x1c, "callee__Fv"),
                           (0x20, "sub_00002000"), (0x24, "data_00003000"), (0x28, "data_00001010")])]
        delink.pair_locals(base, target)
        self.assertEqual([r.target for r in target[0].relocs],
                         ["a.12", "_.tmp_0.13", "callee__Fv", "__tcf_0", "data_00003000", "b.14"])
        self.assertEqual(target[0].functions[0].name, "__tcf_0")


    def test_file_static_data_pairs_through_pointer_tables(self):
        def section(relocs):
            text = CodeSection(".text", bytearray(0x20), [Function("init__Fv", 0, 0x20, True)])
            for offset, target in relocs:
                cmpobj.put(text, offset, R_386_32, target, 0)
            return text
        compiled = cmpobj.pointer_table_name(["castle1", "g_named", "castle2+c"])
        retail = cmpobj.pointer_table_name(["data_00001000", "g_named", "data_0000100c"])
        base = [section([(0x4, "castle1"), (0x8, "ctor__Fv"), (0xc, compiled)])]
        target = [section([(0x4, "data_00001000"), (0x8, "ctor__Fv"), (0xc, retail)])]
        mapping = delink.pair_data(base, target)
        self.assertEqual(mapping["data_0000100c"], ("castle2", 0xc))
        self.assertEqual([r.target for r in target[0].relocs], ["castle1", "ctor__Fv", compiled])
        # A table that differs in a named entry pairs nothing.
        other = cmpobj.pointer_table_name(["data_00002000", "g_other"])
        base = [section([(0x4, cmpobj.pointer_table_name(["castle1", "g_named"]))])]
        target = [section([(0x4, other)])]
        self.assertEqual(delink.pair_data(base, target), {})
        self.assertEqual(target[0].relocs[0].target, other)

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

    def test_one_past_the_end_names_the_next_object(self):
        source = ('extern const int a[4];\nextern const int b;\n'
                  'const int a[4] = { 1, 2, 3, 4 };\nconst int b = 5;\n'
                  'const int* end() { return a + 4; }\n')
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory) / "t.cpp"
            obj = Path(directory) / "t.o"
            src.write_text(source)
            subprocess.run(toolchain.driver_command("-c", "-O0", "-mcpu=pentiumpro", str(src), "-o", str(obj)),
                           env=toolchain.environment(), check=True)
            (text,) = [s for s in delink.base_sections(obj.read_bytes()) if s.name == ".text"]
        self.assertEqual([(r.target, r.addend) for r in text.relocs], [("b", 0)])


if __name__ == "__main__":
    unittest.main()
