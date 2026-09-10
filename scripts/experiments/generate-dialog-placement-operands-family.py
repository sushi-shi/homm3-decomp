#!/usr/bin/env python3
"""Dialog array homes and triple-row operand order after cursor recovery."""
import argparse
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('output', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
s = (root / 'src/kb.cpp').read_text()
arrays = '''    // Before normalization: label_height.
    int labelHeight[g_dialogIconMaxRows];
    // Before normalization: icon_height.
    int iconHeight[g_dialogIconMaxRows];'''
reversed_arrays = '''    // Before normalization: icon_height.
    int iconHeight[g_dialogIconMaxRows];
    // Before normalization: label_height.
    int labelHeight[g_dialogIconMaxRows];'''
addition = '''            dialogInfo.m_icons[firstInRow + 2].m_spriteX =
                dialogInfo.m_icons[firstInRow + 1].m_spriteX
                + dialogInfo.m_icons[firstInRow + 1].m_spriteWidth + 40;'''
reversed_addition = '''            dialogInfo.m_icons[firstInRow + 2].m_spriteX =
                dialogInfo.m_icons[firstInRow + 1].m_spriteWidth
                + dialogInfo.m_icons[firstInRow + 1].m_spriteX + 40;'''
assert s.count(arrays) == s.count(addition) == 1
# Require the adopted, independently reproduced cursor family as parent.
parent = root / 'build/source-families/dfd00b78016955893f91'
checkpoint = json.loads((parent / 'checkpoint.json').read_text())
parent_id = '091ea12557bf81417b095134'
assert any(row['id'] == parent_id for row in checkpoint['elites'])
def function(text):
    start = text.index('void calculateNormalDialogSize(TNormalDialogInfo& dialogInfo)\n{')
    return text[start:text.index('\n}\n', start) + 2]
reproduced = parent / 'candidates' / parent_id / 'repeat/tree/src/kb.cpp'
assert function(reproduced.read_text()) == function(s), 'authored parent changed'

a.output.write_text(json.dumps({
    'schema':1, 'source':'src/kb.cpp', 'units':['kb'],
    'axes':[
        {'name':'array-declaration-order', 'find':arrays, 'options':[
            {'name':'label-before-icon-control'},
            {'name':'icon-before-label', 'replace':reversed_arrays}]},
        {'name':'triple-row-addition-order', 'find':addition, 'options':[
            {'name':'x-plus-width-control'},
            {'name':'width-plus-x', 'replace':reversed_addition}]},
    ],
    'evidence':[
        'Parent091ea12557bf81417b095134 in dfd00b78016955893f91 independently reproduced99.9719 and was adopted then full-built with all gates passing.',
        'DC records both int[2] arrays;5219/5220 initializes icon then label. Declaration order is not recovered. Retail operands at+11..1a,+b1,+d0..d8,+2a0/+2a4,+346 exchange the two array homes. Retest this previously byte-flat declaration choice on the newly recovered cursor lifetime.',
        'DC5407 is one rightmost-icon placement group using the center icon x and width. Both additions are semantically equivalent; do not infer source operand order from SH4 register scheduling. Retail+435/+43b loads x into EDX and width into EAX; the candidate does the converse. Test source addition order against VC6.',
        'Four finite combinations preserve canonical helpers, seven local types, recovered cursor and all field accesses. All kb sibling scores are reviewed.',
    ],
},indent=2)+'\n')
