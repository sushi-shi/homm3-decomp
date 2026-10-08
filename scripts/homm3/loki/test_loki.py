"""Tests for the Loki h3maped pipeline pieces that need no staged inputs, plus
toolchain-backed checks that skip cleanly when GCC 2.95.2 is not staged."""
from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path

from homm3.core import inputs
from homm3.loki import (cmpobj, datacmp, delink, emitorder, image, ledger, link, linkdiff, linklibs, objwriter,
                        toolchain, xstubs)
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

class DataComparisonTest(unittest.TestCase):
    def test_symbol_key_drops_only_the_random_suffix(self):
        retail = "__vt_Q234_GLOBAL_.N.GUIGameObject.cppMHhwNb29TGUIStandardSpecializedObject"
        compiled = "__vt_Q234_GLOBAL_.N.GUIGameObject.cppaB3dE929TGUIStandardSpecializedObject"
        other = "__vt_Q233_GLOBAL_.N.ObjectHelp.cppMHhwNbAB29TGUIStandardSpecializedObject"
        self.assertEqual(datacmp.symbol_key(retail), datacmp.symbol_key(compiled))
        self.assertNotEqual(datacmp.symbol_key(retail), datacmp.symbol_key(other))
        self.assertEqual(datacmp.symbol_key("akAdvObjectTypeTraits"), "akAdvObjectTypeTraits")

    def test_items_cover_symbols_and_gaps(self):
        from homm3.loki.elf import Section, Symbol, STT_OBJECT
        header = Section(3, ".data", 1, 3, 0, 0, 0x20, 0, 0, 4, 0)
        symbols = [Symbol(1, "a", 0x4, 8, 0, STT_OBJECT, 3), Symbol(2, "b", 0xc, 4, 1, STT_OBJECT, 3)]
        compiled = datacmp.Compiled("u", 0, None, symbols, {})
        items = datacmp._items(compiled, header, 0x20)
        self.assertEqual([(i.name, i.offset, i.size) for i in items],
                         [(".data+0x0", 0, 4), ("a", 4, 8), ("b", 0xc, 4), (".data+0x10", 0x10, 0x10)])

    def test_data_rows_bank_bytes(self):
        rows: dict[int, ledger.Row] = {}
        ledger.bank_data(rows, "Army", 4, 90, 100, "f1")
        ledger.bank_data(rows, "Army", 4, 80, 100, "f1")
        self.assertEqual((rows[4].cur, rows[4].max, rows[4].hist), (80, 90, 90))
        ledger.bank_data(rows, "Army", 4, 85, 100, "f2")      # source changed: MAX resets
        self.assertEqual((rows[4].cur, rows[4].max, rows[4].hist), (85, 85, 90))
        ledger.bank_data(rows, "Army", 4, 70, 120, "f2")      # slice resized: new denominator
        self.assertEqual((rows[4].cur, rows[4].max, rows[4].hist), (70, 70, 70))
        self.assertEqual(ledger.data_rows(rows)[0].name, ledger.DATA)


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

    def test_emitted_functions_are_keyed_as_the_image_shows_them(self):
        source = ('struct Shape { virtual int area() { return 0; } };\n'
                  'static int helper(int x) { return x * 3; }\n'
                  'int caller(int k) { Shape s; return helper(k) + s.area(); }\n')
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory) / "t.cpp"
            obj = Path(directory) / "t.o"
            src.write_text(source)
            subprocess.run(toolchain.driver_command("-c", "-O0", "-mcpu=pentiumpro", str(src), "-o", str(obj)),
                           env=toolchain.environment(), check=True)
            entries = emitorder.compiled_entries(obj, {"__tf5Shape"}, {"caller__Fi", "__tf5Shape"})
        names = [e.name for e in entries]
        self.assertIn("__tf5Shape", names)          # the type_info function of a class without key function
        keys = {e.name: e.key for e in entries}
        self.assertEqual(keys["caller__Fi"], "caller__Fi")
        self.assertEqual(keys["__tf5Shape"], "__tf5Shape")     # kept here
        self.assertTrue(keys["helper__Fi"].startswith("static:"))
        self.assertTrue(keys["area__5Shape"].startswith("linkonce:"))   # the image keeps it elsewhere
        order = emitorder.Order("t", 0, entries, entries)
        self.assertTrue(order.identical)
        self.assertFalse(any(line.startswith(("-", "+")) for line in emitorder.render(order)))

    def test_anonymous_namespace_suffixes_compare_like_symbols(self):
        ours = bytearray(b"Q229_GLOBAL_.N.SeersHut.cppjSyidb13TRewardCloner\0" b"_GLOBAL_.N.Hero.cppABCDEF\0")
        image = b"Q229_GLOBAL_.N.SeersHut.cppPBS6wb13TRewardCloner\0" b"_GLOBAL_.N.Town.cppABCDEF\0"
        datacmp.anonymous_suffixes_agree(ours, image)
        self.assertEqual(bytes(ours[:50]), image[:50])
        self.assertNotEqual(bytes(ours[50:]), image[50:])     # another namespace stays different

    def test_rodata_string_sequences_pick_source_files_and_type_names(self):
        raw = (b"Q28TGameMap7TClient\0GzBuf.cpp\0bad_alloc\0" b"9TBitmap16\0"
               b"t6vector2ZiZt9allocator1Zi\0%d of %d\0" b"12\0x\0Map View.h\0")
        strings = emitorder.rodata_strings(raw)
        self.assertEqual([s for s in strings if emitorder.is_source_file(s)], ["GzBuf.cpp"])
        self.assertEqual([s for s in strings if emitorder.is_type_name(s)],
                         ["Q28TGameMap7TClient", "9TBitmap16", "t6vector2ZiZt9allocator1Zi"])
        self.assertFalse(emitorder.is_type_name("9bad_alloc+"))
        self.assertFalse(emitorder.is_type_name("Q28TGameMap"))     # one part short
        order = emitorder.Strings("t", ["A.h", "B.h"], ["B.h", "A.h"])
        self.assertFalse(order.identical)
        self.assertEqual([tag for tag, *_ in order.opcodes], ["delete", "equal", "insert"])


class LinkTest(unittest.TestCase):
    def test_command_follows_the_image_link_order(self):
        argv = link.command(Path("out"), Path("out.map"))
        names = [Path(a).name for a in argv]
        self.assertIn("-export-dynamic", argv)
        self.assertEqual(argv[argv.index("-dynamic-linker") + 1], "/lib/ld-linux.so.2")
        self.assertEqual(names.index("crt1.o") + 1, names.index("crti.o"))
        self.assertEqual(names.index("crti.o") + 1, names.index("crtbegin.o"))
        self.assertEqual(names[-2:], ["crtend.o", "crtn.o"])
        objects = [Path(a) for a in argv if a.endswith(".o") and "/obj/" in a]
        self.assertEqual(len(objects), 104)   # the 103 census objects and the version object
        self.assertEqual(objects, link.project_objects())
        self.assertEqual(objects[-1].name, "version.o")
        archives = [n for n in names if n.endswith(".a")]
        self.assertEqual(archives[:6], list(link.ARCHIVES_BEFORE))
        self.assertEqual(archives[6:], ["libz.a", "libstdc++.a", "libgcc.a", "libgcc.a"])
        shared = [a for a in argv if a.startswith("-l")]
        # libm first comes from g++'s tail, after libstdc++.a (an earlier -lm versions clog).
        self.assertEqual(shared, ["-ldl", "-lXi", "-lXext", "-lX11", "-lm", "-lc"])
        self.assertLess(names.index("libglib.a"), argv.index("-ldl"))
        self.assertLess(names.index("libstdc++.a"), argv.index("-lm"))
        self.assertEqual(argv[argv.index("-rpath-link") + 1].split(":")[0], str(toolchain.LINK / "glibc"))

    def test_link_media_digest_covers_the_pins(self):
        spec = toolchain.specification()
        changed = {**spec, "link": {**spec["link"], "zlib.tgz": "0" * 64}}
        self.assertNotEqual(toolchain._link_digest(spec), toolchain._link_digest(changed))
        self.assertEqual(toolchain._link_digest(spec), toolchain._link_digest(dict(spec)))

    def test_link_media_digest_covers_the_patches(self):
        spec = toolchain.specification()
        self.assertTrue((toolchain.LINK_PATCHES / linklibs.ZLIB_PATCH).is_file())
        with tempfile.TemporaryDirectory() as tmp:
            original = toolchain.LINK_PATCHES
            try:
                toolchain.LINK_PATCHES = Path(tmp)
                empty = toolchain._link_digest(spec)
                (Path(tmp) / "x.patch").write_text("--- a/x\n")
                self.assertNotEqual(toolchain._link_digest(spec), empty)
            finally:
                toolchain.LINK_PATCHES = original


class XStubTest(unittest.TestCase):
    def test_stubs_list_the_table_in_order(self):
        table = xstubs.rows()
        with tempfile.TemporaryDirectory() as tmp:
            paths = xstubs.write(Path(tmp))
            self.assertEqual([p.name for p in paths], list(xstubs.LIBRARIES.values()))
            for library, soname in xstubs.LIBRARIES.items():
                stub = Elf((Path(tmp) / soname).read_bytes())
                self.assertTrue((Path(tmp) / f"{library}.so").is_symlink())
                listed = [r for r in table if r.library == library]
                names = [s.name for s in stub.dynsym[1:]]
                self.assertEqual(names[:len(listed)], [r.name for r in listed])
                self.assertEqual(len(names), len(set(names)))
                self.assertTrue(set(xstubs.STRUCTURE) <= set(names))
                symbols = {s.name: s for s in stub.dynsym}
                for row in table:
                    if row.defined == library:
                        self.assertEqual((symbols[row.name].size, symbols[row.name].type),
                                         (row.size, xstubs.TYPES[row.type]), row)
                        if row.name not in xstubs.STRUCTURE:
                            self.assertNotEqual(symbols[row.name].shndx, 0, row)
                    elif row.library == library:
                        self.assertEqual(symbols[row.name].shndx, 0, row)
                dynamic = stub.bytes(stub.section(".dynamic"))
                tags = [int.from_bytes(dynamic[i:i + 4], "little") for i in range(0, len(dynamic), 8)]
                self.assertIn(xstubs.DT_SONAME, tags)
                versioned = any(r.version for r in listed)
                self.assertEqual(any(s.name == ".gnu.version_r" for s in stub.sections), versioned)

    @unittest.skipUnless(inputs.is_staged(image.executable()), inputs.requires_staged(image.executable()))
    def test_table_is_the_image_dynsym(self):
        retail = image.LokiImage().elf
        table = xstubs.rows()
        runs = [r for r in table if r.version is None]
        self.assertEqual([r.index for r in runs], list(range(6, 204)))
        self.assertEqual([r.library for r in runs], sorted((r.library for r in runs),
                         key=list(xstubs.LIBRARIES).index))
        kinds = {v: k for k, v in xstubs.TYPES.items()}
        for row in table:
            symbol = retail.dynsym[row.index]
            self.assertEqual((symbol.name, kinds[symbol.type], symbol.size), (row.name, row.type, row.size), row)
            self.assertIn(row.defined, (*xstubs.LIBRARIES, "libc.so.6"))
        x_imports = {s.name for s in retail.dynsym if s.shndx == 0 and s.name.startswith("X")}
        self.assertEqual(x_imports, {r.name for r in runs if r.name.startswith("X")})


class LinkDiffTest(unittest.TestCase):
    def test_differing_counts_bytes_and_length(self):
        self.assertEqual(linkdiff.differing(b"abc", b"abc"), 0)
        self.assertEqual(linkdiff.differing(b"abc", b"abd"), 1)
        self.assertEqual(linkdiff.differing(b"abc", b"ab"), 1)
        self.assertEqual(linkdiff.differing(b"", b"xyz"), 3)

    def test_comment_runs_and_order(self):
        self.assertEqual(linkdiff.runs(["e", "e", "g", "e"]), [(2, "e"), (1, "g"), (1, "e")])
        self.assertEqual(linkdiff.longest_increasing([1, 2, 3]), 3)
        self.assertEqual(linkdiff.longest_increasing([3, 1, 2, 4]), 3)
        self.assertEqual(linkdiff.longest_increasing([]), 0)


class LinkLibrariesTest(unittest.TestCase):
    @staticmethod
    def _cpio(entries):
        out = b""
        for inode, name, mode, body in entries + [(0, "TRAILER!!!", 0, b"")]:
            raw = name.encode() + b"\0"
            fields = [inode, mode, 0, 0, 1, 0, len(body), 0, 0, 0, 0, len(raw), 0]
            header = b"070701" + b"".join(b"%08X" % f for f in fields)
            out += header + raw
            out += b"\0" * (-len(out) % 4) + body
            out += b"\0" * (-len(out) % 4)
        return out

    def test_rpm_payload_with_hard_links(self):
        import gzip
        header = b"\x8e\xad\xe8\x01" + bytes(4) + (0).to_bytes(4, "big") + (0).to_bytes(4, "big")
        payload = self._cpio([(7, "./usr/bin/gcc", 0o100755, b""), (7, "./usr/bin/egcs", 0o100755, b"ELF"),
                              (8, "./usr/lib/x.o", 0o100644, b"obj")])
        rpm = b"\xed\xab\xee\xdb" + bytes(92) + header + header + gzip.compress(payload)
        files = linklibs.rpm_files(rpm)
        self.assertEqual(files["usr/bin/gcc"], (0o100755, b"ELF"))
        self.assertEqual(files["usr/bin/egcs"][1], b"ELF")
        self.assertEqual(files["usr/lib/x.o"][1], b"obj")

    def test_linked_text_layout_groups_map_inputs(self):
        lines = [".text           0x00001000      0x100",
                 " *(.text)",
                 " .text          0x00001000       0x10 /t/link/lib/crt1.o",
                 " .text          0x00001010       0x30 /t/obj/main.o",
                 " .text          0x00001040       0x20 /t/link/lib/libglade.a(glade-init.o)",
                 " .text          0x00001060       0x10 /t/link/lib/libstdc++.a(iostream.o)",
                 " .text          0x00001070       0x10 /t/link/lib/libgcc.a(_eh.o)",
                 " .gnu.linkonce.t.f__Fv",
                 "                0x00001080       0x80 /t/obj/main.o",
                 ".fini           0x00001100       0x1a"]
        layout = linkdiff.linked_text_layout("\n" + "\n".join(lines) + "\n")
        self.assertEqual(layout, {"start files": 0x10, "project objects": 0x30, "C libraries": 0x20,
                                  "libstdc++/libgcc": 0x20, "linkonce": 0x80})


class LinkGateTest(unittest.TestCase):
    def test_split_finds_an_inserted_block(self):
        image = bytes(range(1, 41))
        linked = image[:16] + b"\xee" * 8 + image[16:]
        pieces = linkdiff._split(image, linked, 0x1000, 0x1000, 0x1000, 0x1000 + len(image), 0, 8)
        self.assertEqual(pieces, [linkdiff.Piece(0x1010, 8)])

    def test_compare_accepts_only_translated_words(self):
        image = (0x2000).to_bytes(4, "little") + (0x3000).to_bytes(4, "little") + b"abcd"
        linked = (0x2040).to_bytes(4, "little") + (0x3041).to_bytes(4, "little") + b"abce"
        translate = {0x2000: 0x2040, 0x3000: 0x3040}
        explains = lambda r, o: r != o and translate.get(r) == o
        self.assertEqual(linkdiff._compare(image, linked, [(0, 12, 0)], explains), [4, 11])

    def test_stated_differences_parse(self):
        import tomllib
        with linkdiff.FACTS.open("rb") as stream:
            entries = tomllib.load(stream).get("difference", [])
        for entry in entries:
            self.assertTrue(entry["cause"].strip() and entry["evidence"].strip() and entry["closes"].strip())
            for region in entry["region"]:
                self.assertIn("bytes", region)
                self.assertEqual("start" in region, "end" in region)


if __name__ == "__main__":
    unittest.main()
