#!/usr/bin/env python3
"""Refine Hall's reproduced parent with real coordinate-walker lifetimes."""
import argparse
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
    start = text.index('THallWindow::THallWindow(')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
parent = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in parent['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/townmgr.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('Hall function is not a reproduced retained parent')

start = body.index('    switch (which) {')
end = body.index('\n\n    m_widgets.push_back(new bitmapBorder(3', start)
switch = body[start:end]
options = [{'name': 'parent'}]
for traversal in ('for-index-last', 'for-index-first', 'do-index-last'):
    for coordinates in ('dereference', 'pixel-locals'):
        def change_case(match):
            label, contents = match.groups()
            row_matches = set(re.findall(r'hallX\[(\d+)\]\[i\]', contents))
            if len(row_matches) != 1:
                raise ValueError('missing coordinate row: ' + label)
            row = next(iter(row_matches))
            header = re.search(r'        for \(i = 0; i < (\d+); i\+\+\) \{\n', contents)
            if not header:
                raise ValueError('missing positive bounded loop: ' + label)
            count = int(header[1])
            assert count > 0
            prefix, loop = contents[:header.start()], contents[header.end():]
            assert loop.endswith('        }\n')
            loop = loop[:-len('        }\n')]
            declarations = f'        const int* hallRowX = hallX[{row}];\n        const int* hallRowY = hallY[{row}];\n'
            loop = loop.replace(f'hallX[{row}][i]', '*hallRowX').replace(f'hallY[{row}][i]', '*hallRowY')
            if coordinates == 'pixel-locals':
                loop = loop.replace('slotX[*hallRowX]', 'slotPixelX').replace('slotY[*hallRowY]', 'slotPixelY')
                loop = '            const int slotPixelX = slotX[*hallRowX];\n            const int slotPixelY = slotY[*hallRowY];\n' + loop
            if traversal == 'do-index-last':
                walked = '        i = 0;\n        do {\n' + loop + \
                    f'            ++hallRowX;\n            ++hallRowY;\n            ++i;\n        }} while (i < {count});\n'
            else:
                update = '++hallRowX, ++hallRowY, ++i' if traversal == 'for-index-last' else '++i, ++hallRowX, ++hallRowY'
                walked = f'        for (i = 0; i < {count}; {update}) {{\n' + loop + '        }\n'
            return '    case ' + label + ': {\n' + prefix + declarations + walked + '        break;\n    }\n'
        replacement, count = re.subn(r'    case (TOWN_\w+):\n([\s\S]*?)        break;\n', change_case, switch)
        if count != 9:
            raise ValueError('expected the nine retail town cases')
        options.append({'name': traversal + '-' + coordinates, 'replace': replacement})
axes = [{'name': 'coordinate-walker-lifetimes', 'find': switch, 'options': options}]

m = re.search(r'    m_widgets.push_back\(new bitmapBorder\(3, 555, 741, 18, 501,([\s\S]*?)\)\);', body)
ctor = 'new bitmapBorder(3, 555, 741, 18, 501,' + m[1] + ')'
options = [{'name': 'parent'}]
for pointer in ('widget', 'bitmapBorder'):
    options.append({'name': pointer + '-scoped', 'replace': '    {\n        ' + pointer + '* statusBar = ' + ctor + ';\n        m_widgets.push_back(statusBar);\n    }'})
options.append({'name': 'function-scope-result', 'replace': '    bitmapBorder* statusBar = ' + ctor + ';\n    m_widgets.push_back(statusBar);'})
axes.append({'name': 'status-bar-lifetime', 'find': m.group(), 'options': options})

m = re.search(r'    \{\n        widget\* label = new textWidget\(0, 0, 800, 30,([\s\S]*?)\);\n        m_widgets.push_back\(label\);\n    \}', body)
options = [{'name': 'parent'}, {'name': 'derived-result', 'replace': m.group().replace('widget* label', 'textWidget* label')},
           {'name': 'constructor-expression', 'replace': '    m_widgets.push_back(new textWidget(0, 0, 800, 30,' + m[1] + '));'}]
axes.append({'name': 'title-lifetime', 'find': m.group(), 'options': options})
for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('stale or ambiguous source anchor: ' + axis['name'])
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'], 'axes': axes,
    'evidence': [
        'Reproduced Hall parents: ' + ', '.join(parents),
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Retail advances the const coordinate-row address before incrementing the widget ID; the parent schedules these in reverse.',
        'Preserve all four DC-proven const arrays, nine Complete rows, bounded ascending loops, widget order and canonical APIs.',
        'Positive constant loop lengths make do traversal equivalent; coordinate locals read only immutable local arrays.',
        'Status-bar/title lifetimes couple to the remaining constructor EH homes; no padding or pragma alternatives.',
    ],
}, indent=2) + '\n')
