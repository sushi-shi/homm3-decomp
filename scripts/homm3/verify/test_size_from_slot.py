"""A byte array's literal bound fills its retail slot: prove it or flag it.

Dreamcast CodeView proves the count only when the same compiland types the
declared (or cited) name as an array of exactly that many bytes.
"""
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace

from homm3.verify import game_bytes as gb

BASE = 0x1000


def _slots(declaration, comment, dreamcast, size=16):
    with TemporaryDirectory() as tmp:
        src = Path(tmp, 'src')
        src.mkdir()
        lines = [*(f'// {c}' for c in comment), f'DATA(0x{BASE + 0x400000:08x})', declaration]
        (src / 'misc.cpp').write_text('\n'.join(lines) + '\n')
        where = f'src/misc.cpp:{len(lines) - 1}'
        claims = {BASE: ('?x@@3PADA', size, f'char[{size}]')}
        annotation = {BASE: (where, 'DATA', declaration)}
        pe = SimpleNamespace(read=lambda start, n: b'ab' + bytes(n - 2))
        return gb.size_from_slot(claims, annotation, pe, lambda end: end,
                                 dreamcast, tmp)


class SizeFromSlotTests(unittest.TestCase):
    def test_an_unproven_literal_bound_is_flagged(self):
        self.assertEqual(len(_slots('static char buffer[16];', [], {})), 1)

    def test_a_dreamcast_array_of_the_same_name_and_size_proves_it(self):
        self.assertEqual(_slots('static char buffer[16];', [],
                                {('misc', 'buffer'): {16}}), [])

    def test_a_cited_dreamcast_name_proves_it(self):
        self.assertEqual(_slots('char g_x[16];', ['Original DC name: gcX.'],
                                {('misc', 'gcX'): {16}}), [])

    def test_another_size_or_compiland_does_not(self):
        self.assertEqual(len(_slots('static char buffer[16];', [],
                                    {('misc', 'buffer'): {20}})), 1)
        self.assertEqual(len(_slots('static char buffer[16];', [],
                                    {('kb', 'buffer'): {16}})), 1)


if __name__ == '__main__':
    unittest.main()
