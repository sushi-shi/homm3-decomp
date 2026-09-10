#!/usr/bin/env python3
"""Authored widget/result lifetime alternatives after restoring DC helpers."""
import json
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
source = (root / 'src/levelupwindow.cpp').read_text()
axes = []
# All three forms keep constructor and vector helper boundaries. DC's bb
# declaration and the retail-supported named back() results are untouched.
def widgets(name, names):
    edits = []
    for local in names:
        pattern = r'(?m)^( +)(\w+)\* ' + local + r' = new ([\s\S]*?);\n\1m_widgets.push_back\(' + local + r'\);'
        matches = list(re.finditer(pattern, source))
        if len(matches) != 1:
            raise ValueError(f'{local}: expected unique declaration/append pair')
        m = matches[0]
        edits.append((m.group(), m[1] + 'widget* ' + local + ' = new ' + m[3] + ';\n' + m[1] + 'm_widgets.push_back(' + local + ');',
                      m[1] + 'm_widgets.push_back(new ' + m[3] + ');'))
    options = [{'name': 'derived-local'}]
    for index, variant in ((1, 'base-local'), (2, 'constructor-expression')):
        options.append({'name': variant, 'replace': edits[0][index],
                        'extra_edits': [{'find': e[0], 'replace': e[index]} for e in edits[1:]]})
    axes.append({'name': name, 'find': edits[0][0], 'options': options})

widgets('portrait-binding', ['portrait'])
widgets('main-label-bindings', ['titleText', 'heroText', 'gainedText', 'gainedIcon'])
widgets('paired-choice-bindings', ['choiceText', 'orText', 'leftBorder', 'rightBorder', 'leftIcon', 'rightIcon', 'leftLabel', 'rightLabel'])
widgets('single-choice-bindings', ['soleText', 'soleBorder', 'soleIcon', 'soleLabel'])
# Named format values versus their canonical operator[] call in sprintf.
formats = []
for local in ('heroFormat', 'choiceFormat', 'singleChoiceFormat'):
    m = re.search(r'(?m)^( +)const char\* ' + local + r' =\n\s+([^;]+);\n\1sprintf\(g_text, ' + local + ',', source)
    if not m:
        raise ValueError(local)
    formats.append((m.group(), m[1] + 'sprintf(g_text, ' + m[2] + ','))
axes.append({'name': 'format-result-lifetime', 'find': formats[0][0], 'options': [
    {'name': 'named-result'}, {'name': 'argument-result', 'replace': formats[0][1],
     'extra_edits': [{'find': a, 'replace': b} for a, b in formats[1:]]}]})
loop = '''    widget** first = m_widgets.begin();
    if (first != m_widgets.end()) {
        for (widget** it = first; it != m_widgets.end(); ++it) {
            if (*it)
                addWidget(*it, -1);
        }
    }'''
axes.append({'name': 'attachment-iterator-lifetime', 'find': loop, 'options': [
    {'name': 'separate-first'},
    {'name': 'for-iterator', 'replace': '''    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }'''},
    {'name': 'guarded-do-iterator', 'replace': '''    widget** it = m_widgets.begin();
    if (it != m_widgets.end()) {
        do {
            if (*it)
                addWidget(*it, -1);
            ++it;
        } while (it != m_widgets.end());
    }'''}]})
payload = {'schema': 1, 'source': 'src/levelupwindow.cpp', 'units': ['levelupwindow'],
           'evidence': ['DC 0xe8344: bb local and canonical set_visible, set_hotkey, operator[] calls retained.',
                        'DC lines 64/73/76/78/86..138 group widget construction and append; named locals remain a lower-bound inventory.',
                        'DC 140..144: begin/end traversal and AddWidget; retail EH/widget temporary homes and reserve size() decision differ.',
                        'Vary genuine expression/result/iterator lifetimes; no diagnostic padding, copied helper body, or inline pin.'],
           'axes': axes}
Path(sys.argv[1]).write_text(json.dumps(payload, indent=2) + '\n')
