#!/usr/bin/env python3
"""Search widget-result lifetimes with Complete's exit hotkey preserved."""
import argparse
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/townmgr.cpp').read_text()
start = source.index('TTownScreenWindow::TTownScreenWindow(')
body = source[start:source.index('\n}\n', start) + 2]
groups = {}
spans = []

scoped = re.compile(r'(?m)^    \{\n        (\w+)\* (\w+) = new (\w+)\(([\s\S]*?)\);\n        m_widgets.push_back\(\2\);\n    \}')
direct = re.compile(r'(?m)^    m_widgets.push_back\(new (\w+)\(([\s\S]*?)\)\);')

def add(match, typ, ctor_args):
    if 'setHotkey' in match.group() or '\n    }' in ctor_args:
        return
    group = ('backgrounds' if typ in ('border', 'bitmapBorder') else
             'buttons' if typ == 'button' else
             'labels' if typ in ('textWidget', 'bitmapBackedTextWidget') else 'icons')
    local = {'backgrounds': 'background', 'buttons': 'control', 'labels': 'label', 'icons': 'icon'}[group]
    expression = 'new ' + typ + '(' + ctor_args + ')'
    variants = {'expression': '    m_widgets.push_back(' + expression + ');'}
    for binding, pointer in (('derived', typ), ('base', 'widget')):
        variants[binding] = '    {\n        ' + pointer + '* ' + local + ' = ' + expression + ';\n        m_widgets.push_back(' + local + ');\n    }'
    groups.setdefault(group, []).append((match.group(), variants))
    spans.append(match.span())

for m in scoped.finditer(body):
    add(m, m[3], m[4])
for m in direct.finditer(body):
    if not any(a <= m.start() < b for a, b in spans):
        add(m, m[1], m[2])

axes = []
for group, edits in groups.items():
    options = [{'name': 'parent'}]
    for form in ('expression', 'derived', 'base'):
        if all(old == variants[form] for old, variants in edits):
            continue
        options.append({'name': form, 'replace': edits[0][1][form], 'extra_edits': [
            {'find': old, 'replace': variants[form]} for old, variants in edits[1:]]})
    axes.append({'name': group + '-result-lifetimes', 'find': edits[0][0], 'options': options})

exit_start = body.index('    {\n        button* control = new button(744, 544,')
exit_end = body.index('\n    }', exit_start) + len('\n    }')
exit_button = body[exit_start:exit_end]
assert 'control->setHotkey(1);' in exit_button
unscoped = '\n'.join(line[4:] if line.startswith('    ') else line
                     for line in exit_button.splitlines()[1:-1]).replace('control', 'exitButton')
axes.append({'name': 'exit-button-lifetime', 'find': exit_button, 'options': [
    {'name': 'scoped'}, {'name': 'function-scope', 'replace': unscoped}]})

loop = '''    widget** it = m_widgets.begin();
    if (it != m_widgets.end()) {
        do {
            if (*it)
                addWidget(*it, -1);
            else
                memError();
            ++it;
        } while (it != m_widgets.end());
    }'''
options = [{'name': 'guarded-do'}]
options.append({'name': 'for-iterator', 'replace': '''    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }'''})
options.append({'name': 'while-iterator', 'replace': '''    widget** it = m_widgets.begin();
    while (it != m_widgets.end()) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
        ++it;
    }'''})
ending = '\n}\n\nVA_COMPGEN(0x005c58b0'
for option in options[1:]:
    option['replace'] += ending
axes.append({'name': 'widget-attachment', 'find': loop + ending, 'options': options})

for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('source anchor is stale or ambiguous: ' + axis['name'])
    for option in axis['options']:
        for edit in option.get('extra_edits', []):
            if source.count(edit['find']) != 1:
                raise ValueError('extra source anchor is stale or ambiguous')
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'], 'axes': axes,
    'evidence': [
        'Parent includes the retail-proven exit-button setHotkey(1) before m_widgets.push_back.',
        'DC proves reserve, widget constructors and the final attachment sweep; retail expands the Complete-specific widget set.',
        'The missing hotkey changed the inline budget, invalidating conclusions from the old tail-only source family.',
        'Vary actual construction-result types/scopes and equivalent iterator lifetimes; retain canonical helpers, extents, IDs and widget order.',
    ],
}, indent=2) + '\n')
