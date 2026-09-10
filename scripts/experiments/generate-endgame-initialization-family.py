#!/usr/bin/env python3
"""CheckEndGame outcome initialization order from DC3628/3629."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()

def function(text):
    start = text.index('void checkEndGame(int forceWin)\n{')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
parents = []
for row in json.loads(args.parent_checkpoint.read_text())['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('endgame body does not equal a reproduced parent')
anchor = '    int standardVictoryAllowed = 1;\n    gameWon = 0;\n    gameLost = 0;'
assert body.count(anchor) == 1
options = [{'name': 'standard-before-outcomes'}]
for name, text in [
    ('standard-between-outcomes', '    gameWon = 0;\n    int standardVictoryAllowed = 1;\n    gameLost = 0;'),
    ('standard-after-outcomes', '    gameWon = 0;\n    gameLost = 0;\n    int standardVictoryAllowed = 1;'),
    ('chained-outcomes-before-standard', '    gameWon = gameLost = 0;\n    int standardVictoryAllowed = 1;'),
]:
    options.append({'name': name, 'replace': body.replace(anchor, text)})
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'outcome-and-standard-initialization-order', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'At99.7119 all85 blocks and25 named calls agree; the sole instruction difference is mov edi,1 scheduled before rather than after two outcome zero stores.',
        'DC3628 begins materializing zero and storing the first outcome; DC3629 materializes standardVictoryAllowed=1 with the second zero store interleaved. Test both distinct assignments before the standard initialization, the interleaved order, and a chained outcome assignment corresponding to a single source row.',
        'Keep the named int outcomes, ordinary helpers, recovered handled flag and forced/global outcome bookkeeping.',
    ],
}, indent=2) + '\n')
