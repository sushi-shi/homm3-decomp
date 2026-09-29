"""Reviewed padding: only an explicit row accounts for a gap."""
import unittest
from types import SimpleNamespace

from homm3.verify import padding as pd
from homm3.verify import game_bytes as gb
from homm3.verify.byte_accounting import Range, partition

BASE = 0x100
# The user's case: retail FF x 12 (creatures[6], heroName[6]), then zeros up
# to the next object at 0x10.
RETAIL = b'\xff' * 12 + bytes(4) + b'\x01' * 4


def _pe():
    sections = [dict(name='.data', va=BASE, vsize=len(RETAIL), rsize=len(RETAIL), rptr=0)]
    return SimpleNamespace(
        sections=sections, image_base=0x400000,
        section=lambda name: dict(va=0, vsize=0) if name == '.text' else sections[0],
        read=lambda start, size: RETAIL[start - BASE:start - BASE + size])


CLAIMS = [Range(BASE, BASE + 6, 'game', '_creat', 2), Range(BASE + 6, BASE + 12, 'game', '_hero', 2),
          Range(BASE + 16, BASE + 20, 'game', '_next', 2)]


def _row(rva, size, fill='00', proof='after-byte-array'):
    return dict(rva=hex(rva), size=str(size), category='padding', section='.data', fill=fill,
                preceding='_hero | char[6]', following='_next', alignment='16', proof=proof,
                evidence='test')


def _category_at(rows, address):
    return next(r['category'] for r in rows if r['start'] <= address < r['end'])


class ReviewedPaddingTests(unittest.TestCase):
    def test_without_a_row_the_gap_after_a_short_array_is_missing(self):
        rows = partition(BASE + len(RETAIL), CLAIMS)
        for address in range(BASE + 12, BASE + 16):
            self.assertEqual(_category_at(rows, address), 'missing')

    def test_an_explicit_row_is_padding_not_data(self):
        padding = pd.claims(_pe(), [_row(BASE + 12, 4)])
        pd.check_overlaps(padding, CLAIMS)
        rows = partition(BASE + len(RETAIL), CLAIMS + padding)
        for address in range(BASE + 12, BASE + 16):
            self.assertEqual(_category_at(rows, address), 'padding')
        self.assertEqual(_category_at(rows, BASE + 11), 'game')

    def test_a_row_overlapping_an_emitted_definition_is_an_error(self):
        # Even a zero-filled row may not reach into heroName's emitted bytes.
        padding = [Range(BASE + 10, BASE + 16, 'padding', pd.identity(BASE + 10), 2)]
        with self.assertRaises(ValueError):
            pd.check_overlaps(padding, CLAIMS)

    def test_a_row_whose_bytes_are_not_its_fill_is_an_error(self):
        with self.assertRaises(ValueError):
            pd.claims(_pe(), [_row(BASE + 8, 4)])

    def test_proposal_names_the_short_array_risk(self):
        rows = gb.apply_runs(partition(BASE + len(RETAIL), CLAIMS), [])
        types = {'_creat': 'Creature[6]', '_hero': 'char[6]', '_next': 'int'}

        def describe(row):
            name = row['owners'][0] if row['owners'] else ''
            return name, row['category'], types.get(name, ''), 'src/x.cpp:1', row['size']
        proposal, = pd.propose([Range(BASE + 12, BASE + 16, 'alignment-padding', 'link', 0)],
                               rows, _pe(), describe)
        self.assertEqual((proposal['rva'], proposal['size'], proposal['fill'], proposal['proof']),
                         (hex(BASE + 12), '4', '00', 'after-byte-array'))
        self.assertIn('_hero', proposal['preceding'])
        self.assertIn('_next', proposal['following'])

    def test_proof_classes(self):
        self.assertEqual(pd.proof('_count', gb.GAME_DATA_EXACT, 'int'), 'proven-end')
        self.assertEqual(pd.proof('_hero', gb.GAME_DATA_EXACT, 'char[6]'), 'after-byte-array')
        self.assertEqual(pd.proof('??_C@_03ABCD@abc?$AA@', gb.GAME_DATA_EXACT, ''),
                         'after-literal')
        self.assertEqual(pd.proof('', 'missing', ''), 'after-unverified')
        self.assertEqual(pd.proof('LIBCMT', 'library-runtime', ''), 'proven-end')


if __name__ == '__main__':
    unittest.main()
