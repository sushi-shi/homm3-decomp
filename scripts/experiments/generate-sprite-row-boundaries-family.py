"""Guard sprite row cursors, keeping each DC decoder/flip/helper boundary.

DC cspriteframe.cpp Draw 1401/1469, Creature 1916/2068, Adv 2272/2351,
alpha 2472/2550, shadow 2673/2756, Tile 2900..3357, TileShadow
3365..3772, Spell 3817/3876: row loops surround complete run decoders.
Tile's raw Duff loops additionally advance a source row and decrement sh.
Only the unused post-final pointer formation is deliberately changed.
"""
import argparse
import json
import math
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument('output', type=Path)
parser.add_argument('--selection', action='append', default=[])
args = parser.parse_args()
source = Path('src/cspriteframe.cpp').read_text()
names = ['draw', 'drawCreatureImpl', 'drawAdvObjImpl', 'drawAdvObjWithFlagAlpha',
         'drawAdvObjShadowImpl', 'drawTile', 'drawTileShadow', 'drawSpellEffect']
step_re = re.compile(
    r'(?P<indent>^[ \t]*)(?P<var>lineDst|dst) =\s*'
    r'static_cast<unsigned short\*>\(static_cast<void\*>\(\s*'
    r'static_cast<unsigned char\*>\(\s*static_cast<void\*>\((?P=var)\)\)\s*'
    r'(?P<sign>[+-])\s*dpitch\)\);', re.M)
loop_re = re.compile(r'for \(int y = sy; y < sy \+ sh; \+\+y\) \{'
                     r'|do \{\n[ ]+int remaining = sw;')


def guarded(step, condition):
    indent = step[:len(step)-len(step.lstrip())]
    return indent + 'if (' + condition + ') {\n' + '\n'.join(
        '    ' + line for line in step.splitlines()) + '\n' + indent + '}'


def variant(original, choice):
    edits = []
    for match in loop_re.finditer(original):
        start = original.index('{', match.start())
        depth = 1
        end = start + 1
        while depth:
            depth += (original[end] == '{') - (original[end] == '}')
            end += 1
        loop = original[start+1:end-1]
        matches = list(step_re.finditer(loop))
        assert len(matches) == 1
        step_match = matches[0]
        step = step_match.group()
        raw = match.group().startswith('do')
        if raw:
            indent = step_match['indent']
            step = indent + 'line += m_pitch;\n' + step
            assert step in loop
        if choice == 1 or raw:
            loop = loop.replace(step, guarded(step, 'sh > 1' if raw else 'y + 1 < sy + sh'))
        elif choice == 2:
            loop = '\n' + guarded(step, 'y != sy') + loop.replace('\n' + step, '')
        elif choice == 3:
            indent = step_match['indent']
            loop = loop.replace(step, indent + 'if (y + 1 == sy + sh)\n'
                                + indent + '    break;\n' + step)
        edits.append((start+1, end-1, loop))
    assert len(edits) == (8 if 'void CSpriteFrame::drawTile(' in original else
                          4 if 'void CSpriteFrame::drawTileShadow(' in original else 2)
    for start, end, replacement in reversed(edits):
        original = original[:start] + replacement + original[end:]
    return original


axes = []
for name in names:
    start = source.index('void CSpriteFrame::' + name + '(', source.index('VA(0x0047c570'))
    original = source[start:source.index('\n}', start)+2]
    axes.append({'name': name, 'find': original, 'options': [
        {'name': 'unchecked-control'},
        {'name': 'last-row-boundary', 'replace': variant(original, 1)},
        {'name': 'next-row-boundary', 'replace': variant(original, 2)},
        {'name': 'break-before-final-step', 'replace': variant(original, 3)},
    ]})
if args.selection:
    options = [{'name': 'unchanged-control'}]
    for selection in args.selection:
        choices = [int(value) for value in selection.split(',')]
        assert len(choices) == len(axes)
        result = source
        for axis, choice in zip(axes, choices):
            result = result.replace(axis['find'], axis['options'][choice].get('replace', axis['find']))
        options.append({'name': 'recombined-' + selection, 'replace': result})
    axes = [{'name': 'reproduced-row-parents', 'find': source, 'options': options}]
args.output.write_text(json.dumps({'schema': 1, 'source': 'src/cspriteframe.cpp',
    'units': ['cspriteframe'], 'axes': axes}, indent=2) + '\n')
print('Wrote', math.prod(len(axis['options']) for axis in axes), 'sprite row combinations')
