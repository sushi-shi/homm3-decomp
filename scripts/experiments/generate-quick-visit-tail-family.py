#!/usr/bin/env python3
"""Compose QuickInfo's DC visit branches with the restored fountain arm."""
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
if args.parent_result.parent.name != 'repeat':
    raise ValueError('parent must be a reproduced result')
if function((args.parent_result.parent / 'tree/src/advmgr.cpp').read_text()) != body:
    raise ValueError('authored body differs from reproduced parent')

def arm(text, case, following):
    start = text.index('            case ' + case)
    return text[start:text.index('            case ' + following, start)]

arena = arm(body, 'ARENA:', 'BORDER_GUARD:')
buoy = arm(body, 'BUOY:', 'CLOVER_FIELD:')
lean = arm(body, 'LEAN_TO:', 'LIBRARY:')

def separate_formats(text, indent):
    old = indent + '''sprintf(tempText, visitFormat,
''' + indent + '''    visited
''' + indent + '''        ? (*g_generalText)[
''' + indent + '''              GENERAL_TEXT_VISITED_OBJECT]
''' + indent + '''        : (*g_generalText)[
''' + indent + '''              GENERAL_TEXT_UNVISITED_OBJECT]);'''
    new = indent + '''if (visited)
''' + indent + '''    sprintf(tempText, visitFormat,
''' + indent + '''            (*g_generalText)[
''' + indent + '''                GENERAL_TEXT_VISITED_OBJECT]);
''' + indent + '''else
''' + indent + '''    sprintf(tempText, visitFormat,
''' + indent + '''            (*g_generalText)[
''' + indent + '''                GENERAL_TEXT_UNVISITED_OBJECT]);'''
    if text.count(old) != 1:
        raise ValueError('format anchor is stale')
    return text.replace(old, new)

# Every selected body has balanced, non-nested textual braces. Track braces
# only to preserve each existing arm's source while adding its real guard.
def nested_guards(text):
    anchor = '                if (cell->m_isTrigger && currHero) {'
    if text.count(anchor) != 5:
        raise ValueError('expected five trigger/hero guards')
    while anchor in text:
        start = text.index(anchor)
        opening = text.index('{', start)
        level = 1
        closing = opening + 1
        while level:
            if text[closing] == '{': level += 1
            elif text[closing] == '}': level -= 1
            closing += 1
        inner = text[opening + 1:closing - 1].rstrip()
        lines = inner.splitlines()
        inner = '\n'.join('    ' + line if line else line for line in lines)
        text = (text[:start] + '                if (cell->m_isTrigger) {\n'
                '                    if (currHero) {' + inner + '\n'
                '                    }\n                }' + text[closing:])
    return text

options = []
for choices in itertools.product(range(2), repeat=4):
    arena_format, buoy_format, guards, lean_carrier = choices
    candidate = body
    if arena_format:
        candidate = candidate.replace(arena, separate_formats(arena, '                    '))
    if buoy_format:
        candidate = candidate.replace(buoy, separate_formats(buoy, '                        '))
    if lean_carrier:
        candidate = candidate.replace(lean, lean.replace('                    z = ', '                    visited = ').replace('                    if (z)', '                    if (visited)'))
    if guards:
        candidate = nested_guards(candidate)
    row = {'name': '-'.join(map(str, choices))}
    if any(choices): row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/advmgr.cpp', 'units': ['advmgr'],
    'axes': [{'name': 'visited-format-and-guard-composition', 'find': body, 'options': options}],
    'evidence': [
        'Reproduced parent: ' + str(args.parent_result) + ' id ' + parent['id'],
        'The parent retains both direct generator evaluations, the DC fountain query, its visited carrier, low-to-high masked sum and separate formatting branches. These facts are fixed throughout this family.',
        'DC7620 tests ARENA visited;7622/7626 separately call sprintf,7628 appends. DC7682/7684/7688/7690 repeats these stages for BUOY. Retail shares ARENA/BUOY/fountain format blocks. Earlier isolated rewrites lost sharing; compose all three source forms before judging that failure.',
        'DC records trigger then hero tests separately:7614/7616 ARENA,7777/7779 DEAD_GUY,8046/8048 LEAN_TO,8525/8527 SIREN and8550/8552 STABLES. Compare all five nested guards with the current conjunctions, preserving evaluation order.',
        'DC8050 writes the LEAN_TO result into visited (r14+12, sp+0x40), while retail reuses the parameter slot. Test the true carrier instead of an explicit source assignment to z.',
        'Sixteen finite combinations; no new calls, helper replacements, invented diagnostics or shared-header changes. All93 exact advmgr siblings are measured.',
    ],
}, indent=2) + '\n')
