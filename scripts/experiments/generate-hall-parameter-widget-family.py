#!/usr/bin/env python3
"""Hall result boundaries with the DC town-parameter indexing restored."""
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
    p = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/townmgr.cpp'
    if p.is_file() and function(p.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('Hall body does not equal a reproduced parent')
assert body.count('hallX[which][i]') == 36
assert body.count('hallY[which][i]') == 36
groups = {}
for m in re.finditer(r'(?m)^            (iconWidget|textWidget)\* (nameBar|buildingName|buildingImage|builtCheck) = new \1\(([\s\S]*?)\);\n            m_widgets.push_back\(\2\);', body):
    typ, name, ctor_args = m.groups()
    groups.setdefault(name, []).append((m.group(), {
        'constructor-expression': '            m_widgets.push_back(new ' + typ + '(' + ctor_args + '));',
        'base-result': m.group().replace(typ + '* ' + name, 'widget* ' + name, 1),
    }))
if set(groups) != {'nameBar', 'buildingName', 'buildingImage', 'builtCheck'} or any(len(g) != 9 for g in groups.values()):
    raise ValueError('expected four genuine result roles in each town loop')
tail_start = body.index('    m_widgets.push_back(new bitmapBorder(3, 555, 741, 18, 501,')
tail_end = body.index('    button* exitButton', tail_start)
tail = body[tail_start:tail_end]
matches = list(re.finditer(r'    m_widgets.push_back\(new (bitmapBorder|textWidget)\(([\s\S]*?)\)\);', tail))
if len(matches) != 3:
    raise ValueError('expected three status/title constructions')
named = tail
for m, name in zip(matches, ('statusBar', 'statusLabel', 'titleLabel')):
    named = named.replace(m.group(), '    ' + m[1] + '* ' + name + ' = new ' + m[1] + '(' + m[2] + ');\n    m_widgets.push_back(' + name + ');')
options = []
for choices in itertools.product(range(3), range(3), range(3), range(3), range(2)):
    candidate = body
    for choice, edits in zip(choices, groups.values()):
        if choice:
            form = ('constructor-expression', 'base-result')[choice - 1]
            for old, forms in edits:
                candidate = candidate.replace(old, forms[form])
    if choices[-1]:
        candidate = candidate.replace(tail, named)
    row = {'name': '-'.join(map(str, choices))}
    if any(choices):
        row['replace'] = candidate
    options.append(row)
axes = [{'name': 'parameter-indexed-result-boundaries', 'find': body, 'options': options}]
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/townmgr.cpp', 'units': ['townmgr'], 'axes': axes,
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'DC positively indexes both hall tables using which, including multiplication of retained parameter R9 by72 at4344 and peer groups. Restoring both changes the inlining phase to92.5610; either table alone is byte-flat at99.6605, so do not infer that the dynamic parameter itself is incompatible with retail.',
        'Retest the four real result boundaries from this corrected parent: DC4344..4347 constructs/appends each widget in one source group, and the retail cases select different expansion depths of the same canonical vector API.',
        'The earlier result families used already-specialized row constants. These162 states preserve parameter indexing and explore loop-derived results, constructor expressions, base results and the actual status/title result lifetimes.',
        'No synthetic mass, scope-padding statements, inline pins, or changed widget order/arguments. All79 exact townmgr siblings are scored.',
    ],
}, indent=2) + '\n')
