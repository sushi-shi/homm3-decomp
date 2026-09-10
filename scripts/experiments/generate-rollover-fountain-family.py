#!/usr/bin/env python3
"""Fountain source operations from DC3477 and DC3487..3491."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/advmgr.cpp').read_text()

def function(text):
    start = text.index('void advManager::setRolloverText(NewmapCell* testCell, int rx, int ry)\n{')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
parents = []
for row in json.loads(args.parent_checkpoint.read_text())['elites']:
    p = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/advmgr.cpp'
    if p.is_file() and function(p.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('rollover body does not equal a reproduced parent')
start = body.index('    case FOUNTAIN_OF_FORTUNE:')
end = body.index('    case FOUNTAIN_OF_YOUTH:', start)
arm = body[start:end]
mask = '                visited = (currHero->m_flags & (0x38000000UL | 0x20UL));'
assert arm.count(mask) == 1
options = []
for terms, query in itertools.product(range(3), range(2)):
    candidate = arm
    if terms:
        values = ('0x20', '0x08000000', '0x10000000', '0x20000000')
        if terms == 2:
            values = tuple(reversed(values))
        expression = '\n                    + '.join('(currHero->m_flags & ' + v + ')' for v in values)
        candidate = candidate.replace(mask, '                visited = ' + expression + ';')
    if query:
        candidate = candidate.replace('        if (cell->m_isTrigger) {\n', '''        if (cell->m_isTrigger) {
            // DC3477 assigns infolevel from GetInfoFlag before DC3479's
            // independent PlayerKnowsCell test. VC6 may elide the unused
            // result, but the recorded query is real source work.
            infolevel = g_game->getInfoFlag(FountainOfFortuneInfo, thisPlayer);
''')
    row = {'name': f'terms-{terms}-recorded-info-query-{query}'}
    if terms or query:
        row['replace'] = body[:start] + candidate + body[end:]
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/advmgr.cpp', 'units': ['advmgr'],
    'axes': [{'name': 'fountain-recorded-query-and-sum', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'DC3477 (ce34..ce4c) calls GetInfoFlag(FountainOfFortuneInfo,thisPlayer) and stores infolevel, then DC3479 independently tests PlayerKnowsCell. The candidate omitted this real, release-elidable assignment.',
        'DC ce94..cec4 forms four masked terms with additions, in order0x20,0x08000000,0x10000000,0x20000000. The recorded row3491 follows the3486..3490 gap, consistent with a multiline sum, not proof of each line contents.',
        'Retail+0x8b1..+0x8d8 also emits all four ANDs and three ADDs, evaluating high bits first. Test both source operand orders against the flattened one-mask negative control; retain a supported sum even through score dips.',
        'The passive trace currently rejects QuestGuard _Tidy at budget143 vs cost152. Every added expression in this family has direct source/retail evidence; no dummy operations or inline pins.',
    ],
}, indent=2) + '\n')
