"""Pair admission cannot duplicate source identities or overlap target spans."""
from pathlib import Path
import tempfile
import tomllib
import unittest

from homm3.mac import build, pairing
from homm3.mac.relocations import Address
from homm3.mac.source import SourceError, candidate_source, load_pairs
from homm3.mac.test_loader import container
from homm3.mac import test_profiles as fixtures


class TestMacPairing(unittest.TestCase):
    def test_proposal_is_reviewable_and_admission_revalidates(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            fixtures.TestMacProfiles().fixture(root)
            source = root / "src/test.cpp"
            source.write_text(source.read_text() + "VA(0x00400300, 4)\nint third() { return 3; }\n")
            pef = container(bytes(64), ())
            probe = pairing.candidate(root, 0x400300, "test")
            self.assertIn("int third()", candidate_source(probe))
            self.assertEqual(len(load_pairs(root)), 2)
            self.assertEqual(build.object_directory(root, probe).name, "probe-00400300")
            row = pairing.proposal(root, pef, 0x400300, "test", Address(0, 0x40),
                                   4, ".third", "control identity; bounded entry")
            draft = pairing.write(root, row)
            self.assertEqual(tomllib.loads(draft.read_text())["functions"], [row])
            self.assertEqual(len(load_pairs(root)), 2)
            admitted = pairing.write(root, row, admit=True)
            self.assertEqual(admitted, root / "config/mac/functions/test.toml")
            self.assertEqual(len(load_pairs(root)), 3)
            with self.assertRaises(SourceError):
                pairing.write(root, row, admit=True)
            self.assertEqual(len(load_pairs(root)), 3)

    def test_wrong_owner_overlap_symbol_and_unaligned_span_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            fixtures.TestMacProfiles().fixture(root)
            source = root / "src/test.cpp"
            source.write_text(source.read_text() + "VA(0x00400300, 4)\nint third() { return 3; }\n")
            pef = container(bytes(64), ())
            args = dict(root=root, pef=pef, va=0x400300, unit="test",
                        at=Address(0, 0x40), size=4, symbol=".third", evidence="reviewed")
            for changes in (dict(va=0x400900), dict(at=Address(0, 0)),
                            dict(symbol=".first"), dict(at=Address(0, 3)), dict(evidence="")):
                with self.subTest(changes=changes), self.assertRaises(ValueError):
                    pairing.proposal(**dict(args, **changes))


if __name__ == "__main__":
    unittest.main()
