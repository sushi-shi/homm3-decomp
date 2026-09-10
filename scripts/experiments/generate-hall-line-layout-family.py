#!/usr/bin/env python3
"""Test compact Hall construction groups with real vector/loop lifetimes."""
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
    start = text.index('THallWindow::THallWindow(')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
checkpoint = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in checkpoint['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/townmgr.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('Hall body does not equal a reproduced parent')

attachment = '''    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }'''
attachments = [attachment, '''    widget** it = m_widgets.begin();
    while (it != m_widgets.end()) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
        ++it;
    }''', '''    widget** it = m_widgets.begin();
    if (it != m_widgets.end()) {
        do {
            if (*it)
                addWidget(*it, -1);
            else
                memError();
            ++it;
        } while (it != m_widgets.end());
    }''']
assert body.count(attachment) == 1
scoped = list(re.finditer(r'(?m)^            \{\n                iconWidget\* icon = new iconWidget\(([\s\S]*?)\);\n                m_widgets.push_back\(icon\);\n            \}', body))
if len(scoped) != 18:
    raise ValueError('expected nine name bars and nine building images')
options = []
for bar, image, binding, iterator, counter in itertools.product(range(2), range(2), range(3), range(3), range(2)):
    candidate = body
    for match in scoped:
        role = bar if 'TPTHBar.def' in match[1] else image
        if role:
            candidate = candidate.replace(match.group(), '            m_widgets.push_back(new iconWidget(' + match[1] + '));')
    candidate = candidate.replace(attachment, attachments[iterator])
    if counter:
        candidate = candidate.replace('    int i;\n\n', '')
        # Each source case owns its counter; do not rely on VC6's old for scope.
        start = candidate.index('    switch (which) {')
        end = candidate.index('\n\n    bitmapBorder* statusBar', start)
        switch = candidate[start:end]
        def localize(match):
            label, contents = match.groups()
            return '    case ' + label + ': {\n' + contents.replace('for (i = 0;', 'for (int i = 0;') + '        break;\n    }\n'
        switch, count = re.subn(r'    case (TOWN_\w+):\n([\s\S]*?)        break;\n', localize, switch)
        if count != 9:
            raise ValueError('expected nine cases')
        candidate = candidate[:start] + switch + candidate[end:]
    if binding:
        anchor = '    m_widgets.reserve(77);\n'
        alias = '    std::vector<widget*>& widgets = m_widgets;\n' if binding == 1 else '    std::vector<widget*>* widgets = &m_widgets;\n'
        before, after = candidate.split(anchor)
        receiver = 'widgets.' if binding == 1 else 'widgets->'
        candidate = before + anchor + alias + after.replace('m_widgets.', receiver)
    choices = (bar, image, binding, iterator, counter)
    item = {'name': '-'.join(map(str, choices))}
    if any(choices):
        item['replace'] = candidate
    options.append(item)
assert len(options) == 72
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'],
    'axes': [{'name': 'construction-groups-and-vector-lifetimes', 'find': body, 'options': options}],
    'evidence': [
        'Reproduced parent(s): ' + ', '.join(parents),
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'DC 4344..4347 and corresponding town rows group each construction and append on one line; test fused expressions for the two roles still split in the parent.',
        'DC 4335 reserve precedes switch 4338. A real container reference/pointer after reserve is a bounded lifetime hypothesis, not inferred missing text.',
        'Keep all positive array constness, Complete extents/values, named helper boundaries, hotkey and widget order.',
        'Vary actual counter and attachment lifetimes; do not pad source or manufacture operations to match line counts.',
    ],
}, indent=2) + '\n')
