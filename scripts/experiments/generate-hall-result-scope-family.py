#!/usr/bin/env python3
"""Hall widget-result lifetimes within the recovered construction groups."""
import argparse
import itertools
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/townmgr.cpp').read_text()

def function(text):
    start = text.index('THallWindow::THallWindow(int which)')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
parents = []
for row in json.loads(args.parent_checkpoint.read_text())['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/townmgr.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('Hall body does not equal a reproduced parent')
switch_start = body.index('    switch (which) {')
switch_end = body.index('\n\n    bitmapBorder* statusBar', switch_start)
switch = body[switch_start:switch_end]
groups = list(re.finditer(
    r'(?m)^            \{\n                (iconWidget)\* icon = new iconWidget\(([\s\S]*?)\);\n                m_widgets.push_back\(icon\);\n            \}'
    r'|^            m_widgets.push_back\(new (textWidget|iconWidget)\(([\s\S]*?)\)\);', switch))
if len(groups) != 36:
    raise ValueError('expected four construction groups in each of nine town loops')
tail_start = switch_end + 2
tail_end = body.index('    button* exitButton', tail_start)
tail = body[tail_start:tail_end]
tail_groups = list(re.finditer(
    r'    bitmapBorder\* statusBar = new (bitmapBorder)\(([\s\S]*?)\);\n    m_widgets.push_back\(statusBar\);'
    r'|    \{\n        textWidget\* label = new (textWidget)\(([\s\S]*?)\);\n        m_widgets.push_back\(label\);\n    \}', tail))
if len(tail_groups) != 3:
    raise ValueError('expected status bar, status label and title groups')
options = []
for loop_scope, tail_scope in itertools.product(range(4), repeat=2):
    new_switch = switch
    if loop_scope:
        for index, match in enumerate(groups):
            typ = match[1] or match[3]
            ctor = 'new ' + typ + '(' + (match[2] or match[4]) + ')'
            if loop_scope == 1:
                name = ('nameBar', 'buildingName', 'buildingImage', 'builtCheck')[index % 4]
                replacement = '            ' + typ + '* ' + name + ' = ' + ctor + ';\n            m_widgets.push_back(' + name + ');'
            else:
                replacement = '            buildingWidget = ' + ctor + ';\n            m_widgets.push_back(buildingWidget);'
            new_switch = new_switch.replace(match.group(), replacement)
        if loop_scope == 2:
            new_switch = re.sub(r'(        for \(i = 0; i < \d+; i\+\+\) \{\n)', r'\1            widget* buildingWidget;\n', new_switch)
        elif loop_scope == 3:
            new_switch = '    widget* buildingWidget;\n' + new_switch
    new_tail = tail
    if tail_scope:
        for index, match in enumerate(tail_groups):
            typ = match[1] or match[3]
            ctor = 'new ' + typ + '(' + (match[2] or match[4]) + ')'
            if tail_scope == 1:
                replacement = '    m_widgets.push_back(' + ctor + ');'
            elif tail_scope == 2:
                name = ('statusBar', 'statusLabel', 'titleLabel')[index]
                replacement = '    ' + typ + '* ' + name + ' = ' + ctor + ';\n    m_widgets.push_back(' + name + ');'
            else:
                replacement = '    statusWidget = ' + ctor + ';\n    m_widgets.push_back(statusWidget);'
            new_tail = new_tail.replace(match.group(), replacement)
        if tail_scope == 3:
            new_tail = '    widget* statusWidget;\n' + new_tail
    candidate = body[:switch_start] + new_switch + '\n\n' + new_tail + body[tail_end:]
    row = {'name': f'building-results-{loop_scope}-status-results-{tail_scope}'}
    if loop_scope or tail_scope:
        row['replace'] = candidate
    elif candidate != body:
        raise ValueError('control differs from current Hall body')
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'],
    'axes': [{'name': 'real-result-variable-scope', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'Hall currently has369 matched CFG blocks and remaining differences in result homes, the saved this pointer, and pointer-loop increment scheduling.',
        'DC4344..4347 groups each widget construction/append inside two nested loop scopes; only the four const layout arrays have recorded local names. Test distinct actual role results at loop scope and a shared widget result in the loop or function, instead of adding any operation.',
        'DC4441/4442/4444 constructs and appends the status bar, status label and title in that order, with no recorded local names. Test expression results, separate named results and a shared result across these real constructions.',
        'This differs from the exhausted per-role base/derived/fused family by varying whole-loop and shared-result lifetimes. Keep all helpers, array constness/values, arguments, constructor order, hotkey and attachment loop.',
    ],
}, indent=2) + '\n')
