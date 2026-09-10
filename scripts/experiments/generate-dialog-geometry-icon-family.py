#!/usr/bin/env python3
"""Probe real max operands/receiver lifetimes and experience text assignments."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('function', choices=('geometry', 'icon'))
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()
axes = []
if args.function == 'geometry':
    for name, first, second in (
        ('sprite-maximum', 'dialogInfo.m_icons[i].m_spriteWidth', 'widestIcon'),
        ('caption-maximum', 'dialogInfo.m_icons[i].m_textWidth', 'widestIcon'),
        ('caption-overhang', 'spacing', 'dialogInfo.m_icons[i].m_textWidth\n                                      - dialogInfo.m_icons[i].m_spriteWidth'),
    ):
        import re
        pattern = r'std::_cpp_max\(' + re.escape(first) + r',\s*' + re.escape(second) + r'\)'
        match = re.search(pattern, source)
        if not match:
            raise ValueError('stale maximum anchor: ' + name)
        axes.append({'name': name, 'find': match.group(), 'options': [
            {'name': 'control'}, {'name': 'commuted-value-operands', 'replace': 'std::_cpp_max(' + second + ', ' + first + ')'}]})
    start = source.index('    for (i = 0; i < 8; i++) {', source.index('void calculateNormalDialogSize('))
    end = source.index('\n    dialogInfo.m_width = 40;', start)
    anchor = source[start:end]
    options = [{'name': 'control'}]
    for kind, declaration, receiver in (
        ('reference', 'type_dialog_icon& icon = dialogInfo.m_icons[i];', 'icon.'),
        ('pointer', 'type_dialog_icon* icon = &dialogInfo.m_icons[i];', 'icon->'),
    ):
        body = anchor.replace('        if (dialogInfo.m_icons[i].m_resource', '        ' + declaration + '\n        if (dialogInfo.m_icons[i].m_resource', 1)
        # Only the body after the binding uses the new receiver.
        pos = body.index(declaration) + len(declaration)
        body = body[:pos] + body[pos:].replace('dialogInfo.m_icons[i].', receiver)
        options.append({'name': kind + '-receiver', 'replace': body})
    # These edits overlap, so compose one bounded whole-loop axis.
    import itertools
    maximum_axes = axes
    combined = []
    for choices in itertools.product(range(2), range(2), range(2), range(3)):
        body = options[choices[3]].get('replace', anchor)
        receiver = ('dialogInfo.m_icons[i].', 'icon.', 'icon->')[choices[3]]
        for axis, choice in zip(maximum_axes, choices):
            if choice:
                find = axis['find'].replace('dialogInfo.m_icons[i].', receiver)
                replacement = axis['options'][choice]['replace'].replace('dialogInfo.m_icons[i].', receiver)
                if body.count(find) != 1:
                    raise ValueError('ambiguous composed maximum')
                body = body.replace(find, replacement)
        item = {'name': '-'.join(map(str, choices))}
        if any(choices):
            item['replace'] = body
        combined.append(item)
    axes = [{'name': 'icon-grid-values-and-receiver', 'find': anchor, 'options': combined}]
    evidence = [
        'DC 5219/5220 row initialization, long spacing/widest_icon, three std::max<long> const-reference helpers, and width_changed do-loop are preserved.',
        'Retail 0x4f5d80 has a remaining register/live-range disagreement after those source facts are restored.',
        '24 finite choices commute pure maximum values or bind the actual icon receiver; no false inline or artificial operation.',
    ]
else:
    for name, anchor, rhs in (
        ('experience-label', '            m_text = (*g_generalText)[443];', '(*g_generalText)[443]'),
        ('experience-empty', '        } else if (m_qualifier == 0) {\n            m_text = DATA_COMPGEN(0x00691210, dialogEmptyText, "");\n        } else {\n            m_text = formatString(', 'DATA_COMPGEN(0x00691210, dialogEmptyText, "")'),
    ):
        assignment = '            m_text = ' + rhs + ';'
        options = [{'name': 'control'}]
        for label, replacement in (
            ('named-string-reference', '            std::string& text = m_text;\n            text = ' + rhs + ';'),
            ('scoped-text-pointer', '            const char* text = ' + rhs + ';\n            m_text = text;'),
        ):
            options.append({'name': label, 'replace': anchor.replace(assignment, replacement)})
        axes.append({'name': name + '-result', 'find': anchor, 'options': options})
    evidence = [
        'DC and retail experience arms assign char-pointer results through the canonical string operator=.',
        'Retail retains two nested _Eos calls where VC6 currently expands them; vary genuine result/receiver bindings without changing helper boundaries.',
        'Nine finite source combinations. No dummy code or inline-depth pin.',
    ]
for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('stale or ambiguous anchor: ' + axis['name'])
args.output.write_text(json.dumps({'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'], 'axes': axes, 'evidence': evidence}, indent=2) + '\n')
