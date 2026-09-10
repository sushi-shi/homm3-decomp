#!/usr/bin/env python3
"""Recover the dialog icon cursor's per-element advance from DC 5430."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()
a = source.index('        for (int k = 0; k < inRow; k++) {')
b = source.index('        iconY +=', a)
original = source[a:b]
assert original.count('firstInRow + k') == 7
replacement = original.replace('firstInRow + k', 'firstInRow').replace(
    '        }\n        firstInRow += inRow;\n',
    '            ++firstInRow;\n        }\n')
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'placement-cursor', 'find': original, 'options': [
        {'name': 'bulk-row-advance-control'},
        {'name': 'dc-per-element-advance', 'replace': replacement},
    ]}],
    'evidence': [
        'Fresh complete DC show/lines/asm/inline and retail sema summary/structure/source pass in build/preprocessor-audit/continued/dialog-fresh-*.txt.',
        'DC5425 at e5e7c indexes the icon using R11;5426 stores spriteY;5429 stores textX;5430 at e5ea8 advances R11 by one inside the loop. R6 independently advances the inner loop count at e5e74. Restore that source cursor lifetime.',
        'Retail loop strength reduction advances the icon pointer by0x4c and adds the row count to ECX before the loop. This permits the DC cursor form; it does not require a bulk source advance after the loop.',
        'DC5257 recomputes the quotient through __divls; retain the separate perRow calculation. DC5433 uses the second row-array elements; retain those indices and the retail20-pixel gap.',
        'Two finite states, all kb siblings scored; no artificial width fillers or helper changes.',
    ],
}, indent=2) + '\n')
