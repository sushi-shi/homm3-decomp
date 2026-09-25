"""Source MAC_ADDRESS claims, the Mac function tables and parity accounting."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import addresses
from homm3.mac.pef import Section


def scan(text: str, path: str = "src/unit.cpp"):
    return addresses.scan_text(text, path)


class FakePEF:
    def __init__(self, size: int = 0x1000):
        self.size = size

    def section(self, index):
        return Section(index, 0 if index == 0 else 2, 0, self.size, self.size, self.size)

    def code(self, section, offset, size):
        if section != 0 or offset % 4 or size % 4 or offset + size > self.size:
            raise ValueError("invalid code span")
        return bytes(size)


def tables(root: Path, functions=(), runtime=(), aliases=(), dispositions=()):
    config = root / "config/mac"
    config.mkdir(parents=True, exist_ok=True)
    (config / "functions.tsv").write_text(
        "offset\tsize\n" + "".join(f"0x{o:x}\t0x{s:x}\n" for o, s in functions))
    (config / "runtime-map.tsv").write_text(
        "offset\tname\n" + "".join(f"0x{o:x}\t{n}\n" for o, n in runtime))
    (config / "runtime-aliases.tsv").write_text(
        "offset\tname\n" + "".join(f"0x{o:x}\t{n}\n" for o, n in aliases))
    (config / "dispositions.tsv").write_text(
        "identity\tdisposition\tevidence\n"
        + "".join(f"{i}\t{d}\t{e}\n" for i, d, e in dispositions))


class TestMacAddressScan(unittest.TestCase):
    def test_same_line_claim_pairs_with_its_va(self):
        claims, windows, problems = scan(
            "VA(0x004d8720, 0x568) MAC_ADDRESS(0x0f3fe4, 0x568)  // dc 0x1\n"
            "void hero::initialize(short id)\n{\n}\n")
        self.assertEqual(problems, [])
        self.assertEqual(claims[0].identity, "va:0x004d8720")
        self.assertEqual((claims[0].offset, claims[0].size), (0xf3fe4, 0x568))
        self.assertIs(windows[0].mac, claims[0])
        self.assertEqual(windows[0].label, "hero::initialize")

    def test_standalone_claim_owns_following_definition(self):
        claims, _, problems = scan(
            "// helper comment\n"
            "MAC_ADDRESS(0x0f1db4, 0x294)\n"
            "static unsigned char initializeMoveConstants()\n{\n    return 1;\n}\n")
        self.assertEqual(problems, [])
        self.assertEqual(claims[0].identity,
                         "source:src/unit.cpp:initializeMoveConstants()")

    def test_overloads_have_distinct_identities(self):
        claims, _, problems = scan(
            "MAC_ADDRESS(0x100, 0x10)\nint f(int value)\n{\n    return value;\n}\n"
            "MAC_ADDRESS(0x110, 0x10)\nint f(long value)\n{\n    return 1;\n}\n")
        self.assertEqual(problems, [])
        self.assertEqual(len({claim.identity for claim in claims}), 2)
        self.assertEqual(addresses.claim_problems(claims), [])

    def test_duplicate_identity_and_overlap_fail(self):
        claims, _, problems = scan(
            "VA(0x00401000, 0x10) MAC_ADDRESS(0x100, 0x20)\nvoid a()\n{\n}\n")
        claims += scan("VA(0x00401000, 0x10) MAC_ADDRESS(0x110, 0x20)\nvoid a()\n{\n}\n",
                       "src/other.cpp")[0]
        found = addresses.claim_problems(claims)
        self.assertTrue(any("2 Mac address claims" in item for item in found))
        self.assertTrue(any("overlaps" in item for item in found))

    def test_functions_only(self):
        _, _, problems = scan("MAC_ADDRESS(0x100, 0x10)\nint g_table[4] = { 0 };\n")
        self.assertTrue(any("functions only" in item for item in problems))
        _, _, problems = scan("MAC_ADDRESS(0x100, 0x10)\nint g_value;\n")
        self.assertTrue(any("functions only" in item for item in problems))

    def test_orphan_and_misplaced_claims_fail(self):
        _, _, problems = scan("void f()\n{\n}\nMAC_ADDRESS(0x100, 0x10)\n")
        self.assertTrue(any("orphan" in item for item in problems))
        _, _, problems = scan("VA(0x00401000, 0x10)\nMAC_ADDRESS(0x100, 0x10)\nvoid f()\n{\n}\n")
        self.assertTrue(any("VA(0x00401000)" in item for item in problems))
        _, _, problems = scan("class C {\nMAC_ADDRESS(0x100, 0x10)\npublic:\n    C(int x) : m(x) {}\n};\n")
        self.assertTrue(any("access label" in item for item in problems))
        _, _, problems = scan("int x = 1; MAC_ADDRESS(0x100, 0x10)\nvoid f()\n{\n}\n")
        self.assertTrue(any("same line" in item for item in problems))

    def test_misaligned_or_malformed_spans_fail(self):
        for text in ("MAC_ADDRESS(0x102, 0x10)\nvoid f()\n{\n}\n",
                     "MAC_ADDRESS(0x100, 0x0)\nvoid f()\n{\n}\n",
                     "MAC_ADDRESS(0x100)\nvoid f()\n{\n}\n",
                     "MAC_ADDRESS(section, 0x10)\nvoid f()\n{\n}\n"):
            self.assertTrue(scan(text)[2], text)

    def test_compgen_claim_must_agree_with_va_compgen(self):
        claims, _, problems = scan(
            "VA_COMPGEN(0x0052c570, 0x21, SCALAR_DELETING_DTOR, TPuzzleWindow) "
            "MAC_COMPGEN_ADDRESS(0x100, 0x40, SCALAR_DELETING_DTOR, TPuzzleWindow)\n")
        self.assertEqual(problems, [])
        self.assertEqual(claims[0].identity, "compgen:0x0052c570")
        _, _, problems = scan(
            "VA_COMPGEN(0x0052c570, 0x21, SCALAR_DELETING_DTOR, TPuzzleWindow) "
            "MAC_COMPGEN_ADDRESS(0x100, 0x40, IMPLICIT_DTOR, TPuzzleWindow)\n")
        self.assertTrue(any("disagrees" in item for item in problems))
        _, _, problems = scan(
            "VA_COMPGEN(0x0052c570, 0x21, SCALAR_DELETING_DTOR, TPuzzleWindow) "
            "MAC_ADDRESS(0x100, 0x40)\n")
        self.assertTrue(any("MAC_COMPGEN_ADDRESS" in item for item in problems))

    def test_comments_strings_and_macro_definitions_are_ignored(self):
        claims, _, problems = scan(
            "#define MAC_ADDRESS(offset, size)\n"
            "// MAC_ADDRESS(0x100, 0x10)\n"
            "const char* s = \"MAC_ADDRESS(0x100, 0x10)\";\n")
        self.assertEqual((claims, problems), ([], []))


class TestMacAddressTables(unittest.TestCase):
    def check(self, root, text, **kwargs):
        tables(root, **kwargs)
        claims, windows, problems = scan(text)
        self.assertEqual(problems, [])
        with patch.object(addresses, "reviewed", return_value=[]):
            return addresses.check(root, FakePEF(), claims, windows)

    def test_claim_must_resolve_to_one_function_row(self):
        text = "VA(0x00401000, 0x10) MAC_ADDRESS(0x100, 0x20)\nvoid f()\n{\n}\n"
        with tempfile.TemporaryDirectory() as folder:
            self.assertEqual(self.check(Path(folder), text, functions=[(0x100, 0x20)]), [])
        with tempfile.TemporaryDirectory() as folder:
            found = self.check(Path(folder), text, functions=[(0x100, 0x24)])
            self.assertTrue(any("is not a config/mac/functions.tsv row" in item for item in found))

    def test_out_of_bounds_and_overlapping_rows_fail(self):
        with tempfile.TemporaryDirectory() as folder:
            found = self.check(Path(folder), "", functions=[(0xff0, 0x20), (0x100, 0x20), (0x110, 0x8)])
            self.assertTrue(any("in-bounds" in item for item in found))
            self.assertTrue(any("overlaps" in item for item in found))

    def test_runtime_labels_stay_out_of_source_claims(self):
        text = "VA(0x00401000, 0x10) MAC_ADDRESS(0x100, 0x20)\nvoid f()\n{\n}\n"
        with tempfile.TemporaryDirectory() as folder:
            found = self.check(Path(folder), text, functions=[(0x100, 0x20), (0x200, 0x8)],
                               runtime=[(0x100, ".f"), (0x200, ".g"), (0x300, ".h")],
                               aliases=[(0x200, ".g")])
            self.assertTrue(any("also source claim" in item for item in found))
            self.assertTrue(any(".h at 0x300 has no" in item for item in found))
            self.assertTrue(any("duplicate name .g" in item for item in found))

    def test_disposition_cannot_hide_a_located_function(self):
        text = "VA(0x00401000, 0x10) MAC_ADDRESS(0x100, 0x20)\nvoid f()\n{\n}\n"
        with tempfile.TemporaryDirectory() as folder:
            found = self.check(Path(folder), text, functions=[(0x100, 0x20)],
                               dispositions=[("va:0x00401000", "inlined_only", "calls expand")])
            self.assertTrue(any("has a Mac address" in item for item in found))
        with tempfile.TemporaryDirectory() as folder:
            found = self.check(Path(folder), text, functions=[(0x100, 0x20)],
                               dispositions=[("va:0x00402000", "inlined_only", "calls expand")])
            self.assertTrue(any("names no source claim" in item for item in found))

    def test_dispositions_need_known_kind_and_evidence(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            tables(root, dispositions=[("va:0x00401000", "gone", "reason")])
            with self.assertRaises(addresses.AddressError):
                addresses.read_dispositions(root)
            tables(root, dispositions=[("va:0x00401000", "inlined_only", " ")])
            with self.assertRaises(addresses.AddressError):
                addresses.read_dispositions(root)


class TestCoverage(unittest.TestCase):
    def test_bytes_are_split_by_owner_and_remainder_stays_unresolved(self):
        claims, _, _ = scan("VA(0x00401000, 0x10) MAC_ADDRESS(0x100, 0x20)\nvoid f()\n{\n}\n")
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            tables(root, functions=[(0x100, 0x20), (0x200, 0x8), (0x300, 0x10)],
                   runtime=[(0x200, ".g")])
            report = addresses.coverage(root, FakePEF(0x1000), claims)
        self.assertEqual(report["rows_by_owner"], {"source": 1, "runtime": 1, "unowned": 1})
        self.assertEqual(report["covered_bytes"], 0x38)
        self.assertEqual(report["unresolved_bytes"], 0x1000 - 0x38)


class TestParityIndex(unittest.TestCase):
    def test_unannotated_functions_stay_visible_and_dispositions_apply(self):
        text = ("VA(0x00401000, 0x10) MAC_ADDRESS(0x100, 0x20)\nvoid a()\n{\n}\n"
                "VA(0x00401010, 0x10)\nvoid b()\n{\n}\n"
                "VA(0x00401020, 0x10)\nvoid c()\n{\n}\n")
        claims, windows, _ = scan(text)
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            tables(root, dispositions=[("va:0x00401020", "inlined_only",
                                        "every Mac caller expands the body")])
            with patch.object(addresses, "unit_of", return_value="unit"):
                rows, problems = addresses.index(root, claims, windows)
        self.assertEqual(problems, [])
        self.assertEqual({row["identity"]: row["state"] for row in rows},
                         {"va:0x00401000": "located", "va:0x00401010": "unlocated",
                          "va:0x00401020": "inlined_only"})

    def test_standalone_claim_must_bind_to_an_ast_definition(self):
        from homm3.match.source_ownership import Definition
        claims, windows, _ = scan("\n\nMAC_ADDRESS(0x100, 0x20)\nvoid helper()\n{\n}\n")
        definition = Definition("src/unit.cpp", 4, 0, 10, "helper", "void ()", 0,
                                False, False, None, "?helper@@YAXXZ")
        stranger = Definition("src/unit.cpp", 9, 40, 50, "other", "void ()", 0,
                              False, False, None, "?other@@YAXXZ")
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            tables(root)
            with patch.object(addresses, "unit_of", return_value="unit"):
                rows, problems = addresses.index(root, claims, windows, [definition, stranger])
                self.assertEqual(problems, [])
                states = {row["name"]: row["state"] for row in rows}
                self.assertEqual(states, {"helper": "located", "other": "unlocated"})
                _, problems = addresses.index(root, claims, windows, [stranger])
                self.assertTrue(any("binds to no unique authored definition" in item
                                    for item in problems))


class TestMigrationEdits(unittest.TestCase):
    def test_standalone_insertion_skips_a_matched_access_label(self):
        text = ("class C : public B {\npublic:\n    // comment\n"
                "    C(int value)\n        : B(value) {}\n};\n")
        at, inserted = addresses._above_definition(text, text.index("public:"),
                                                   addresses.spell(0x100, 0x8))
        updated = text[:at] + inserted + text[at:]
        self.assertIn("    // comment\n    MAC_ADDRESS(0x000100, 0x8)\n    C(int value)", updated)
        self.assertEqual(scan(updated)[2], [])

    def test_same_line_insertion_is_idempotent_and_conflicts_fail(self):
        text = "VA(0x00401000, 0x10)  // dc 0x1\nvoid f()\n{\n}\n"
        masked = addresses._mask(text)
        end = addresses._windows_ends(text, masked)[(False, 0x401000)][0]
        at, inserted = addresses._after_windows_claim(text, end, addresses.spell(0x100, 0x20))
        updated = text[:at] + inserted + text[at:]
        self.assertTrue(updated.startswith("VA(0x00401000, 0x10) MAC_ADDRESS(0x000100, 0x20)  //"))
        end = addresses._windows_ends(updated, addresses._mask(updated))[(False, 0x401000)][0]
        self.assertIsNone(addresses._after_windows_claim(updated, end, addresses.spell(0x100, 0x20)))
        with self.assertRaises(addresses.AddressError):
            addresses._after_windows_claim(updated, end, addresses.spell(0x104, 0x20))

    def test_standalone_insertion_keeps_indentation(self):
        text = "class C {\n    int get() const\n    {\n        return 1;\n    }\n};\n"
        start = text.index("int get")
        at, inserted = addresses._above_definition(text, start, addresses.spell(0x100, 0x8))
        self.assertEqual(inserted, "    MAC_ADDRESS(0x000100, 0x8)\n")
        updated = text[:at] + inserted + text[at:]
        self.assertIsNone(addresses._above_definition(
            updated, updated.index("int get"), addresses.spell(0x100, 0x8)))


class TestRatchetFingerprint(unittest.TestCase):
    def test_same_line_mac_address_is_not_part_of_the_definition(self):
        from homm3.match import status
        from homm3.retail_labels.source import mask_lexical_noise
        plain = "VA(0x00401000, 0x10)  // dc 0x1\nvoid f()\n{\n}\n"
        paired = "VA(0x00401000, 0x10) MAC_ADDRESS(0x000100, 0x20)  // dc 0x1\nvoid f()\n{\n}\n"
        texts = []
        for text in (plain, paired):
            masked = mask_lexical_noise(text)
            end = text.index(")")
            texts.append(status._definition_text(
                text, masked, status._after_windows_claim(masked, end)))
        self.assertEqual(texts[0], texts[1])


if __name__ == "__main__":
    unittest.main()
