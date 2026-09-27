"""Source-owned Mac DATA claims retained in registered C++ fragments."""
from hashlib import sha256
from pathlib import Path
import tempfile
import unittest

from homm3.mac.source import SourceError, load_data


class TestFragmentData(unittest.TestCase):
    def test_registered_file_scope_const_table_and_duplicate_rejection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            (root / "config/source").mkdir(parents=True)
            (root / "src").mkdir()
            (root / "include/inline").mkdir(parents=True)
            owner = root / "src/owner.cpp"
            owner.write_text('#include "../include/inline/table.inl"\n')
            fragment = root / "include/inline/table.inl"
            fragment.write_text(
                "DATA(0x00400100) static const int table[3] = {1, 2, 3};\n")
            registry = root / "config/source/header-fragments.toml"
            registry.write_text('''[[fragments]]
fragment = "include/inline/table.inl"
owner = "src/owner.cpp"
evidence = "The literal include retains the canonical table definition."
''')
            (root / "config/mac/data.toml").write_text(f'''[[data]]
retail_va = 0x00400100
source = "src/owner.cpp"
mac_section = 1
mac_offset = 0x40
mac_size = 12
sha256 = "{sha256(bytes(12)).hexdigest()}"
evidence = "Pinned source-owned table and TOC destination."
''')

            pair, = load_data(root)
            self.assertEqual(pair.name, "table")
            self.assertTrue(pair.read_only)
            self.assertIn("static const int table[3]", pair.definition)

            owner.write_text(owner.read_text() +
                             "DATA(0x00400100) static const int duplicate[3] = {1, 2, 3};\n")
            with self.assertRaisesRegex(SourceError, "expected one DATA"):
                load_data(root)

            owner.write_text('void f() {\n#include "../include/inline/table.inl"\n}\n')
            with self.assertRaises(SourceError):
                load_data(root)

            owner.write_text('#include "../include/inline/table.inl"\n')
            registry.unlink()
            with self.assertRaisesRegex(SourceError, "expected one DATA"):
                load_data(root)

    def test_function_owned_mutable_initializer_and_owner_guards(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config/mac").mkdir(parents=True)
            (root / "config/units.toml").write_text("")
            (root / "src").mkdir()
            owner = root / "src/owner.cpp"
            body = "VA(0x00400200, 32)\nvoid f() {\n%s\n}\n"
            declaration = "DATA(0x00400100) static int towers[4] = {3,7,0,6};"
            owner.write_text(body % declaration)
            manifest = root / "config/mac/data.toml"
            manifest.write_text(f'''[[data]]
retail_va = 0x00400100
owner_va = 0x00400200
source = "src/owner.cpp"
mac_section = 1
mac_offset = 0x40
mac_size = 16
sha256 = "{sha256(bytes(16)).hexdigest()}"
evidence = "Reviewed mutable local static array, complete payload and owner."
''')
            pair, = load_data(root)
            self.assertEqual(pair.local_owner_va, 0x00400200)
            self.assertEqual(pair.mac_symbol, "towers")
            self.assertFalse(pair.read_only)
            self.assertEqual(pair.definition, "")
            owner.write_text(body % declaration.replace("static ", ""))
            with self.assertRaisesRegex(SourceError, "canonical static initializer"):
                load_data(root)
            owner.write_text(body % declaration.replace(" = {3,7,0,6}", ""))
            with self.assertRaisesRegex(SourceError, "unsupported Mac data definition"):
                load_data(root)
            owner.write_text(declaration + "\n" + body % "")
            with self.assertRaisesRegex(SourceError, "outside its claimed function owner"):
                load_data(root)
            owner.write_text(body % declaration)
            manifest.write_text(manifest.read_text() + 'mac_symbol = "towers$7"\n')
            with self.assertRaisesRegex(SourceError, "counters must not be pinned"):
                load_data(root)


if __name__ == "__main__":
    unittest.main()
