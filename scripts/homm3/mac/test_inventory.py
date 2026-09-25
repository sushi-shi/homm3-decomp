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
