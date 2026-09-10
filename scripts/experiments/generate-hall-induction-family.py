#!/usr/bin/env python3
"""Hall loop induction after all temporary homes agree with retail."""
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
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/townmgr.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('Hall body does not equal a reproduced parent')
assert body.count(' i++) {') == 9
options = []
for increment, id_order, counter_type in itertools.product(range(4), range(2), range(2)):
    candidate = body
    if increment == 1:
        candidate = candidate.replace(' i++) {', ' ++i) {')
    elif increment == 2:
        candidate = candidate.replace(' i++) {', ' i += 1) {')
    elif increment == 3:
        candidate = candidate.replace(' i++) {', ') {')
        candidate = candidate.replace('            m_widgets.push_back(builtCheck);\n', '            m_widgets.push_back(builtCheck);\n            ++i;\n')
    if id_order:
        candidate, count = re.subn(r'\b([45678]00) \+ i\b', r'i + \1', candidate)
        if count != 36:
            raise ValueError('expected four ID additions per town')
    if counter_type:
        candidate = candidate.replace('    int i;\n', '    long i;\n')
    row = {'name': f'increment-{increment}-id-order-{id_order}-counter-type-{counter_type}'}
    if increment or id_order or counter_type:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'],
    'axes': [{'name': 'loop-induction-expression', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'The reproduced99.6605 parent has all temporary stack homes and369 CFG blocks equal to retail. The source diff now contains only six adjacent swaps: target advances EDI by4 before incrementing the EBX ID induction, while the candidate increments EBX first.',
        'DC4344..4347 and peer groups contain the four constructions, followed by their loop back edge. Keep construction/call order and test the increment as header post/pre/add or a statement after the last append.',
        'The induction is signed32 in retail. DC does not record the counter type; int and long are the bounded type hypotheses. Reversing the four commutative ID additions tests expression ownership of the ID induction without adding a second counter.',
        'All four const arrays, real result lifetimes and direct status constructions remain fixed. No explicit synthetic induction variables or assembly controls.',
    ],
}, indent=2) + '\n')
