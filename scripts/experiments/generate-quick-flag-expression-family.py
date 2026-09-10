#!/usr/bin/env python3
"""QuickInfo flag-expression groups after restoring cell-type indexes."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_result', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]

def function(text):
    start = text.index('void advManager::quickInfo(int cellX, int cellY, int z)\n{')
    return text[start:text.index('\n}\n', start) + 2]

body = function((root / 'src/advmgr.cpp').read_text())
parent = json.loads(args.parent_result.read_text())
if args.parent_result.parent.name != 'repeat' or function((args.parent_result.parent / 'tree/src/advmgr.cpp').read_text()) != body:
    raise ValueError('authored function must equal reproduced parent')
edits = [(
'''                        testFlag = 1UL << (cell->m_extraInfo & 0x1f);
                        visited = testFlag & currHero->m_arenaFlags;''',
'''                        visited = currHero->m_arenaFlags
                            & (1UL << (cell->m_extraInfo & 0x1f));'''), (
'''                    testFlag = 1UL << (cell->m_extraInfo & 0x1f);
                    visited = (g_currentPlayer->m_mysticalGardenFlags & testFlag)
                        && !((cell->m_extraInfo >> 10) & 1);''',
'''                    visited = (g_currentPlayer->m_mysticalGardenFlags
                        & (1UL << (cell->m_extraInfo & 0x1f)))
                        && !((cell->m_extraInfo >> 10) & 1);'''), (
'''                        visited = ((currHero->m_flags
                            & 0x04000000UL) + (currHero->m_flags & 0x100UL));''',
'''                        visited = ((currHero->m_flags
                            & 0x100UL) + (currHero->m_flags & 0x04000000UL));''')]
assert all(body.count(old) == 1 for old, new in edits)
options = []
for choices in itertools.product(range(2), repeat=3):
    candidate = body
    for choice, (old, new) in zip(choices, edits):
        if choice: candidate = candidate.replace(old, new)
    row = {'name': '-'.join(map(str, choices))}
    if any(choices): row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/advmgr.cpp', 'units': ['advmgr'],
    'axes': [{'name': 'visit-mask-expression-stages', 'find': body, 'options': options}],
    'evidence': [
        'Reproduced parent: ' + str(args.parent_result) + ' id ' + parent['id'],
        'DC7618/16496..164ac groups hero arena flags, masked cell shift, AND and the visited store; no extra source row identifies the testFlag assignment. Test a direct expression without claiming the temporary was absent. The positive unsigned-long testFlag declaration remains.',
        'DC8332/17d78..17d96 groups the garden visit mask and visited store. Preserve Complete\'s additional availability test and current 5-bit index representation; the DC GetItemId boundary remains an explicit header-ownership review gap.',
        'DC8588/185ca..185e2 sums low0x100 then high0x4000000 into visited. Test that operand order while retaining both masked terms and all formatting.',
        'Eight finite source states preserve the adopted query, name indexes, canonical helpers, scopes, signed visited carrier and all93 exact advmgr siblings. No dummy statements or inline controls.',
    ],
}, indent=2) + '\n')
