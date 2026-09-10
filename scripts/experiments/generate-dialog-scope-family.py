#!/usr/bin/env python3
"""Dialog loop-counter scopes and actual minimum/width operand lifetimes."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()

def function(text):
    start = text.index('void calculateNormalDialogSize(TNormalDialogInfo& dialogInfo)\n{')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
# The selected parent may be an explicitly reproduced byte-equivalent state,
# so inspect repeat results as well as the runner's object-diverse elites.
parents = []
checkpoint = json.loads(args.parent_checkpoint.read_text())
for row in checkpoint['records']:
    parent = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat'
    result = parent / 'result.json'
    path = parent / 'tree/src/kb.cpp'
    if result.is_file() and path.is_file() and function(path.read_text()) == body:
        repeated = json.loads(result.read_text())
        if repeated['object_hash'] != row['object_hash'] or repeated['scores'] != row['scores']:
            raise ValueError('parent failed reproduction')
        parents.append(row['id'])
if not parents:
    raise ValueError('dialog body does not equal a reproduced parent')

loops = [
    ('    for (i = 0; i < g_dialogIconMaxRows; ++i) {', '\n\n    int numIcons', 'initRow'),
    ('    for (i = 0; i < 8; i++)\n', '\n    iconRows =', 'countIcon'),
    ('    for (i = 0; i < 8; i++) {', '\n\n    dialogInfo.m_width = 40;', 'measureIcon'),
    ('        for (i = 0; i < iconRows; i++)\n', '\n        boxHeight += 20 *', 'heightRow'),
    ('    for (i = 0; i < iconRows; i++) {', '\n}', 'placeRow'),
]
# Exact bounded spans from this one reviewed function; not a function roster
# or a comparison of SH4 and candidate source structures.
import re
counter_options = [body]
for kind in ('owning-name', 'block-scope'):
    candidate = body.replace('    int i;\n', '')
    for begin, after, name in loops:
        start = candidate.index(begin)
        end = candidate.index(after, start)
        anchor = candidate[start:end]
        replacement = anchor.replace('for (i =', 'for (int i =', 1)
        if kind == 'owning-name':
            replacement = re.sub(r'\bi\b', name, replacement)
        else:
            indent = begin[:len(begin) - len(begin.lstrip())]
            replacement = indent + '{\n' + '\n'.join('    ' + line for line in replacement.splitlines()) + '\n' + indent + '}'
        candidate = candidate[:start] + replacement + candidate[end:]
    counter_options.append(candidate)

options = []
for counters, minimum, width in itertools.product(range(3), repeat=3):
    candidate = counter_options[counters]
    start = candidate.index('    dialogInfo.m_width = max(dialogInfo.m_width, dialogInfo.m_textWidgetWidth + 50);')
    before, tail = candidate[:start], candidate[start:]
    if minimum:
        declaration = ('const int' if minimum == 1 else 'int') + ' minimumDimension = 128;'
        tail = '    ' + declaration + '\n' + re.sub(r'\b128\b', 'minimumDimension', tail)
    if width:
        declaration = 'int& width = dialogInfo.m_width;' if width == 1 else 'int* width = &dialogInfo.m_width;'
        receiver = 'width' if width == 1 else '(*width)'
        tail = '    ' + declaration + '\n' + tail.replace('dialogInfo.m_width', receiver)
    candidate = before + tail
    row = {'name': f'counters-{counters}-minimum-{minimum}-width-{width}'}
    if counters or minimum or width:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'loop-and-dimension-lifetimes', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced source parents: ' + ', '.join(parents),
        'DC records separate loop scopes at 5219..5221, 5226..5228, 5236..5252, 5333..5336 and the placement tail. Test real counter lifetimes while preserving their operations and seven named local facts.',
        'Retail carries 128 in EDX from the width floor through the height floor; test a single named minimum-dimension value, without new operations.',
        'Retail reloads width after height layout where the candidate carries EBX. Test references/pointers to the actual width field; all stores remain source operations.',
        '27 finite states. Preserve the proven else-if, initialization order, returned line-count lifetime and canonical by-value max boundary.',
    ],
}, indent=2) + '\n')
