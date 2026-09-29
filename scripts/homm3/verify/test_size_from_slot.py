"""A byte array's literal bound fills its retail slot: prove it or flag it.

Dreamcast CodeView proves the count only when the same compiland types the
declared (or cited) name as an array of exactly that many bytes. The next
symbol proves it when VC6's placement rule leaves no room for a shorter
array before a verified following claim; a retail access to the last bytes
closes the alignment slack.
"""
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace

from homm3.verify import game_bytes as gb

BASE = 0x1000


def _run(declaration, comment, dreamcast, size=16, following=None, verified=True,
         touched=None, compilands=()):
    """(flagged, proven) for one claim at BASE whose slot ends at the next claim."""
    with TemporaryDirectory() as tmp:
        src = Path(tmp, 'src')
        src.mkdir()
        lines = [*(f'// {c}' for c in comment), f'DATA(0x{BASE + 0x400000:08x})', declaration]
        (src / 'misc.cpp').write_text('\n'.join(lines) + '\n')
        where = f'src/misc.cpp:{len(lines) - 1}'
        claims = {BASE: ('?x@@3PADA', size, f'char[{size}]')}
        annotation = {BASE: (where, 'DATA', declaration)}
        pe = SimpleNamespace(read=lambda start, n: b'ab' + bytes(n - 2))
        nxt = BASE + size
        return gb.size_from_slot(
            claims, annotation, pe, lambda end: end, dreamcast, tmp,
            following={nxt: following}.get if following else None,
            verified_at=lambda address: verified and address == nxt,
            compilands=compilands, touched=touched)


def _slots(declaration, comment, dreamcast, size=16, **kw):
    return _run(declaration, comment, dreamcast, size, **kw)[0]


class SizeFromSlotTests(unittest.TestCase):
    def test_an_unproven_literal_bound_is_flagged(self):
        self.assertEqual(len(_slots('static char buffer[16];', [], {})), 1)

    def test_a_dreamcast_array_of_the_same_name_and_size_proves_it(self):
        flagged, proven = _run('static char buffer[16];', [], {('misc', 'buffer'): {16}})
        self.assertEqual(flagged, [])
        self.assertEqual(proven[0]['proof'], 'dreamcast-record')

    def test_a_cited_dreamcast_name_proves_it(self):
        self.assertEqual(_slots('char g_x[16];', ['Original DC name: gcX.'],
                                {('misc', 'gcX'): {16}}), [])

    def test_another_size_or_compiland_does_not(self):
        self.assertEqual(len(_slots('static char buffer[16];', [],
                                    {('misc', 'buffer'): {20}})), 1)
        self.assertEqual(len(_slots('static char buffer[16];', [],
                                    {('kb', 'buffer'): {16}})), 1)


class NextSymbolTests(unittest.TestCase):
    """The claim at 0x1000 fills its slot to the next claim."""

    def test_a_char_aligned_next_symbol_is_exact(self):
        # 0x1000 + 15 = 0x100f: an unsigned char there is placed on 1 byte,
        # and an odd address cannot open another contribution.
        flagged, proven = _run('char g_x[15];', [], {}, size=15,
                               following=(1, 'unsigned char'))
        self.assertEqual(flagged, [])
        self.assertEqual(proven[0]['proof'], 'next-symbol')

    def test_an_int_aligned_next_symbol_leaves_a_range(self):
        # 0x1000 + 0x104 = 0x1104 (4 mod 8): only a 4-aligned contribution
        # could start there, so either way three fewer bytes still fit.
        flagged, proven = _run('char g_x[260];', [], {}, size=260, following=(4, 'int'))
        self.assertEqual(proven, [])
        self.assertEqual(flagged[0][6]['range'], [257, 260])

    def test_an_eight_aligned_start_without_compiland_evidence_widens_it(self):
        flagged, _ = _run('char g_x[256];', [], {}, size=256, following=(4, 'int'))
        self.assertEqual(flagged[0][6]['range'], [249, 256])
        flagged, _ = _run('char g_x[256];', [], {}, size=256, following=(4, 'int'),
                          compilands=[(BASE - 4, BASE + 0x200, 'include/terrain.h')])
        self.assertEqual(flagged[0][6]['range'], [253, 256])

    def test_a_usage_that_touches_the_last_byte_pins_it(self):
        flagged, proven = _run('char g_x[260];', [], {}, size=260, following=(4, 'int'),
                               touched=lambda start, end: start + 260)
        self.assertEqual(flagged, [])
        self.assertEqual(proven[0]['proof'], 'next-symbol+usage')

    def test_an_unproven_next_symbol_is_flagged(self):
        flagged, proven = _run('char g_x[15];', [], {}, size=15,
                               following=(1, 'unsigned char'), verified=False)
        self.assertEqual(proven, [])
        self.assertEqual(flagged[0][6]['reason'], 'the next symbol is not verified')

    def test_a_dreamcast_size_outside_the_bound_is_reported(self):
        flagged, _ = _run('static char buffer[16];', [], {('misc', 'buffer'): {20}},
                          following=(4, 'int'))
        self.assertEqual(flagged[0][6]['dreamcast'], [20])


class PlacementTests(unittest.TestCase):
    def test_measured_rules(self):
        cases = {('char[297]', 297): 4, ('char[2]', 2): 4, ('unsigned char', 1): 1,
                 ('short', 2): 2, ('void*', 4): 4, ('std::bitset<10>', 4): 4,
                 ('configStruct', 212): 8, ('double', 8): 8, ('C3', 3): None,
                 ('Big[2]', 16): 8, ('', 4): None}
        for (text, size), alignment in cases.items():
            self.assertEqual(gb.placement_alignment(text, size), alignment, text)


if __name__ == '__main__':
    unittest.main()
