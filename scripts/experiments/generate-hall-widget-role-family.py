#!/usr/bin/env python3
"""Vary the four genuine Hall widget-result roles independently."""
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

groups = {}
pattern = r'(?m)^            \{\n                (iconWidget|textWidget)\* (\w+) = new \1\(([\s\S]*?)\);\n                m_widgets.push_back\(\2\);\n            \}'
for match in re.finditer(pattern, body):
    typ, local, ctor_args = match.groups()
    if '"TPTHBar.def"' in ctor_args:
        role = 'name-bar'
    elif '"TPTHChk.def"' in ctor_args:
        role = 'built-check'
    elif typ == 'textWidget':
        role = 'building-name'
    else:
        role = 'building-image'
    groups.setdefault(role, []).append((match.group(), {
        'base-result': match.group().replace(typ + '* ' + local, 'widget* ' + local, 1),
        'constructor-expression': '            m_widgets.push_back(new ' + typ + '(' + ctor_args + '));',
    }))
if set(groups) != {'name-bar', 'built-check', 'building-name', 'building-image'}:
    raise ValueError('missing retail widget roles')
if any(len(edits) != 9 for edits in groups.values()):
    raise ValueError('expected four widget roles in each Complete town case')
axes = []
for role, edits in groups.items():
    options = [{'name': 'parent'}]
    for form in ('base-result', 'constructor-expression'):
        options.append({'name': form, 'replace': edits[0][1][form], 'extra_edits': [
            {'find': old, 'replace': choices[form]} for old, choices in edits[1:]]})
    axes.append({'name': role + '-result-lifetime', 'find': edits[0][0], 'options': options})

m = re.search(r'    m_widgets.push_back\(new bitmapBorder\(3, 555, 741, 18, 501,([\s\S]*?)\)\);', body)
ctor = 'new bitmapBorder(3, 555, 741, 18, 501,' + m[1] + ')'
options = [{'name': 'parent'}]
for pointer in ('widget', 'bitmapBorder'):
    options.append({'name': pointer + '-scoped', 'replace': '    {\n        ' + pointer + '* statusBar = ' + ctor + ';\n        m_widgets.push_back(statusBar);\n    }'})
options.append({'name': 'function-scope-result', 'replace': '    bitmapBorder* statusBar = ' + ctor + ';\n    m_widgets.push_back(statusBar);'})
axes.append({'name': 'status-bar-lifetime', 'find': m.group(), 'options': options})
m = re.search(r'    \{\n        widget\* label = new textWidget\(0, 0, 800, 30,([\s\S]*?)\);\n        m_widgets.push_back\(label\);\n    \}', body)
axes.append({'name': 'title-lifetime', 'find': m.group(), 'options': [
    {'name': 'parent'},
    {'name': 'derived-result', 'replace': m.group().replace('widget* label', 'textWidget* label')},
    {'name': 'constructor-expression', 'replace': '    m_widgets.push_back(new textWidget(0, 0, 800, 30,' + m[1] + '));'},
]})
for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('stale or ambiguous anchor: ' + axis['name'])
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'], 'axes': axes,
    'evidence': [
        'Reproduced Hall parents: ' + ', '.join(parents),
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'The earlier coarse family changed all 36 slot widgets together. Retail differs at selected insertion expansions and temporary homes.',
        'Split the actual name-bar/name/image/built-check roles so their result types and lifetimes can vary independently.',
        'Keep coordinate arrays const, their retail values/extents, constructor/call order, hotkey, and attachment loop unchanged.',
    ],
}, indent=2) + '\n')
