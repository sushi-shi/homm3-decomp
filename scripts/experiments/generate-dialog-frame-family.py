#!/usr/bin/env python3
"""Dialog row ownership and box statement groups from a reproduced parent."""
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
    start = text.index('void calculateNormalDialogSize(TNormalDialogInfo& dialogInfo)\n{')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
checkpoint = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in checkpoint['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('dialog body does not equal a reproduced parent')
axes = []
anchor = '''    // Before normalization: icon_height.
    int iconHeight[g_dialogIconMaxRows];
    // Before normalization: label_height.
    int labelHeight[g_dialogIconMaxRows];'''
axes.append({'name': 'row-array-declaration-order', 'find': anchor, 'options': [
    {'name': 'control'}, {'name': 'label-before-icon', 'replace': '''    // Before normalization: label_height.
    int labelHeight[g_dialogIconMaxRows];
    // Before normalization: icon_height.
    int iconHeight[g_dialogIconMaxRows];'''},
]})
anchor = '''    if (dialogInfo.m_mbType != NORMAL_DIALOG_POPUP
        && dialogInfo.m_width < 256)'''
axes.append({'name': 'minimum-width-branch-scope', 'find': anchor, 'options': [
    {'name': 'independent-control'}, {'name': 'dc-else-if', 'replace': anchor.replace('    if (', '    else if (')},
]})
anchor = '    int boxHeight = dialogInfo.m_textWidgetHeight + 60;'
axes.append({'name': 'height-base-and-text-stages', 'find': anchor, 'options': [
    {'name': 'fused-control'},
    {'name': 'base-then-text', 'replace': '    int boxHeight = 60;\n    boxHeight += dialogInfo.m_textWidgetHeight;'},
    {'name': 'text-then-base', 'replace': '    int boxHeight = dialogInfo.m_textWidgetHeight;\n    boxHeight += 60;'},
]})
anchor = '        boxHeight += 20 * iconRows - 20;'
axes.append({'name': 'inter-row-gap-product', 'find': anchor, 'options': [
    {'name': 'expanded-control'}, {'name': 'dc-row-count-minus-one', 'replace': '        boxHeight += 20 * (iconRows - 1);'},
]})
anchor = '''    dialogInfo.m_width =
        (max(dialogInfo.m_width, dialogInfo.m_textWidgetWidth + 50) + 63)
        & ~63;'''
axes.append({'name': 'maximum-and-rounding-stages', 'find': anchor, 'options': [
    {'name': 'fused-control'}, {'name': 'separate-groups', 'replace': '''    dialogInfo.m_width = max(dialogInfo.m_width, dialogInfo.m_textWidgetWidth + 50);
    dialogInfo.m_width = (dialogInfo.m_width + 63) & ~63;'''},
]})
for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('stale anchor: ' + axis['name'])
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'], 'axes': axes,
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced source parent(s): ' + ', '.join(parents),
        'Retail first divergence is row-array initialization/home order. DC records both int[2] arrays and icon then label initialization at 5219/5220, but does not recover declaration order.',
        'DC 5317/5319 then 5321/5323 scopes and branch to join support else-if; retail skips the second width predicate after the popup store.',
        'DC 5328 materializes the base height; 5329 adds text height. Probe meaningful value stages with Complete constants.',
        'DC 5336 computes row count minus one before multiplying the gap; preserve the Complete 20-pixel gap.',
        'DC 5312 maximum and 5315 rounding form distinct statement groups; retest their separation with corrected branch scope.',
        '48 finite source combinations preserve named locals, types, array initialization loop and canonical helper calls. No source padding or dummy operations.',
    ],
}, indent=2) + '\n')
