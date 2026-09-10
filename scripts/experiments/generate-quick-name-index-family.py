#!/usr/bin/env python3
"""Restore QuickInfo's cell-type name indexing and retest shared tails."""
import argparse
import itertools
import json
from pathlib import Path
import re

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

def arm(text, case, following):
    start = text.index('            case ' + case)
    return text[start:text.index('            case ' + following, start)]

arms = [arm(body, 'ARENA:', 'BORDER_GUARD:'), arm(body, 'BUOY:', 'CLOVER_FIELD:'), arm(body, 'FOUNTAIN_OF_FORTUNE:', 'FOUNTAIN_OF_YOUTH:')]
indent = '                        '
ternary = indent + '''sprintf(tempText, visitFormat,
''' + indent + '''    visited
''' + indent + '''        ? (*g_generalText)[
''' + indent + '''              GENERAL_TEXT_VISITED_OBJECT]
''' + indent + '''        : (*g_generalText)[
''' + indent + '''              GENERAL_TEXT_UNVISITED_OBJECT]);'''
branches = indent + '''if (visited)
''' + indent + '''    sprintf(tempText, visitFormat,
''' + indent + '''            (*g_generalText)[
''' + indent + '''                GENERAL_TEXT_VISITED_OBJECT]);
''' + indent + '''else
''' + indent + '''    sprintf(tempText, visitFormat,
''' + indent + '''            (*g_generalText)[
''' + indent + '''                GENERAL_TEXT_UNVISITED_OBJECT]);'''
assert arms[0].count(ternary) == 1
assert all(a.count(branches) == 1 for a in arms[1:])
name_pattern = r'g_adventureObjectNames\[([A-Z][A-Z_0-9]*)\]'
names = re.findall(name_pattern, body)
assert len(names) == 32, names
options = []
for choices in itertools.product(range(2), repeat=4):
    indexes, arena, buoy, fountain = choices
    candidate = body
    for i, choice in enumerate((arena, buoy, fountain)):
        if choice:
            old, new = (ternary, branches) if i == 0 else (branches, ternary)
            candidate = candidate.replace(arms[i], arms[i].replace(old, new))
    if indexes:
        candidate = re.sub(name_pattern, 'g_adventureObjectNames[cell->m_type]', candidate)
    row = {'name': '-'.join(map(str, choices))}
    if any(choices): row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/advmgr.cpp', 'units': ['advmgr'],
    'axes': [{'name': 'cell-type-index-and-format-lowering', 'find': body, 'options': options}],
    'evidence': [
        'Reproduced parent: ' + str(args.parent_result) + ' id ' + parent['id'],
        'DC7612 at16468 loads testCell+28 into the gQuickViewText subscript in ARENA, and7644 at165b2 does the same in BORDER_TENT sprintf. DC7886 at16e00 repeats this for fountain;8184 at17804 repeats it later. The cases use the cell type itself, not an already-specialized case constant.',
        'Restore the 32 literal object-name subscripts to cell->m_type; their cases select those values before the first call. Existing dynamic default/border/special-terrain subscripts and all helper argument enums stay unchanged.',
        'The Hall exact recovery showed that premature case specialization can change natural VC6 inlining even when later passes fold the index. Measure this same evidence-backed source issue here, with all93 exact advmgr siblings.',
        'DC formatting groups motivate both explicit branches and a conditional argument; either can lower to two sprintf calls. Preserve the visited/unvisited semantics and canonical calls without demanding equal source statement counts. Compose the three shared-tail spellings with the recovered index rather than rejecting it on an isolated score dip.',
        'The source-fact query, visited carriers, generator evaluations and nested guards from the parent are fixed. No synthetic mass, fake helper declarations, new inline pins or shared-header edits.',
    ],
}, indent=2) + '\n')
