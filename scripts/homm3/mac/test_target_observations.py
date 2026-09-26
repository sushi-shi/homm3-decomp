from pathlib import Path
from types import SimpleNamespace
import hashlib
import tempfile
import unittest

from homm3.core import tsv
from homm3.mac import target_observations as observations


class ObservationTests(unittest.TestCase):
    def test_byte_span_and_duplicate_guards(self):
        body = bytes.fromhex('386000004e800020')
        pef = SimpleNamespace(read=lambda section, offset, size: body)
        row = ['0x100', '0x8', hashlib.sha256(body).hexdigest(), 'platform',
               'returnZero', 'Two instructions return zero without side effects.']
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / observations.PATH
            tsv.write(path, [], observations.FIELDS, [row])
            self.assertEqual(observations.read(root, pef, {0x100: 8})[0x100]['operation'], 'returnZero')
            with self.assertRaisesRegex(ValueError, 'unverified span'):
                observations.read(root, pef, {0x100: 12})
            changed = SimpleNamespace(read=lambda section, offset, size: b'\0' * 8)
            with self.assertRaisesRegex(ValueError, 'byte hash mismatch'):
                observations.read(root, changed, {0x100: 8})
            tsv.write(path, [], observations.FIELDS, [row, row])
            with self.assertRaisesRegex(ValueError, 'duplicate target'):
                observations.read(root, pef, {0x100: 8})

    def test_missing_observations_are_optional(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(observations.read(Path(directory), None, {}), {})


if __name__ == '__main__':
    unittest.main()
