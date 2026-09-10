#!/usr/bin/env python3
"""Constructor result/scope families preserving DC helpers and retail layouts."""
import argparse
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('window', choices=('hall', 'screen'))
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/townmgr.cpp').read_text()
name = 'THallWindow' if args.window == 'hall' else 'TTownScreenWindow'
start = source.index(name + '::' + name + '(')
end = source.index('\n}\n', start) + 2
body = source[start:end]
axes = []
groups = {}
for m in re.finditer(r'(?m)^( +)m_widgets.push_back\(new (\w+)\(([\s\S]*?)\)\);', body):
    indent, typ, ctor_args = m.groups()
    if args.window == 'hall':
        group = 'backgrounds' if typ == 'bitmapBorder' else 'title' if 'bigfont.fnt' in ctor_args else 'building-widgets'
    else:
        group = 'backgrounds' if typ in ('border', 'bitmapBorder') else 'buttons' if typ == 'button' else 'labels' if 'TextWidget' in typ or typ == 'textWidget' else 'icons'
    local = {'border': 'frame', 'bitmapBorder': 'background', 'textWidget': 'label',
             'bitmapBackedTextWidget': 'label', 'button': 'control', 'iconWidget': 'icon'}[typ]
    alternatives = [m.group()]
    for binding in (typ, 'widget'):
        alternatives.append(indent + '{\n' + indent + '    ' + binding + '* ' + local + ' = new ' + typ + '(' + ctor_args + ');\n' +
                            indent + '    m_widgets.push_back(' + local + ');\n' + indent + '}')
    if source.count(alternatives[0]) != 1:
        prior_line = body.rfind('\n', 0, max(0, m.start() - 1)) + 1
        prefix = body[prior_line:m.start()]
        alternatives = [prefix + variant for variant in alternatives]
    groups.setdefault(group, []).append(alternatives)
for group, edits in groups.items():
    options = [{'name': 'constructor-expression'}]
    for i, label in ((1, 'derived-result-scope'), (2, 'base-result-scope')):
        options.append({'name': label, 'replace': edits[0][i], 'extra_edits': [
            {'find': e[0], 'replace': e[i]} for e in edits[1:]]})
    axes.append({'name': name + '-' + group, 'find': edits[0][0], 'options': options})
loop = '''    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }'''
# The same traversal occurs elsewhere; use its immediately following function
# boundary to keep the authored anchor unique without absorbing widget edits.
if args.window == 'hall':
    loop += '\n}\n\n\n// The sixteen bytes'
    ending = '\n}\n\n\n// The sixteen bytes'
else:
    loop += '\n}\n\nVA_COMPGEN(0x005c58b0'
    ending = '\n}\n\nVA_COMPGEN(0x005c58b0'
variants = [
    '''    widget** it = m_widgets.begin();
    while (it != m_widgets.end()) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
        ++it;
    }''',
    '''    widget** it = m_widgets.begin();
    if (it != m_widgets.end()) {
        do {
            if (*it)
                addWidget(*it, -1);
            else
                memError();
            ++it;
        } while (it != m_widgets.end());
    }''']
axes.append({'name': name + '-attachment-scope', 'find': loop, 'options': [
    {'name': 'for-iterator'}, *[{'name': label, 'replace': variant + ending}
                              for label, variant in zip(('while-iterator', 'guarded-do-iterator'), variants)]]})
if args.window == 'screen':
    buffer = '''    m_zBuffer = new unsigned short[m_height * m_width];
    memset(m_zBuffer, 0, m_height * m_width * 2);'''
    axes.append({'name': 'buffer-extent-result', 'find': buffer, 'options': [
        {'name': 'direct-extent'}, {'name': 'named-extent', 'replace': '''    int bufferPixels = m_height * m_width;
    m_zBuffer = new unsigned short[bufferPixels];
    memset(m_zBuffer, 0, bufferPixels * 2);'''}]})
payload = {'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'], 'axes': axes,
           'evidence': ['DC ' + ('0x16e6cc' if args.window == 'hall' else '0x16a72c') + ': construction/append boundaries and begin/end/AddWidget/memError traversal.',
                        'Retail reserve/helper expansion and widget allocation/EH homes differ after removal of unattested diagnostic padding.',
                        'Vary actual constructor-result and iterator lifetimes. Keep array constness/extents, member ownership, canonical helpers and side-effect order.']}
args.output.write_text(json.dumps(payload, indent=2) + '\n')
print(name, {g: len(e) for g, e in groups.items()})
