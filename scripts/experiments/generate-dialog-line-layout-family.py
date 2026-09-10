#!/usr/bin/env python3
"""Dialog sizing families from DC statement groups and retail operand homes."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()
start = source.index('void calculateNormalDialogSize(TNormalDialogInfo& dialogInfo)\n{')
end = source.index('\n}\n', start) + 2
body = source[start:end]
axes = []

start = body.index('    // Before normalization: icon_height.')
end = body.index('    for (i = 0; i < 8; i++)', start)
anchor = body[start:end]
# DC 5207 initializes widest_icon, 5209 initializes spacing; 5219/5220
# initialize the row arrays and 5223 begins the icon count.
widest = '    // Before normalization: widest_icon.\n    long widestIcon = 0;\n'
spacing = '    // Before normalization: spacing.\n    long spacing = 0;\n'
assert anchor.count(widest) == anchor.count(spacing) == 1
reordered = anchor.replace(widest, '').replace(spacing, '')
reordered = widest + spacing + reordered
late_count = reordered.replace('    int numIcons = 0;\n', '') + '    int numIcons = 0;\n'
axes.append({'name': 'declaration-and-initialization-order', 'find': anchor, 'options': [
    {'name': 'control'}, {'name': 'dc-widest-before-spacing', 'replace': reordered},
    {'name': 'dc-order-and-count-at-counting-loop', 'replace': late_count},
]})

start = body.index('            widestIcon = std::_cpp_max(')
end = body.index('            iconHeight[row] =', start)
anchor = body[start:end]
wrapper = anchor.replace('std::_cpp_max(', 'max(')
original_operands = '''            widestIcon = max(dialogInfo.m_icons[i].m_spriteWidth, widestIcon);
            widestIcon = max(dialogInfo.m_icons[i].m_textWidth, widestIcon);
            spacing = max(spacing, dialogInfo.m_icons[i].m_textWidth
                                      - dialogInfo.m_icons[i].m_spriteWidth);
'''
axes.append({'name': 'maximum-call-boundary', 'find': anchor, 'options': [
    {'name': 'direct-std-reference-control'},
    {'name': 'canonical-value-wrapper', 'replace': wrapper},
    {'name': 'canonical-wrapper-dc-operands', 'replace': original_operands},
]})

anchor = '''        dialogInfo.m_textWidgetHeight =
            currentFont->lineLength(dialogInfo.m_dialogText.c_str(),
                                   dialogInfo.m_textWidgetWidth)
            * currentFont->m_fs.m_height;'''
options = [{'name': 'fused-control'}]
call = '''currentFont->lineLength(dialogInfo.m_dialogText.c_str(),
                                    dialogInfo.m_textWidgetWidth)'''
for label, multiplication in (
    ('line-count-then-scale', 'lines * currentFont->m_fs.m_height'),
    ('line-count-then-height-times-lines', 'currentFont->m_fs.m_height * lines'),
):
    options.append({'name': label, 'replace': '        int lines = ' + call + ';\n        dialogInfo.m_textWidgetHeight = ' + multiplication + ';'})
options.append({'name': 'height-field-in-two-stages', 'replace': '        dialogInfo.m_textWidgetHeight = ' + call + ';\n        dialogInfo.m_textWidgetHeight *= currentFont->m_fs.m_height;'})
axes.append({'name': 'line-measure-and-pixel-height-statements', 'find': anchor, 'options': options})

anchor = '''    dialogInfo.m_width =
        (max(dialogInfo.m_width, dialogInfo.m_textWidgetWidth + 50) + 63)
        & ~63;'''
axes.append({'name': 'width-maximum-and-rounding-statements', 'find': anchor, 'options': [
    {'name': 'fused-control'}, {'name': 'dc-separate-stages', 'replace': '''    dialogInfo.m_width = max(dialogInfo.m_width, dialogInfo.m_textWidgetWidth + 50);
    dialogInfo.m_width = (dialogInfo.m_width + 63) & ~63;'''},
]})
for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('stale or ambiguous anchor: ' + axis['name'])
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'], 'axes': axes,
    'evidence': [
        'DC 5207/5209 initialize widest_icon then spacing; 5219/5220 initialize arrays; counting starts at 5223.',
        'DC 5284 obtains the line count and 5285 scales it to pixels. Separate statement groups support a returned-value lifetime hypothesis, not exact missing source text.',
        'DC 5312 maximum and 5315 rounding are distinct statement groups.',
        'Retail 0x4f5d80 maximum expansions copy both inputs to fresh stack homes. Direct const-reference std::_cpp_max selects original objects instead; test the existing canonical by-value wrapper.',
        'Preserve the reference parameter, long locals, all seven named DC local facts, width_changed do-loop, Complete geometry constants, and original APIs.',
        '72 bounded source combinations; no blank-line padding, synthetic operations, false helpers or inline-control directives.',
    ],
}, indent=2) + '\n')
