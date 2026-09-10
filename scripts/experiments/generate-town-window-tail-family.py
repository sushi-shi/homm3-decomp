#!/usr/bin/env python3
"""Refine a reproduced town-window parent at its remaining loop/tail sites."""
import argparse
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('window', choices=('hall', 'screen'))
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/townmgr.cpp').read_text()
name = 'THallWindow' if args.window == 'hall' else 'TTownScreenWindow'
def function(text):
    start = text.index(name + '::' + name + '(')
    return text[start:text.index('\n}\n', start) + 2]
body = function(source)
parent = json.loads(args.parent_checkpoint.read_text())
parents = []
for record in parent['elites']:
    tree = args.parent_checkpoint.parent / 'candidates' / record['id'] / 'repeat/tree/src/townmgr.cpp'
    if not tree.is_file():
        raise ValueError('parent was not reproduced: ' + record['id'])
    if function(tree.read_text()) == body:
        parents.append(record['id'])
if not parents:
    raise ValueError('authored function does not equal any reproduced retained parent')
axes = []

def scoped_result(kind, ctor_prefix, local_name, *, include_function_local=True):
    pattern = r'    \{\n        (\w+)\* (\w+) = new ' + kind + r'\(' + re.escape(ctor_prefix) + r'([\s\S]*?)\);\n        m_widgets.push_back\(\2\);\n    \}'
    m = re.search(pattern, body)
    if not m:
        raise ValueError(kind + ': scoped constructor anchor absent')
    typ, local, rest = m.groups()
    ctor = 'new ' + kind + '(' + ctor_prefix + rest + ')'
    options = [{'name': 'parent'}]
    other = 'widget' if typ != 'widget' else kind
    options.append({'name': 'alternate-pointer-type', 'replace': m.group().replace(typ + '* ' + local, other + '* ' + local, 1)})
    options.append({'name': 'constructor-expression', 'replace': '    m_widgets.push_back(' + ctor + ');'})
    if include_function_local:
        options.append({'name': 'function-scope-result', 'replace': '    ' + typ + '* ' + local_name + ' = ' + ctor + ';\n    m_widgets.push_back(' + local_name + ');'})
    axes.append({'name': local_name + '-result', 'find': m.group(), 'options': options})

if args.window == 'screen':
    scoped_result('button', '744, 544, 48, 30, EXIT_BUTTON_ID,', 'exitButton')
    scoped_result('bitmapBackedTextWidget', '7, 555, 734, 19, 0,', 'statusBar')
    scoped_result('textWidget', '85, 387, 147, 20, 0,', 'townTitle')
else:
    switch_start = body.index('    switch (which) {')
    switch_end = body.index('\n\n    m_widgets.push_back(new bitmapBorder(3', switch_start)
    switch = body[switch_start:switch_end]
    options = [{'name': 'parent'}]
    for counter in ('shared', 'case-local'):
        for row_binding in ('direct', 'pointer', 'array-reference'):
            if (counter, row_binding) == ('shared', 'direct'):
                continue
            def change_case(m):
                label, contents = m.groups()
                # Each case uses one town row. Keep all existing widget/result
                # scopes and expressions except the actual row binding.
                rows = set(re.findall(r'hallX\[(\d+)\]\[i\]', contents))
                if len(rows) != 1:
                    raise ValueError(label + ': ambiguous row')
                row = next(iter(rows))
                if counter == 'case-local':
                    contents = contents.replace('for (i = 0;', 'for (int i = 0;')
                if row_binding != 'direct':
                    if row_binding == 'pointer':
                        decl = f'        const int* hallRowX = hallX[{row}];\n        const int* hallRowY = hallY[{row}];\n'
                    else:
                        decl = f'        const int (&hallRowX)[18] = hallX[{row}];\n        const int (&hallRowY)[18] = hallY[{row}];\n'
                    at = contents.index('        for (')
                    contents = contents[:at] + decl + contents[at:]
                    contents = contents.replace(f'hallX[{row}][i]', 'hallRowX[i]').replace(f'hallY[{row}][i]', 'hallRowY[i]')
                return '    case ' + label + ': {\n' + contents + '        break;\n    }\n'
            replacement = re.sub(r'    case (TOWN_\w+):\n([\s\S]*?)        break;\n', change_case, switch)
            option = {'name': counter + '-' + row_binding, 'replace': replacement}
            if counter == 'case-local':
                option['extra_edits'] = [{'find': '    int i;\n\n    m_widgets.reserve(77);', 'replace': '    m_widgets.reserve(77);'}]
            options.append(option)
    axes.append({'name': 'building-row-and-counter-lifetimes', 'find': switch, 'options': options})
    m = re.search(r'    m_widgets.push_back\(new bitmapBorder\(3, 555, 741, 18, 501,([\s\S]*?)\)\);', body)
    ctor = 'new bitmapBorder(3, 555, 741, 18, 501,' + m[1] + ')'
    options = [{'name': 'parent'}]
    for typ in ('widget', 'bitmapBorder'):
        options.append({'name': typ + '-scoped', 'replace': '    {\n        ' + typ + '* statusBar = ' + ctor + ';\n        m_widgets.push_back(statusBar);\n    }'})
    options.append({'name': 'function-scope-result', 'replace': '    bitmapBorder* statusBar = ' + ctor + ';\n    m_widgets.push_back(statusBar);'})
    axes.append({'name': 'status-bar-result', 'find': m.group(), 'options': options})
    scoped_result('textWidget', '0, 0, 800, 30,', 'hallTitle', include_function_local=False)

payload = {'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'], 'axes': axes,
           'evidence': ['Reproduced parent(s): ' + ', '.join(parents),
                        'Parent checkpoint: ' + str(args.parent_checkpoint),
                        'Retail diff localizes remaining allocation/EH result homes and loop induction/store scheduling.',
                        'DC arrays, constructor/append calls, setHotkey, text operator[], traversal and Complete extents retained.']}
args.output.write_text(json.dumps(payload, indent=2) + '\n')
