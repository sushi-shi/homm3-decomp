#!/usr/bin/env python3
"""Restore the two recorded generator references in rollover text."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/advmgr.cpp').read_text()
start = source.index('void advManager::setRolloverText(NewmapCell* testCell, int rx, int ry)\n{')
body = source[start:source.index('\n}\n', start) + 2]
anchor = '''        generator* mapGenerator = &g_game->m_generators[cell->m_extraInfo];
        int owner = mapGenerator->getOwner();
        int generatorType = mapGenerator->m_genType;'''
assert body.count(anchor) == 2
replacement = '''        // DC local: this_generator (generator&), 3321/3342.
        generator& mapGenerator = g_game->m_generators[cell->m_extraInfo];
        int owner = mapGenerator.getOwner();
        // DC local: type (int), 3324/3345.
        int generatorType = mapGenerator.m_genType;'''
options = []
for first, second in itertools.product(range(2), repeat=2):
    parts = body.split(anchor)
    candidate = parts[0] + (replacement if first else anchor) + parts[1] + (replacement if second else anchor) + parts[2]
    row = {'name': f'generator1-reference-{first}-generator4-reference-{second}'}
    if first or second:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/advmgr.cpp', 'units': ['advmgr'],
    'axes': [{'name': 'recorded-generator-reference-boundaries', 'find': body, 'options': options}],
    'evidence': [
        'Fresh full-build checkpoint after the exact endgame recovery; unchanged-source and opposite-corner controls are required.',
        'DC SetRolloverText records this_generator as generator& twice at sp+0x58 and sp+0x54. The candidate incorrectly uses two pointers, and its broad intro mapping does not associate those local names with the AST audit.',
        'DC3321/3342 binds the vector element; 3323/3344 calls get_owner; 3324/3345 reads type. Preserve that statement order and both existing arm scopes.',
        'Four states measure each reference and their combination; both proven references are retained unless retail semantics or ABI contradict them. A lower score alone is not a rejection.',
    ],
}, indent=2) + '\n')
