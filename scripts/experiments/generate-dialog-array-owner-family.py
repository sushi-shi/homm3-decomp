#!/usr/bin/env python3
"""Dialog array-element and center-icon lifetimes after cursor recovery."""
import argparse
import itertools
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('output', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
s = (root / 'src/kb.cpp').read_text()
begin = s.index('void calculateNormalDialogSize(TNormalDialogInfo& dialogInfo)\n{')
body = s[begin:s.index('\n}\n',begin)+2]
rows = '''            iconHeight[row] = max(iconHeight[row],
                                        dialogInfo.m_icons[i].m_spriteHeight);
            labelHeight[row] = max(labelHeight[row],
                                      dialogInfo.m_icons[i].m_textHeight);'''
triple_begin = body.index('        case g_dialogIconRowTriple:\n')
triple_end = body.index('        case g_dialogIconRowQuad:\n',triple_begin)
triple = body[triple_begin:triple_end]
initial = '        iconHeight[i] = 0;\n        labelHeight[i] = 0;'
assert body.count(rows) == body.count(initial) == 1
options = []
for row_kind, chain_init, center_kind in itertools.product(range(3),range(2),range(3)):
    candidate = body
    if row_kind:
        prefix = 'int&' if row_kind == 1 else 'int*'
        take = '' if row_kind == 1 else '&'
        icon = 'rowIconHeight' if row_kind == 1 else '(*rowIconHeight)'
        label = 'rowLabelHeight' if row_kind == 1 else '(*rowLabelHeight)'
        candidate = candidate.replace(rows,
            f'            {prefix} rowIconHeight = {take}iconHeight[row];\n' +
            rows[:rows.index('            labelHeight')].replace('iconHeight[row]',icon) +
            f'            {prefix} rowLabelHeight = {take}labelHeight[row];\n' +
            rows[rows.index('            labelHeight'):].replace('labelHeight[row]',label))
    if chain_init:
        candidate = candidate.replace(initial,'        labelHeight[i] = iconHeight[i] = 0;')
    if center_kind:
        decl = ('type_dialog_icon& centerIcon = dialogInfo.m_icons[firstInRow + 1];' if center_kind == 1 else
                'type_dialog_icon* centerIcon = &dialogInfo.m_icons[firstInRow + 1];')
        receiver = 'centerIcon.' if center_kind == 1 else 'centerIcon->'
        replacement = triple.replace('        case g_dialogIconRowTriple:\n',
            '        case g_dialogIconRowTriple: {\n            ' + decl + '\n')
        replacement = replacement.replace('dialogInfo.m_icons[firstInRow + 1].',receiver)
        replacement = replacement.replace('            break;\n','            break;\n        }\n')
        candidate = candidate.replace(triple,replacement)
    option = {'name':f'row-{row_kind}-chain-{chain_init}-center-{center_kind}'}
    if row_kind or chain_init or center_kind: option['replace']=candidate
    options.append(option)
a.output.write_text(json.dumps({
    'schema':1,'source':'src/kb.cpp','units':['kb'],
    'axes':[{'name':'actual-array-and-icon-owners','find':body,'options':options}],
    'evidence':[
        'Current full-built cursor reconstruction99.9719; complete DC and retail evidence pass available in continued/dialog-fresh-* and dialog-cursor-*.',
        'DC5248 forms the icon-height element address in R12, loads its value, calls canonical max and writes back through R12;5250 does the same for label height. Test direct subscript versus actual element reference/pointer lifetimes within that scope, keeping both proven int[2] arrays and canonical max calls.',
        'DC5219 stores icon height zero,5220 label height zero, in the same ascending loop. A chained assignment is an explicit equivalent-expression hypothesis that retains this order and every store; distinct source rows do not uniquely recover the original punctuation.',
        'DC5402/5405/5407 computes the center icon address and consumes centerX and centerWidth when placing the rightmost icon. Test an actual reference/pointer scoped to the triple arm. No local type/name for that binding is recovered; this is a lifetime hypothesis. Retail+435/+43b exchanges those two field loads.',
        'Eighteen finite states with reviewed current-source control. No repeated/inert operations, alternate helpers, array types, or pragma changes. Score every kb sibling.',
    ],
},indent=2)+'\n')
