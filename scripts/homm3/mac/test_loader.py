"""PEF decompression, relocation provenance and TOC matching contracts."""
from hashlib import sha256
from pathlib import Path
import struct
import tempfile
import unittest

from homm3.mac.loader import ImportedAddress, Loader
from homm3.mac.object import CodeHunk, DataHunk, ObjectError, parse_data_hunks, parse_metadata_hunks
from homm3.mac.pef import PEF, PEFError, unpack_data
from homm3.mac.relocations import Address, TocBinding, link_code
from homm3.mac.toc import bindings


def container(data, relocations, *, code_default=0, imports=(), packed=False, code=None):
    """Create an independently encoded PEF fixture, with code/data/loader sections."""
    strings = b"Library\0" + b"".join(name.encode() + b"\0" for name in imports)
    libraries = int(bool(imports))
    table_end = 56 + libraries * 24 + len(imports) * 4 + 12
    stream = struct.pack(f">{len(relocations)}H", *relocations)
    strings_at = table_end + len(stream)
    loader = struct.pack(">iIiIiI8I", 1, 0, -1, 0, -1, 0, libraries, len(imports), 1,
                         table_end, strings_at, strings_at + len(strings), 0, 0)
    if imports:
        loader += struct.pack(">5IBBH", 0, 0, 0, len(imports), 0, 0, 0, 0)
        name_offset = 8
        for name in imports:
            loader += struct.pack(">I", (2 << 24) | name_offset)
            name_offset += len(name) + 1
    loader += struct.pack(">HHII", 1, 0, len(relocations), 0) + stream + strings
    raw_data = bytes([0x20, len(data)]) + data if packed else data
    code = bytes(256) if code is None else code
    parts = [(0, code, len(code), code_default),
             (2 if packed else 1, raw_data, len(data), 0), (4, loader, 0, 0)]
    header = b"Joy!peffpwpc" + struct.pack(">5IHHI", 1, 0, 0, 0, 0, 3, 2, 0)
    offset = 40 + 28 * len(parts)
    for kind, payload, unpacked, default in parts:
        header += struct.pack(">iIIIIIBBBB", -1, default, unpacked, unpacked,
                              len(payload), offset, kind, 1, 2, 0)
        offset += len(payload)
    return PEF(header + b"".join(part[1] for part in parts))


class TestPefLoader(unittest.TestCase):
    def test_all_packed_data_operations(self):
        cases = [(b"\x03", bytes(3)), (b"\x23abc", b"abc"),
                 (b"\x42\x02xy", b"xyxyxy"),
                 (b"\x61\x02\x02xabcd", b"xabxcdx"),
                 (b"\x82\x01\x02ab", b"\0\0a\0\0b\0\0"),
                 (b"\x00\x81\x00", bytes(128))]
        for packed, expected in cases:
            with self.subTest(packed=packed):
                self.assertEqual(unpack_data(packed, len(expected)), expected)
        for packed, size in [(b"\x23ab", 3), (b"\x03", 2), (b"\x03", 4),
                             (b"\xa1", 1), (b"\x00\x80", 0),
                             (b"\x00\xff\xff\xff\xff\xff", 0)]:
            with self.subTest(packed=packed), self.assertRaises(PEFError):
                unpack_data(packed, size)

    def test_compression_can_be_larger_than_initialized_data(self):
        pef = container(bytes(8), (), packed=True)
        self.assertEqual(pef.contents(1), bytes(8))
        self.assertGreater(pef.section(1).packed_size, pef.section(1).unpacked_size)

    def test_symbolic_relocations_repeats_imports_and_default_base(self):
        data = bytearray(256)
        struct.pack_into(">IIII", data, 0, 0x110, 0x40, 4, 8)
        for at in (0x14, 0x18, 0x1c, 0x34, 0x38):
            struct.pack_into(">I", data, at, 0x80)
        struct.pack_into(">I", data, 0x30, 0x120)
        loader = Loader(container(bytes(data), (
            0x4600, 0x6001, 0x4a00, 0x8003, 0x4200, 0x9001,
            0xa000, 0x0030, 0x6600, 0xb400, 0x0001, 0xb040, 0x0001),
            code_default=0x100, imports=("a", "b", "c")))
        self.assertEqual(loader.pointers[Address(1, 0)], Address(0, 0x10))
        self.assertEqual(loader.toc(), Address(1, 0x40))
        self.assertEqual(loader.pointers[Address(1, 8)], ImportedAddress(loader.imports[1], 4))
        self.assertEqual(loader.pointers[Address(1, 12)], ImportedAddress(loader.imports[2], 8))
        self.assertEqual(loader.imports[1].library, "Library")
        for at in (0x14, 0x18, 0x1c, 0x34, 0x38):
            self.assertEqual(loader.pointers[Address(1, at)], Address(1, 0x80))
        self.assertEqual(loader.pointers[Address(1, 0x30)], Address(0, 0x20))
        self.assertEqual(len(loader.pointers), 10)

    def test_invalid_relocation_streams_are_errors(self):
        cases = [(0xa000,), (0xc000,), (0x6000,), (0x9000,),
                 (0xa000, 0x0000, 0x9000),  # Repeat starts in a wide operand.
                 (0x4200, 0x9000, 0x9000),  # Nested repeat.
                 (0x4200, 0xa000, 0, 0x4200),  # Double relocation.
                 (0xa000, 0x100, 0x4200)]  # Write past section end.
        for chunks in cases:
            with self.subTest(chunks=chunks), self.assertRaises(PEFError):
                Loader(container(bytes(256), chunks))

    def test_data_hunks_keep_classes_and_pointer_references(self):
        listing = '''Hunk: Kind=HUNK_LOCAL_IDATA Align=8 Class=TD Name="@14"(1) Size=8
00000000: 43 30 00 00 80 00 00 00  'C0......'
Hunk: Kind=HUNK_LOCAL_IDATA Align=4 Class=TC Name="@14"(1) Size=4
00000000: 00 00 00 00  '....'
XRef: Kind=HUNK_XREF_32BIT Offset=$00000000 Class=RW Name="@14"(1)
'''
        hunks = parse_data_hunks(listing)
        self.assertEqual([h.storage_class for h in hunks], ["TD", "TC"])
        self.assertEqual(hunks[0].data, bytes.fromhex("4330000080000000"))
        self.assertEqual(hunks[1].xrefs, ((0, "HUNK_XREF_32BIT", "@14"),))
        with self.assertRaises(ObjectError):
            parse_data_hunks(listing.replace("Size=8", "Size=12"))

    def test_truncated_data_is_unavailable_instead_of_zero_filled(self):
        listing = '''Hunk: Kind=HUNK_GLOBAL_IDATA Align=4 Class=RO Name="large"(1) Size=32
00000000: 01 02 03 04
  ...
0000001C: AA BB CC DD
'''
        hunk, = parse_data_hunks(listing)
        self.assertIsNone(hunk.data)
        self.assertEqual(hunk.declared_size, 32)
        self.assertTrue(hunk.initialized)
        with self.assertRaises(ObjectError):
            parse_data_hunks(listing.replace('  ...\n', ''))
        with self.assertRaises(ObjectError):
            parse_data_hunks(listing.replace('0000001C', '00000020'))

    def test_exception_table_metadata_is_explicit_and_not_fabricated_data(self):
        listing = '''Hunk: Kind=HUNK_LOCAL_IDATA Align=2 Class=TB Name="@12"(1) Size=18
Saved Registers: r30-r31
Small Exception Table Ranges
000002: PC=00000048 Action=00000A
00000A: DESTROYLOCAL Local=56(SP).
XRef: Kind=HUNK_XREF_32BIT Offset=$0000000E Class=DS Name="destructor"(2)
Hunk: Kind=HUNK_LOCAL_IDATA Align=4 Class=TI Name="@13"(3) Size=4
00000000: 00 00 00 00
'''
        self.assertEqual([h.storage_class for h in parse_data_hunks(listing)], ["TI"])
        metadata, = parse_metadata_hunks(listing)
        self.assertEqual((metadata.size, metadata.comparison), (18, "not_compared"))
        self.assertEqual(metadata.xrefs, ((14, "HUNK_XREF_32BIT", "destructor"),))
        with self.assertRaises(ObjectError):
            parse_metadata_hunks(listing.replace("Size=18", "Size=16"))

    def test_external_global_is_bound_without_inventing_initializer(self):
        # Separate-TU MWLink control: an IL global in TOC range relaxes to addi,
        # including a pointer variable whose contents themselves are relocated.
        data = bytearray(128)
        struct.pack_into(">III", data, 0, 0, 0x40, 0x50)
        struct.pack_into(">I", data, 0x50, 0x60)
        pef = container(bytes(data), (0x4600, 0x4200, 0xa000, 0x50, 0x4200))
        code = CodeHunk(".reader", bytes.fromhex("80620000806300004e800020"),
                        ((0, "HUNK_XREF_16BIT_IL", "pointer"),))
        hunks = (DataHunk("pointer", "TC", bytes(4), ((0, "HUNK_XREF_32BIT", "pointer"),)),)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            (root / "globals.h").write_text("DATA(0x00400100) extern int* pointer;\n")
            (root / "config/mac/data.toml").write_text(f'''[[data]]
retail_va=0x00400100
source="globals.h"
mac_section=1
mac_offset=0x50
mac_size=4
sha256="{sha256(data[0x50:0x54]).hexdigest()}"
evidence="separate-TU external pointer control"
declaration_only=true
''')
            toc = bindings(root, pef, code, hunks)
            self.assertEqual(toc["pointer"], TocBinding(16, Address(1, 0x50), False, True, True))
            linked = link_code(code, Address(0, 0), {}, collapse_reloads=True, toc=toc)
            self.assertEqual(linked.data, bytes.fromhex("38620010806300004e800020"))
            self.assertTrue(linked.data_references[0].declaration_only)
            with self.assertRaises(ObjectError):
                bindings(root, pef, code, (*hunks, DataHunk("pointer", "RW", bytes(4), ())))

    def test_same_tu_udata_requires_zero_storage_and_direct_toc(self):
        listing = '''Hunk: Kind=HUNK_LOCAL_UDATA Align=1 Class=TD Name="viewFlag"(1) Size=1
'''
        hunk, = parse_data_hunks(listing)
        self.assertFalse(hunk.initialized)
        self.assertEqual(hunk.data, b"\0")
        anchor, = parse_data_hunks(
            'Hunk: Kind=HUNK_GLOBAL_IDATA Align=4 Class=TC Name="TOC"(2) Size=0\n')
        self.assertTrue(anchor.initialized)
        self.assertEqual(anchor.data, b"")
        with self.assertRaises(ObjectError):
            parse_data_hunks(listing + "00000000: 00\n")
        with self.assertRaises(ObjectError):
            parse_data_hunks(listing + 'XRef: Kind=HUNK_XREF_32BIT Offset=$00000000 Class=TD Name="other"(2)\n')

        data = bytearray(128)
        struct.pack_into(">II", data, 0, 0, 0x40)
        pef = container(bytes(data), (0x4600, 0x4200))
        code = CodeHunk(".reader", bytes.fromhex("880200004e800020"),
                        ((0, "HUNK_XREF_16BIT", "viewFlag"),))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            (root / "source.cpp").write_text("DATA(0x00400100) static unsigned char viewFlag;\n")
            manifest = root / "config/mac/data.toml"
            zero_digest = sha256(bytes(1)).hexdigest()
            manifest.write_text(f'''[[data]]
retail_va=0x00400100
source="source.cpp"
same_tu_definition=true
mac_section=1
mac_offset=0x50
mac_size=1
sha256="{zero_digest}"
evidence="same-TU direct zero byte"
''')
            toc = bindings(root, pef, code, (hunk,))
            self.assertEqual(toc["viewFlag"], TocBinding(16, Address(1, 0x50), False))
            linked = link_code(code, Address(0, 0), {}, collapse_reloads=True, toc=toc)
            self.assertEqual(linked.data, bytes.fromhex("880200104e800020"))
            with self.assertRaises(ObjectError):
                bindings(root, pef, CodeHunk(code.name, code.data,
                         ((0, "HUNK_XREF_16BIT_IL", "viewFlag"),)), (hunk,))
            with self.assertRaises(ObjectError):
                bindings(root, pef, code, (DataHunk("viewFlag", "TD", b"\0", ()),))
            manifest.write_text(manifest.read_text().replace(
                'mac_size=1', 'mac_size=2').replace(zero_digest,
                                                  sha256(bytes(2)).hexdigest()))
            with self.assertRaises(ObjectError):
                bindings(root, pef, code, (hunk,))
            manifest.write_text(manifest.read_text().replace(
                'mac_size=2', 'mac_size=1').replace(sha256(bytes(2)).hexdigest(),
                                                  zero_digest))
            changed = bytearray(data)
            changed[0x50] = 1
            manifest.write_text(manifest.read_text().replace(zero_digest,
                                                        sha256(bytes([1])).hexdigest()))
            with self.assertRaises(ObjectError):
                bindings(root, container(bytes(changed), (0x4600, 0x4200)), code, (hunk,))

    def test_address_only_source_owners_and_worker_manifest_conflicts(self):
        from homm3.mac.source import SourceError, load_data
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            folder = root / "config/mac/data"
            folder.mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            path = folder / "worker.toml"
            row = ('[[data]]\nretail_va=0x00400100\nsource="source.cpp"\n'
                   'declaration_only=true\nmac_section=1\nmac_offset=16\nmac_size=4\n'
                   f'sha256="{sha256(bytes(4)).hexdigest()}"\nevidence="address proof"\n')
            path.write_text(row)
            for declaration, name in (("int*pointer;", "pointer"),
                                      ("extern int *pointer;", "pointer"),
                                      ("extern const Traits (&traits)[150];", "traits"),
                                      ("Slot slots[8] = { 1, 2 };", "slots"),
                                      ("Context contexts[4];", "contexts")):
                with self.subTest(declaration=declaration):
                    (root / "source.cpp").write_text("DATA(0x00400100) " + declaration)
                    pair, = load_data(root)
                    self.assertEqual(pair.name, name)
                    self.assertTrue(pair.definition.startswith("extern "))
                    self.assertNotIn("=", pair.definition)
            other = folder / "second.toml"
            other.write_text(row)
            self.assertEqual(len(load_data(root)), 1)
            other.write_text(row.replace("mac_offset=16", "mac_offset=20"))
            with self.assertRaises(SourceError):
                load_data(root)

    def test_direct_toc_address_fixup_preserves_opcode_and_checks_addend(self):
        code = CodeHunk(".format", bytes.fromhex("388200004e800020"),
                        ((0, "HUNK_XREF_16BIT", "@format"),))
        toc = {"@format": TocBinding(-0x3378, Address(1, 0x4c88), False)}
        result = link_code(code, Address(0, 0), {}, collapse_reloads=True, toc=toc)
        self.assertEqual(result.data, bytes.fromhex("3882cc884e800020"))
        for word in ("38820001", "38830000"):
            with self.subTest(word=word), self.assertRaises(ObjectError):
                link_code(CodeHunk(code.name, bytes.fromhex(word + "4e800020"), code.xrefs),
                          Address(0, 0), {}, collapse_reloads=True, toc=toc)

    def test_toc_reference_uses_loader_destination_and_literal_payload(self):
        payload = bytes.fromhex("3ff3333333333333")
        data = bytearray(128)
        struct.pack_into(">III", data, 0, 0, 0x40, 0x50)
        data[0x50:0x58] = payload
        pef = container(bytes(data), (0x4600, 0x4200))
        code = CodeHunk(".reader", bytes.fromhex("80620000c80200004e800020"), (
            (0, "HUNK_XREF_16BIT_IL", "@14"), (4, "HUNK_XREF_16BIT", "@12")))
        hunks = (DataHunk("@14", "TD", payload, ()), DataHunk("@12", "TD", payload, ()),
                 DataHunk("@14", "TC", bytes(4), ((0, "HUNK_XREF_32BIT", "@14"),)))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            manifest = root / "config/mac/data.toml"
            manifest.write_text(f'''[[constants]]
mac_section = 1
mac_offset = 80
mac_size = 8
sha256 = "{sha256(payload).hexdigest()}"
evidence = "fixture"
''')
            toc = bindings(root, pef, code, hunks)
            self.assertEqual(toc["@14"], TocBinding(-56, Address(1, 0x50), True))
            self.assertEqual(toc["@12"], TocBinding(16, Address(1, 0x50), False))
            linked = link_code(code, Address(0, 0), {}, collapse_reloads=True, toc=toc)
            self.assertEqual(linked.data, bytes.fromhex("8062ffc8c80200104e800020"))
            # Unlike a TD compiler literal, an IL-marked RW address in range
            # is relaxed by MWLink from lwz to addi.
            rw_hunks = (DataHunk("@14", "RW", payload, ()), *hunks[1:])
            relaxed = bindings(root, pef, code, rw_hunks)
            self.assertEqual(relaxed["@14"], TocBinding(16, Address(1, 0x50), False, True))
            linked = link_code(code, Address(0, 0), {}, collapse_reloads=True, toc=relaxed)
            self.assertEqual(linked.data, bytes.fromhex("38620010c80200104e800020"))
            # Equal literal bytes in different TUs are distinct reviewed pools.
            # A unit scope must select one; it never guesses between two spans.
            original = manifest.read_text()
            data[0x60:0x68] = payload
            scoped_pef = container(bytes(data), (0x4600, 0x4200))
            manifest.write_text(original + original.replace("mac_offset = 80", "mac_offset = 96")
                                .replace('[[constants]]', '[[constants]]\nunits=["other"]'))
            self.assertIn("@12", bindings(root, scoped_pef, code, hunks, unit="current"))
            with self.assertRaises(ObjectError):
                bindings(root, scoped_pef, code, hunks, unit="other")
            manifest.write_text(original.replace('[[constants]]', '[[constants]]\nunits=["current"]')
                                + original.replace("mac_offset = 80", "mac_offset = 96")
                                .replace('[[constants]]', '[[constants]]\nunits=["other"]'))
            self.assertEqual(bindings(root, scoped_pef, code, hunks, unit="current")["@12"].target,
                             Address(1, 0x50))
            manifest.write_text(original)
            # The pointer cell must actually relocate to this constant.
            changed = bytearray(data)
            struct.pack_into(">I", changed, 8, 0x58)
            with self.assertRaises(ObjectError):
                bindings(root, container(bytes(changed), (0x4600, 0x4200)), code, hunks)
            # An arbitrary @number never authorizes a different payload.
            wrong = (DataHunk("@14", "TD", bytes(8), ()), *hunks[1:])
            with self.assertRaises(ObjectError):
                bindings(root, pef, code, wrong)
            with self.assertRaises(ObjectError):
                link_code(code, Address(0, 0), {}, collapse_reloads=True)

    def test_identical_anonymous_literals_require_reviewed_use_sites(self):
        data = bytearray(128)
        struct.pack_into(">II", data, 0, 0, 0x40)
        code_bytes = bytearray(256)
        struct.pack_into(">II", code_bytes, 0x20, 0x88020010, 0x88020011)
        pef = container(bytes(data), (0x4600, 0x4200), code=bytes(code_bytes))
        code = CodeHunk(".clear", bytes.fromhex("8802000088020000"), (
            (0, "HUNK_XREF_16BIT", "@20"), (4, "HUNK_XREF_16BIT", "@21")))
        hunks = (DataHunk("@20", "TD", b"\0", ()), DataHunk("@21", "TD", b"\0", ()))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            manifest = root / "config/mac/data.toml"
            zero_digest = sha256(bytes(1)).hexdigest()

            def row(candidate, target, address):
                return ("[[constants]]\nunits = [\"hero\"]\nowner_va = 0x4d8720\n"
                        f"candidate_offset = {candidate}\ntarget_offset = {target}\n"
                        f"mac_section = 1\nmac_offset = {address}\nmac_size = 1\n"
                        f"sha256 = \"{zero_digest}\"\n"
                        "evidence = \"reviewed source order and target TOC load\"\n")

            manifest.write_text(row(0, 0, 0x50) + row(4, 4, 0x51))
            bound = bindings(root, pef, code, hunks, unit="hero", retail_va=0x4d8720,
                             target_origin=Address(0, 0x20), target_size=8)
            self.assertEqual(bound["@20"].target, Address(1, 0x50))
            self.assertEqual(bound["@21"].target, Address(1, 0x51))
            linked = link_code(code, Address(0, 0x20), {}, collapse_reloads=True, toc=bound)
            self.assertEqual(linked.data, bytes.fromhex("8802001088020011"))
            manifest.write_text(row(0, 4, 0x50) + row(4, 0, 0x51))
            with self.assertRaises(ObjectError):
                bindings(root, pef, code, hunks, unit="hero", retail_va=0x4d8720,
                         target_origin=Address(0, 0x20), target_size=8)

            # One emitted literal can have two uses. Both target loads must
            # identify the same TOC address, and no use may be left unreviewed.
            struct.pack_into(">I", code_bytes, 0x24, 0x88020010)
            multi_pef = container(bytes(data), (0x4600, 0x4200), code=bytes(code_bytes))
            multi_code = CodeHunk(".clear", code.data, (
                (0, "HUNK_XREF_16BIT", "@20"), (4, "HUNK_XREF_16BIT", "@20")))
            manifest.write_text(row(0, 0, 0x50) + row(4, 4, 0x50))
            bound = bindings(root, multi_pef, multi_code, hunks[:1], unit="hero",
                             retail_va=0x4d8720, target_origin=Address(0, 0x20), target_size=8)
            self.assertEqual(bound["@20"].target, Address(1, 0x50))
            manifest.write_text(row(0, 0, 0x50))
            with self.assertRaises(ObjectError):
                bindings(root, multi_pef, multi_code, hunks[:1], unit="hero",
                         retail_va=0x4d8720, target_origin=Address(0, 0x20), target_size=8)

    def test_site_bounded_indirect_literal_requires_loader_pointer(self):
        data = bytearray(128)
        struct.pack_into(">II", data, 0, 0, 0x40)
        struct.pack_into(">I", data, 0x60, 0x70)
        data[0x70] = 0x0a
        code_bytes = bytearray(256)
        struct.pack_into(">I", code_bytes, 0x20, 0x80620020)  # lwz r3,0x20(r2)
        relocations = (0x4600, 0x4200, 0x8057, 0x4200)
        pef = container(bytes(data), relocations, code=bytes(code_bytes))
        code = CodeHunk(".clear", bytes.fromhex("80620000"),
                        ((0, "HUNK_XREF_16BIT_IL", "@22"),))
        payload = DataHunk("@22", "RW", b"\x0a", ())
        cell = DataHunk("@22", "TC", bytes(4),
                        ((0, "HUNK_XREF_32BIT", "@22"),))
        lf_digest = sha256(bytes([0x0a])).hexdigest()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            (root / "config/mac/data.toml").write_text(
                "[[constants]]\nunits = [\"hero\"]\nowner_va = 0x4d8720\n"
                "candidate_offset = 0\ntarget_offset = 0\n"
                "mac_section = 1\nmac_offset = 0x70\nmac_size = 1\n"
                f"sha256 = \"{lf_digest}\"\n"
                "evidence = \"reviewed indirect literal and TOC cell\"\n")
            bound = bindings(root, pef, code, (payload, cell), unit="hero",
                             retail_va=0x4d8720, target_origin=Address(0, 0x20),
                             target_size=4)
            self.assertEqual(bound["@22"].target, Address(1, 0x70))
            self.assertTrue(bound["@22"].indirect)
            linked = link_code(code, Address(0, 0x20), {}, collapse_reloads=True,
                               toc=bound)
            self.assertEqual(linked.data, bytes.fromhex("80620020"))
            with self.assertRaises(ObjectError):
                bindings(root, pef, code, (payload,), unit="hero",
                         retail_va=0x4d8720, target_origin=Address(0, 0x20),
                         target_size=4)
            struct.pack_into(">I", data, 0x60, 0x71)
            wrong = container(bytes(data), relocations, code=bytes(code_bytes))
            with self.assertRaises(ObjectError):
                bindings(root, wrong, code, (payload, cell), unit="hero",
                         retail_va=0x4d8720, target_origin=Address(0, 0x20),
                         target_size=4)


if __name__ == "__main__":
    unittest.main()
