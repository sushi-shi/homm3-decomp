"""Generated-code verdicts belong to one executable, even at equal RVAs."""
import importlib
import os
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

from homm3.core import paths, tsv
from homm3.verify import generated_code


class GeneratedImageLedgerTests(unittest.TestCase):
    def test_each_image_reads_and_writes_only_its_own_generated_ledger(self):
        for key in ('game', 'h3maped', 'h3ccmped'):
            with self.subTest(image=key), TemporaryDirectory() as tmp:
                root = Path(tmp)
                (root / 'config').mkdir()
                game = root / 'config/generated_code_baseline.tsv'
                header = generated_code.BASELINE_HEADER
                old = dict(rva='0x100',size='1',category='init-thunk',
                           owner='game',verdict='exact',reason='-')
                tsv.write(game, [], header, [[old[h] for h in header]])
                before = game.read_bytes()
                retail = root / ('config/retail' if key == 'game' else f'config/retail/{key}')
                try:
                    with patch.dict(os.environ, {'HOMM3_IMAGE': key}), \
                         patch.object(paths, 'ROOT', root), patch.object(paths, 'RETAIL', retail):
                        module = importlib.reload(generated_code)
                        expected = (game if key == 'game' else
                                    root / f'config/generated_code_baseline.{key}.tsv')
                        self.assertEqual(module.BASELINE, expected)
                        self.assertEqual(module.committed().get('init-thunk', set()),
                                         {0x100} if key == 'game' else set())
                        row = dict(old, rva='0x200', owner=key)
                        module.write_baseline([row])
                        self.assertEqual(module.committed()['init-thunk'], {0x200})
                        self.assertFalse(module.stale(None, [row]))
                        self.assertTrue(expected.is_file())
                        if key != 'game':
                            self.assertEqual(game.read_bytes(), before)
                finally:
                    importlib.reload(generated_code)


if __name__ == '__main__':
    unittest.main()
