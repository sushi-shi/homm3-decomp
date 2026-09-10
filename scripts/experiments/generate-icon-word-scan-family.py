#!/usr/bin/env python3
"""Icon word-scan expression and counter lifetimes from DC5096..5111."""
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
    start = text.index('void type_dialog_icon::set(EGameResource resource, long qualifier)\n{')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
parents = []
for row in json.loads(args.parent_checkpoint.read_text())['elites']:
    p = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if p.is_file() and function(p.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('icon body does not equal a reproduced parent')
scan = '''        while (*current && *current != ' ') {
            wordWidth += g_unnamed698a08->getCharacterWidth(*current);
            ++current;
        }'''
assert body.count(scan) == 1
options = []
for increment, accumulation, scope in itertools.product(range(3), range(2), range(2)):
    loop = scan
    if increment == 1:
        loop = loop.replace('getCharacterWidth(*current);\n            ++current;', 'getCharacterWidth(*current++);')
    elif increment == 2:
        loop = loop.replace("while (*current && *current != ' ')", "for (; *current && *current != ' '; ++current)")
        loop = loop.replace('            ++current;\n', '')
    if accumulation:
        loop = loop.replace('wordWidth += ', 'wordWidth = wordWidth + ')
    candidate = body.replace(scan, loop)
    if scope:
        candidate = candidate.replace('    const char* current = m_text.c_str();', '    int wordWidth;\n    const char* current = m_text.c_str();')
        candidate = candidate.replace('        int wordWidth = 2;', '        wordWidth = 2;')
    row = {'name': f'increment-{increment}-accumulation-{accumulation}-scope-{scope}'}
    if increment or accumulation or scope:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'word-scan-statement-and-counter-lifetime', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'DC5106 e58a2..e58b0 groups GetCharacterWidth, pointer increment and accumulation;5107 has no row and5108 owns the loop back edge. This supports testing a postincrement argument or loop-header increment against separate statements, without claiming the missing line identifies source text.',
        'DC5102 initializes the word width to2 inside the outer scan. No named locals survive. Test the declaration at that initialization versus an outer declaration in the5091..5095 gap, keeping the initialization and comparison at their recorded stages.',
        'The compound and explicit accumulation forms preserve the helper call, signed int width, character argument, update order and all loop predicates.',
        'The empty-label return and width-store/min boundary are preserved. All45 existing exact kb functions must be measured alongside the icon.',
    ],
}, indent=2) + '\n')
