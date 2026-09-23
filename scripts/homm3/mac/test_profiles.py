"""Grouped compilation must track context without changing own-source MAX."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import build, profiles
from homm3.mac.source import candidate_source, load_pairs, source_identity


class TestMacProfiles(unittest.TestCase):
    def fixture(self, root):
        (root / "src").mkdir()
        (root / "config/mac").mkdir(parents=True)
        (root / "include").mkdir()
        (root / "include/compiler.h").write_text("// Compiler compatibility\n")
        (root / "config/mac/sdk.toml").write_text("trees=[]\n")
        (root / "include/value.h").write_text("#define VALUE 1\n")
        (root / "include/test.h").write_text(
            '// Ordinary project header\n#include "value.h"\nint first();\nint second();\n')
        (root / "src/test.cpp").write_text(
            '#include "test.h"\n'
            "VA(0x00400100, 4)\nint first() { return VALUE; }\n"
            "VA(0x00400200, 4)\nint second() { return first(); }\n")
        (root / "config/units.toml").write_text(
            '[[unit]]\nunit="test"\nsource="src/test.cpp"\n')
        (root / "config/mac/units.toml").write_text(
            '[units.test]\nmode="paired_bodies"\n'
            'flags=["-O1", "-nolink"]\n')
        (root / "config/mac/functions.toml").write_text("\n".join(
            f'[[functions]]\nretail_va={va}\nunit="test"\nsource="src/test.cpp"\n'
            f'mac_section=0\nmac_offset={at}\nmac_size=4\nmac_symbol=".{name}"\nevidence="control"'
            for va, at, name in [(0x400200, 4, "second"), (0x400100, 0, "first")]))
        return sorted(load_pairs(root), key=lambda pair: pair.retail_va)

    def test_source_order_neighbor_context_and_header_identity(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            first, second = self.fixture(root)
            source = candidate_source(first)
            self.assertEqual(source, candidate_source(second))
            self.assertLess(source.index("int first()"), source.index("int second()"))
            identity = source_identity(first, source.encode())
            path = root / "src/test.cpp"
            path.write_text(path.read_text().replace("return first();", "return first() + 1;"))
            changed = candidate_source(first)
            self.assertNotEqual(source, changed)
            self.assertEqual(identity, source_identity(first, changed.encode()))
            (root / "include/value.h").write_text("#define VALUE 2\n")
            self.assertNotEqual(identity, source_identity(first, changed.encode()))

    def test_va_redeclaration_uses_earlier_canonical_body_for_identity(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.fixture(root)
            source_path = root / "src/test.cpp"
            source_path.write_text(
                "VA(0x00400100, 4)\nint first() { return VALUE; }\n"
                "int second() { return first(); }\n"
                "VA(0x00400200, 4)\nint second();\n"
                "int unrelated() { return 7; }\n")
            profile_path = root / "config/mac/units.toml"
            profile_path.write_text(profile_path.read_text() + 'source_helpers=["second"]\n')
            second = next(pair for pair in load_pairs(root) if pair.retail_va == 0x400200)
            generated = candidate_source(second)
            self.assertEqual(generated.count("int second() {"), 1)
            self.assertIn("int second();", generated)
            self.assertNotIn("int unrelated()", generated)
            before = source_identity(second, generated.encode())
            source_path.write_text(source_path.read_text().replace("return first();", "return first() + 1;"))
            self.assertNotEqual(before, source_identity(second, candidate_source(second).encode()))
            after = source_identity(second, candidate_source(second).encode())
            source_path.write_text(source_path.read_text().replace("return 7;", "return 8;"))
            self.assertEqual(after, source_identity(second, candidate_source(second).encode()))

    def test_cache_shared_across_selectors_but_not_tampered_headers(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            first, second = self.fixture(root)
            compiles = []

            def run(command, cwd, env):
                if command[1].endswith("MWCPPC.exe"):
                    compiles.append(command)
                    (cwd / "candidate.o").write_bytes(b"MWOBPPC " + bytes([len(compiles)]))
                    return ""
                return "\n".join(
                    f'Hunk: Kind=HUNK_GLOBAL_CODE Align=4 Class=PR Name=".{name}"(1) Size=4\n'
                    "00000000: 4E800020 blr" for name in ("first", "second"))

            with patch.object(build, "ROOT", root), \
                 patch.object(build, "_wine_version", return_value="control"), \
                 patch.object(build.toolchain, "specification", return_value={
                     "flags": ["-O1", "-nolink"], "files": {}, "collapse_reloads": True}), \
                 patch.object(build, "_run", side_effect=run):
                a = build.compile_pair(first, root / "tools")
                b = build.compile_pair(second, root / "tools")
                self.assertEqual(len(compiles), 1)
                self.assertEqual(a.build_hash, b.build_hash)
                self.assertNotEqual(a.source_hash, b.source_hash)
                staged = build.object_directory(root, first) / "_inputs/include/value.h"
                staged.write_text("tampered")
                build.compile_pair(first, root / "tools")
                self.assertEqual(len(compiles), 2)
                self.assertEqual(staged.read_text(), "#define VALUE 1\n")
                path = root / "src/test.cpp"
                path.write_text(path.read_text().replace("return first();", "return first() + 1;"))
                c = build.compile_pair(first, root / "tools")
                self.assertEqual(a.source_hash, c.source_hash)
                self.assertNotEqual(a.build_hash, c.build_hash)

    def test_mac_headers_are_captured_verbatim(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.fixture(root)
            profile = profiles.load(root, "test")
            header = root / "include/test.h"
            # The actual compiler diagnoses missing or macro-selected includes.
            header.write_text('#include HEADER\n')
            self.assertEqual(profiles.headers(root, profile)["include/test.h"],
                             b'#include HEADER\n')
            header.unlink()
            header.symlink_to("/etc/passwd")
            with self.assertRaises(ValueError):
                profiles.headers(root, profile)

    def test_duplicate_header_profile_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.fixture(root)
            path = root / "config/mac/units.toml"
            path.write_text(path.read_text() + 'preamble="alternate.h"\n')
            with self.assertRaisesRegex(ValueError, "retired header settings"):
                profiles.load(root, "test")


if __name__ == "__main__":
    unittest.main()
