"""Exception records wait on their function; they never wait forever.

A FuncInfo whose owner is not byte-exact is `game-data-pending-function`
(tracked by the function's score, off the finish line); the same record is
`game-data-exact` once its owner matches and the bytes agree.
"""
import struct
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import patch

from homm3.build.test_eh_handler_normalization import (
    FixtureSection, _coff, _section_aux, _symbol)
from homm3.verify import eh_records, game_bytes as gb

DIR32 = 6
BASE = 0x1000
OWNER = '_f'
FUNCINFO = struct.pack('<IiIIIIII', 0x19930520, 0, 0, 0, 0, 0, 0, 0)


def _object():
    """`_f` with its `.text$x` registration stub naming `$T1` in `.xdata$x`."""
    text = b'\xc3'
    stub = b'\xb8\0\0\0\0\xe9\0\0\0\0'
    sections = (FixtureSection('.text', text, ()),
                FixtureSection('.text$x', stub, ((1, 7, DIR32),)),
                FixtureSection('.xdata$x', FUNCINFO, ()))
    symbols = (_symbol('.text', 0, 1, 0, 3, _section_aux(len(text), 0)),
               _symbol('.text$x', 0, 2, 0, 3, _section_aux(len(stub), 1, parent=1,
                                                          selection=5)),
               _symbol('.xdata$x', 0, 3, 0, 3, _section_aux(len(FUNCINFO), 0)),
               _symbol(OWNER, 0, 1, 0x20, 2),
               _symbol('$T1', 0, 3, 0, 3))
    return _coff(sections, symbols)


def _compare(retail, owner_exact):
    rows = [dict(object='unit.c', name=f'__ehfuncinfo${OWNER}', rva=hex(BASE),
                 size=hex(len(FUNCINFO)))]
    pe = SimpleNamespace(image_base=0x400000,
                         read=lambda start, size: retail[start - BASE:start - BASE + size])
    image = SimpleNamespace(relocs_in=lambda lo, hi: [])
    model = SimpleNamespace(functions=[SimpleNamespace(name=OWNER, channel='src',
                                                       unit='unit', rva=0x10)])
    with TemporaryDirectory() as tmp, \
            patch('homm3.delink.image.Image', return_value=image), \
            patch.object(eh_records, '_funclets', return_value={}), \
            patch.object(eh_records, '_exact_functions',
                         return_value=lambda unit, owner: owner_exact):
        Path(tmp, 'unit.obj').write_bytes(_object())
        return eh_records.compare(model, rows, pe, {}, Path(tmp))


def _category(results):
    from homm3.verify.byte_accounting import Range, partition
    rows = partition(BASE + len(FUNCINFO),
                     [Range(BASE, BASE + len(FUNCINFO), 'game', f'__ehfuncinfo${OWNER}', 2)])
    runs = gb.game_runs(rows, text=(0, 0), functions={}, code={},
                        data=gb.data_status(results))
    rows = gb.apply_runs(rows, runs, default=(gb.GAME_DATA_UNVERIFIED, 'no-comparison'))
    return {(r['category'], r['reason']) for r in rows if r['start'] == BASE}


class PendingFunctionRecordTests(unittest.TestCase):
    def test_a_differing_record_of_a_non_exact_owner_is_pending(self):
        retail = FUNCINFO[:4] + struct.pack('<i', 3) + FUNCINFO[8:]
        result, = _compare(retail, owner_exact=False)
        self.assertEqual(result['verdict'], 'unavailable')
        self.assertEqual(_category([result]),
                         {(gb.GAME_DATA_PENDING_FUNCTION, f'pending-function:{OWNER}')})
        self.assertNotIn(gb.GAME_DATA_PENDING_FUNCTION, gb.FINISH_LINE)
        rows = [dict(start=BASE, end=BASE + 32, category=gb.GAME_DATA_PENDING_FUNCTION,
                     reason=f'pending-function:{OWNER}')]
        self.assertEqual(gb.pending_functions(rows), {OWNER: 32})

    def test_an_exact_owner_with_equal_records_is_exact(self):
        result, = _compare(FUNCINFO, owner_exact=True)
        self.assertEqual(result['verdict'], 'exact')
        self.assertEqual(_category([result]), {(gb.GAME_DATA_EXACT, '')})

    def test_an_exact_owner_with_different_records_is_a_mismatch(self):
        retail = FUNCINFO[:4] + struct.pack('<i', 3) + FUNCINFO[8:]
        result, = _compare(retail, owner_exact=True)
        self.assertEqual(result['verdict'], 'mismatch')


if __name__ == '__main__':
    unittest.main()
