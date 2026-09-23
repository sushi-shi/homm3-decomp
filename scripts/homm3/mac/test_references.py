"""Callee-only identities never become scored pairs or bypass source ownership."""
from hashlib import sha256
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import references, symbols
from homm3.mac.source import SourceError, load_pairs
from homm3.mac import test_profiles
from homm3.mac.test_loader import container


class TestMacReferences(unittest.TestCase):
    def fixture(self, root):
        test_profiles.TestMacProfiles().fixture(root)
        source = root / "src/test.cpp"
        source.write_text(source.read_text() + "VA(0x00400300, 4)\nint third() { return 3; }\n")
        folder = root / "config/mac/references"
        folder.mkdir()
        row = ('[[functions]]\nretail_va=0x00400300\nunit="test"\n'
               'mac_section=0\nmac_offset=0x40\nmac_size=4\nmac_symbol=".third"\n'
               f'target_sha256="{sha256(bytes(4)).hexdigest()}"\nevidence="control identity and boundary"\n')
        path = folder / "caller.toml"
        path.write_text(row)
        (root / "config/mac/runtime.toml").write_text("")
        return path, row

    def test_source_name_hash_and_score_denominator(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path, row = self.fixture(root)
            ref, = references.load(root)
            self.assertEqual(ref.signature, "int third")
            self.assertEqual(len(load_pairs(root)), 2)
            pef = container(bytes(64), ())
            with patch("homm3.mac.glue.imports", return_value={}):
                self.assertIn(".third", symbols.targets(root, pef))
                path.write_text(row.replace(sha256(bytes(4)).hexdigest(), "1" * 64))
                with self.assertRaises(SourceError):
                    symbols.targets(root, pef)

    def test_forward_declaration_can_own_reference_but_not_compiled_body(self):
        from homm3.mac.source import _claim
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.fixture(root)
            source = root / "src/test.cpp"
            source.write_text(source.read_text().replace("int third() { return 3; }", "int third();"))
            ref, = references.load(root)
            self.assertEqual(ref.signature, "int third")
            with self.assertRaises(SourceError):
                _claim(source.read_text(), 0x400300, source)

    def test_cached_source_scan_does_not_reuse_same_size_changed_claim(self):
        from homm3.mac.source import _claim
        source = Path("same.cpp")
        original = "VA(0x00400100, 4)\nint first() { return 1; }\n"
        changed = original.replace("0x00400100", "0x00400200")
        self.assertEqual(len(original), len(changed))
        self.assertEqual(_claim(original, 0x400100, source)[1], "int first")
        with self.assertRaises(SourceError):
            _claim(changed, 0x400100, source)
        self.assertEqual(_claim(changed, 0x400200, source)[1], "int first")

    def test_conflicting_owner_extent_and_symbols_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path, row = self.fixture(root)
            for old, new in (('unit="test"', 'unit="absent"'),
                             ("0x00400300", "0x00400900"),
                             ("mac_offset=0x40", "mac_offset=0"),
                             ("mac_offset=0x40", "mac_offset=3"),
                             ('mac_symbol=".third"', 'mac_symbol=".first"')):
                with self.subTest(new=new):
                    path.write_text(row.replace(old, new))
                    with self.assertRaises(SourceError):
                        references.load(root)
            path.write_text(row)
            duplicate = path.with_name("other.toml")
            duplicate.write_text(row)
            self.assertEqual(len(references.load(root)), 1)
            duplicate.write_text(row.replace("mac_offset=0x40", "mac_offset=0x44"))
            with self.assertRaises(SourceError):
                references.load(root)
