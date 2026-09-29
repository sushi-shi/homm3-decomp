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
        kept, stale = pd.supersede(padding, CLAIMS)
        self.assertEqual((kept, stale), (padding, []))
        rows = partition(BASE + len(RETAIL), CLAIMS + padding)
        for address in range(BASE + 12, BASE + 16):
            self.assertEqual(_category_at(rows, address), 'padding')
        self.assertEqual(_category_at(rows, BASE + 11), 'game')

    def test_a_row_overlapping_an_emitted_definition_is_an_error(self):
        # Even a zero-filled row may not reach into heroName's emitted bytes.
        # heroName is not yet exact, so the game claim does not supersede it.
        padding = [Range(BASE + 10, BASE + 16, 'padding', pd.identity(BASE + 10), 2)]
        _kept, stale = pd.supersede(padding, CLAIMS)
        image = [dict(start=BASE + 6, end=BASE + 12, category=gb.GAME_DATA_UNVERIFIED)]
        with self.assertRaises(ValueError):
            pd.confirm(stale, image)

    def test_a_row_fully_covered_by_exact_fill_is_stale(self):
        padding = pd.claims(_pe(), [_row(BASE + 12, 4)])
        fill = Range(BASE + 12, BASE + 16, 'source-padding-exact', 'u:?f@@YAXXZ@100', 2)
        kept, stale = pd.supersede(padding, CLAIMS + [fill])
        self.assertEqual(kept, [])
        self.assertEqual([(r['rva'], r['covered_bytes'], r['remaining']) for r in stale],
                         [(hex(BASE + 12), 4, [])])
        rows = partition(BASE + len(RETAIL), CLAIMS + [fill] + kept)
        for address in range(BASE + 12, BASE + 16):
            self.assertEqual(_category_at(rows, address), 'source-padding-exact')

    def test_a_partially_covered_row_keeps_the_rest_as_padding(self):
        padding = pd.claims(_pe(), [_row(BASE + 12, 4)])
        fill = Range(BASE + 12, BASE + 14, 'source-padding-exact', 'u:?f@@YAXXZ@100', 2)
        kept, stale = pd.supersede(padding, CLAIMS + [fill])
        self.assertEqual([(r.start, r.end) for r in kept], [(BASE + 14, BASE + 16)])
        self.assertEqual(stale[0]['remaining'], [[hex(BASE + 14), 2]])
        rows = partition(BASE + len(RETAIL), CLAIMS + [fill] + kept)
        self.assertEqual(_category_at(rows, BASE + 13), 'source-padding-exact')
        self.assertEqual(_category_at(rows, BASE + 14), 'padding')

    def test_a_row_overlapping_an_unverified_claim_is_an_error(self):
        padding = pd.claims(_pe(), [_row(BASE + 12, 4)])
        for category in ('library-unverified', 'source-padding-aligned', 'padding'):
            other = Range(BASE + 12, BASE + 14, category, 'other', 2)
            with self.assertRaises(ValueError, msg=category):
                if category == 'padding':
                    pd.supersede(padding + [other], CLAIMS)
                else:
                    pd.supersede(padding, CLAIMS + [other])

    def test_a_game_claim_supersedes_only_with_an_exact_verdict(self):
        padding = pd.claims(_pe(), [_row(BASE + 12, 4)])
        grown = Range(BASE + 12, BASE + 16, 'game', '_grown', 2)
        kept, stale = pd.supersede(padding, CLAIMS + [grown])
        self.assertEqual(kept, [])
        exact = [dict(start=BASE + 12, end=BASE + 16, category=gb.GAME_DATA_EXACT)]
        self.assertEqual(pd.confirm(stale, exact)[0]['covered'][0]['category'],
                         gb.GAME_DATA_EXACT)
        _, stale = pd.supersede(padding, CLAIMS + [grown])
        unverified = [dict(start=BASE + 12, end=BASE + 16, category=gb.GAME_DATA_UNVERIFIED)]
        with self.assertRaises(ValueError):
            pd.confirm(stale, unverified)

    def test_retire_removes_exactly_the_stale_rows(self):
        import tempfile
        from pathlib import Path
        from homm3.core.tsv import read, write
        fields = ['rva', 'size', 'category', 'evidence', 'section', 'fill', 'preceding',
                  'following', 'alignment', 'proof']
        rows = [dict(_row(BASE + 12, 4), evidence='a'), dict(_row(BASE + 32, 4), evidence='b'),
                dict(_row(BASE + 48, 8), evidence='c'),
                dict(rva=hex(BASE + 64), size='4', category='patch-residue', evidence='d',
                     section='.data', fill='-', preceding='-', following='-', alignment='-',
                     proof='-')]
        stale = [dict(rva=hex(BASE + 12), size=4, covered_bytes=4, remaining=[]),
                 dict(rva=hex(BASE + 48), size=8, covered_bytes=4,
                      remaining=[[hex(BASE + 52), 4]]),
                 # A row that has since changed size is left alone.
                 dict(rva=hex(BASE + 32), size=2, covered_bytes=2, remaining=[])]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'code-extents.tsv'
            write(path, ['# test'], fields, rows)
            self.assertEqual(pd.retire(stale, (path,)), (1, 1, 8))
            left = [(r['rva'], r['size'], r['evidence']) for r in read(path)[2]]
        self.assertEqual(left, [(hex(BASE + 32), '4', 'b'), (hex(BASE + 52), '4', 'c'),
                                (hex(BASE + 64), '4', 'd')])

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
