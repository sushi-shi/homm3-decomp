"""Code-section byte accounting, boundary proofs and emitted-symbol joins."""
from pathlib import Path
import tempfile
import unittest

from homm3.mac import emitted, inventory, tables
from homm3.mac.pef import Section

MFLR, BLR, NOP = 0x7C0802A6, 0x4E800020, 0x60000000
BCTR = 0x4E800420


def code(*words: int) -> bytes:
    return b"".join(word.to_bytes(4, "big") for word in words)


def branch(at: int, target: int, link: bool = False) -> int:
    return 0x48000000 | ((target - at) & 0x03FFFFFC) | int(link)


def beq(at: int, target: int) -> int:
    return 0x41820000 | ((target - at) & 0xFFFC)


class FakePEF:
    def __init__(self, size: int):
        self.size = size

    def section(self, index):
        return Section(index, 0, 0, self.size, self.size, self.size)


class TestSingleFunctionProof(unittest.TestCase):
    def test_multi_return_body_is_one_function(self):
        body = code(MFLR, beq(4, 12), BLR, NOP, BLR)
        self.assertTrue(inventory.proves_single_function(body, 0, 20))

    def test_two_adjacent_functions_are_not_one(self):
        body = code(MFLR, BLR, MFLR, BLR)
        self.assertFalse(inventory.proves_single_function(body, 0, 16))
        self.assertTrue(inventory.proves_single_function(body, 0, 8))

    def test_calls_fall_through_and_tail_calls_end_the_path(self):
        body = code(branch(0, 0x100, link=True), NOP, branch(8, 0x200))
        self.assertTrue(inventory.proves_single_function(body, 0, 12))

    def test_falling_off_the_span_fails(self):
        self.assertFalse(inventory.proves_single_function(code(MFLR, NOP), 0, 8))

    def test_jump_table_labels_seed_reachability(self):
        body = code(MFLR, BCTR, NOP, BLR, NOP, BLR)
        self.assertFalse(inventory.proves_single_function(body, 0, 24))
        self.assertTrue(inventory.proves_single_function(body, 0, 24, [8, 16]))
        # A dispatch-only prefix is closed by bctr; labels outside it are ignored.
        self.assertTrue(inventory.proves_single_function(body, 0, 8, [8, 16]))


class TestRegions(unittest.TestCase):
    def write(self, root: Path, functions=(), regions=()):
        config = root / "config/mac"
        config.mkdir(parents=True, exist_ok=True)
        (config / "functions.tsv").write_text(
            "offset\tsize\n" + "".join(f"0x{o:x}\t0x{s:x}\n" for o, s in functions))
        (config / "code-regions.tsv").write_text(
            "start\tend\tcategory\tevidence\n"
            + "".join(f"0x{s:x}\t0x{e:x}\treadonly_data\tpool\n" for s, e in regions))

    def test_every_byte_is_in_exactly_one_region(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.write(root, functions=[(0x10, 0x10), (0x40, 0x8)], regions=[(0x80, 0x100)])
            layout = inventory.regions(root, FakePEF(0x100), source={0x10: "va:0x00401000"})
        self.assertEqual([(r.start, r.end, r.category, r.owner) for r in layout], [
            (0x0, 0x10, "unresolved", ""), (0x10, 0x20, "function", "source"),
            (0x20, 0x40, "unresolved", ""), (0x40, 0x48, "function", "unowned"),
            (0x48, 0x80, "unresolved", ""), (0x80, 0x100, "readonly_data", "")])

    def test_overlapping_regions_fail(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.write(root, functions=[(0x10, 0x80)], regions=[(0x80, 0x100)])
            with self.assertRaises(tables.TableError):
                inventory.regions(root, FakePEF(0x100), source={})

    def test_rows_must_not_contain_another_entry(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.write(root, functions=[(0x10, 0x20)])
            leads = {0x10: {"tvector_entry"}, 0x18: {"code_pointer"}, 0x20: {"call_target"}}
            problems = inventory.entry_conflicts(root, leads)
        self.assertEqual(len(problems), 1)
        self.assertIn("call_target entry 0x20", problems[0])


class TestVtableSlots(unittest.TestCase):
    def test_reviewed_slots_must_be_verified_rows(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            TestRegions().write(root, functions=[(0x100, 0x20)])
            (root / "config/mac/vtables").mkdir(parents=True)
            (root / "config/mac/vtables/unit.toml").write_text(
                '[[vtables]]\nsymbol = "__vt__4Unit"\n'
                '[[vtables.slots]]\ncode_section = 0\ncode_offset = 0x100\ncode_size = 0x20\n'
                '[[vtables.slots]]\ncode_section = 0\ncode_offset = 0x200\ncode_size = 0x10\n')
            problems = inventory.vtable_problems(root)
            self.assertEqual(len(problems), 1)
            self.assertIn("slot 1 code 0x200+0x10", problems[0])
            self.assertEqual([slot["offset"] for slot in inventory.vtable_slots(root)], [0x100, 0x200])


class TestEmittedJoin(unittest.TestCase):
    def test_codewarrior_names_demangle_to_qualified_names(self):
        cases = {
            ".getExperience__4heroFi": "hero::getExperience",
            ".getDescription__13type_artifactCFv": "type_artifact::getDescription",
            ".__ct__Q23std6stringFRCQ23std6string": "std::string::string",
            ".__dt__4heroFv": "hero::~hero",
            ".__eq__Q23std10bitset<19>CFRCQ23std10bitset<19>": "std::bitset<19>::operator==",
            ".markSpells__F12TSpellSchool": "markSpells",
            ".adler32": "adler32",
        }
        for symbol, qualified in cases.items():
            self.assertEqual(emitted.demangle(symbol), qualified, symbol)
        self.assertEqual(emitted.key("TResourceHandle<T>::get<int>"), "TResourceHandle::get")

    def test_owner_classification(self):
        from homm3.match.source_ownership import Definition
        own = Definition("src/hero.cpp", 1, 0, 1, "hero::getLevel", "int ()", 0, True, False,
                         None, "?getLevel@hero@@QAEHXZ")
        header = Definition("include/hero.h", 1, 0, 1, "hero::getId", "int ()", 0, True, True,
                            None, "?getId@hero@@QAEHXZ")
        self.assertEqual(emitted.classify("hero::getLevel", [own], "src/hero.cpp"), "unit_source")
        self.assertEqual(emitted.classify("hero::getId", [header], "src/hero.cpp"), "header")
        self.assertEqual(emitted.classify("std::string::string", [], "src/hero.cpp"), "library_template")
        self.assertEqual(emitted.classify("hero::hero", [], "src/hero.cpp", ".__ct__4heroFv"),
                         "compiler_generated")
        self.assertEqual(emitted.classify("adler32", [], "vendor/zlib-1.1.3/adler32.c"), "vendor")
        self.assertEqual(emitted.classify("helper", [], "src/hero.cpp", ".helper__Fv"), "no_source_owner")


class TestClaimBodies(unittest.TestCase):
    def test_claims_bind_to_definitions_and_emitted_hunks(self):
        import json
        from homm3.mac import addresses
        from homm3.match.source_ownership import Definition
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "config").mkdir()
            (root / "config/units.toml").write_text(
                '[build]\n[flags]\nx = []\n'
                '[[unit]]\nunit = "hero"\nsource = "src/hero.cpp"\nflags = "x"\n'
                '[[unit]]\nunit = "town"\nsource = "src/town.cpp"\nflags = "x"\n')
            (root / "build/mac/obj").mkdir(parents=True)
            (root / "build/mac/obj/hero.hunks.json").write_text(json.dumps({"code": [
                {"symbol": ".getLevel__4heroFi", "size": 0x40, "references": []},
                {"symbol": ".getLevel__4heroFv", "size": 0x10, "references": []}]}))
            (root / "build/mac/obj/hero.o").write_bytes(b"MWOBPPC ")
            (root / "build/mac/obj/town.hunks.json").write_text('{"code": []}')  # stale index, no object
            claims = addresses.scan_text(
                "VA(0x004d0000, 0x10) MAC_ADDRESS(0x100, 0x40)\nint hero::getLevel(int x)\n{\n}\n"
                "MAC_ADDRESS(0x200, 0x8)\nstatic int helper()\n{\n}\n", "src/hero.cpp")[0]
            claims += addresses.scan_text(
                "VA(0x005d0000, 0x10) MAC_ADDRESS(0x300, 0x20)\nvoid town::build()\n{\n}\n",
                "src/town.cpp")[0]
            level = Definition("src/hero.cpp", 2, 0, 1, "hero::getLevel", "int (int)", 1, True, False,
                               0x004D0000, "?getLevel@hero@@QAEHH@Z", argument_types=("int",))
            helper = Definition("src/hero.cpp", 6, 50, 60, "helper", "int ()", 0, False, False,
                                None, "?helper@@YAHXZ", mac_offset=0x200, mac_size=0x8)
            build = Definition("src/town.cpp", 2, 0, 1, "town::build", "void ()", 0, True, False,
                               0x005D0000, "?build@town@@QAEXXZ")
            rows = {row["identity"]: row for row in
                    emitted.claim_bodies(root, [level, helper, build], claims)}
        self.assertEqual(rows["va:0x004d0000"]["state"], "emitted")
        self.assertEqual(rows["va:0x004d0000"]["symbol"], ".getLevel__4heroFi")  # overload by parameters
        self.assertEqual(rows["va:0x004d0000"]["symbol_size"], "0x40")
        self.assertEqual(rows["source:src/hero.cpp:helper()"]["state"], "not_emitted")
        self.assertEqual(rows["va:0x005d0000"]["state"], "no_object")


class TestZlibMap(unittest.TestCase):
    def test_labels_need_a_function_row_and_a_unique_owner(self):
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            config = root / "config/mac"
            config.mkdir(parents=True)
            (config / "functions.tsv").write_text("offset\tsize\n0x100\t0x20\n")
            (config / "runtime-map.tsv").write_text(
                "offset\tname\towner\tcall_kind\tevidence\n0x100\t.f\tmsl_c\tdirect\treviewed\n")
            (config / "zlib-map.tsv").write_text("offset\tname\tunit\n0x100\t.adler32\tadler32\n"
                                                 "0x200\t.crc32\tcrc32\n")
            with patch("homm3.mac.glue.stubs", return_value=[]):
                problems = tables.validate(root, FakePEF(0x1000))
        self.assertTrue(any("also a runtime or glue label" in item for item in problems))
        self.assertTrue(any(".crc32 at 0x200 has no" in item for item in problems))


if __name__ == "__main__":
    unittest.main()


class TestParameterJoin(unittest.TestCase):
    def test_codewarrior_parameters_and_clang_spellings_agree(self):
        self.assertEqual(emitted.parameters(".initialize__4heroFPC9HeroExtra"),
                         (False, ("PC9HeroExtra",)))
        self.assertEqual(emitted.parameters(".total__9armyGroupCFv"), (True, ()))
        self.assertEqual(emitted.parameters(".__ct__6buttonFlPFR7message_ii"),
                         (False, ("l", "PFR7message_i", "i")))
        self.assertEqual(emitted.encode("const HeroExtra *"), "PC9HeroExtra")
        self.assertEqual(emitted.encode("const type_icon_definition &"), "RC20type_icon_definition")
        self.assertEqual(emitted.encode("A::B &"), "RQ21A1B")
        self.assertIsNone(emitted.encode("int (*)(message &)"))

    def test_owner_picks_the_one_fitting_overload(self):
        from homm3.match.source_ownership import Definition
        short = Definition("src/hero.cpp", 1, 0, 1, "hero::initialize", "void (short)", 1, True,
                           False, 0x4d8720, "", argument_types=("short",))
        extra = Definition("src/hero.cpp", 9, 2, 3, "hero::initialize", "void (const HeroExtra *)", 1,
                           True, False, 0x4d8b30, "", argument_types=("const HeroExtra *",))
        self.assertIs(emitted.owner(".initialize__4heroFs", [short, extra]), short)
        self.assertIs(emitted.owner(".initialize__4heroFPC9HeroExtra", [short, extra]), extra)
        self.assertEqual(emitted.select(extra, [".initialize__4heroFs",
                                                ".initialize__4heroFPC9HeroExtra"]),
                         ".initialize__4heroFPC9HeroExtra")
