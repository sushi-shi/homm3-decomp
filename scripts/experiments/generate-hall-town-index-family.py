#!/usr/bin/env python3
"""Hall's recorded town-parameter indexing versus specialized case rows."""
import argparse
import itertools
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/townmgr.cpp').read_text()

def function(text):
    start = text.index('THallWindow::THallWindow(int which)')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
parents = []
for row in json.loads(args.parent_checkpoint.read_text())['elites']:
    p = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/townmgr.cpp'
    if p.is_file() and function(p.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('Hall body does not equal a reproduced parent')
options = []
for x, y in itertools.product(range(2), repeat=2):
    candidate = body
    for axis, enabled in (('X', x), ('Y', y)):
        if enabled:
            candidate, count = re.subn('hall' + axis + r'\[[0-8]\]\[i\]', 'hall' + axis + '[which][i]', candidate)
            if count != 36:
                raise ValueError('expected four coordinate reads per town')
    row = {'name': f'parameter-x-{x}-parameter-y-{y}'}
    if x or y:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'],
    'axes': [{'name': 'town-parameter-row-index', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'DC16e6d8 retains the which parameter in R9; switch4338 dispatches that R9. At4344 (16eafe..16eb28) it multiplies R9 by72 and uses that offset for both hallX and hallY; the current source has already specialized each town row to its case number.',
        'Retain that positive parameter-index source fact and check whether VC6 propagates the case value. Four states isolate each table and their combination; array values, constness and both inner indices remain unchanged.',
        'The preceding16 increment/ID-order/counter-type states emitted one object. Do not infer a different original loop index merely from the strength-reduced retail pointer.',
    ],
}, indent=2) + '\n')
