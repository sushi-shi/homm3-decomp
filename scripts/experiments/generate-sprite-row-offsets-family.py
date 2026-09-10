"""Preserve DC row/decoder scopes; form pointers only for visited rows.

Compare integral byte-displacement recurrences with row-index multiplication.
Reverse destination walks use negative displacements from the last real row;
the integer can advance after the final visit without forming a pointer.
"""
import json
import math
from pathlib import Path
import re
import sys

source = Path('src/cspriteframe.cpp').read_text()
names = ['draw', 'drawCreatureImpl', 'drawAdvObjImpl', 'drawAdvObjWithFlagAlpha',
         'drawAdvObjShadowImpl', 'drawTile', 'drawTileShadow', 'drawSpellEffect']
step_re = re.compile(r'(?P<indent>^[ \t]*)(?P<var>lineDst|dst) =\s*'
    r'static_cast<unsigned short\*>\(static_cast<void\*>\(\s*'
    r'static_cast<unsigned char\*>\(\s*static_cast<void\*>\((?P=var)\)\)\s*'
    r'(?P<sign>[+-])\s*dpitch\)\);', re.M)
loop_re = re.compile(r'for \(int y = sy; y < sy \+ sh; \+\+y\) \{'
                     r'|do \{\n[ ]+int remaining = sw;')


def closing(text, start):
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return end


def variant(original, indexed):
    edits = []
    for match in loop_re.finditer(original):
        start = original.index('{', match.start())
        end = closing(original, start)
        contents = original[start+1:end-1]
        steps = list(step_re.finditer(contents))
        assert len(steps) == 1
        step = steps[0]
        raw = match.group().startswith('do')
        guard = re.search(r'^[ ]*if \((?:y \+ 1 [<=]=? sy \+ sh|sh > 1)\)', contents, re.M)
        assert guard is not None
        if 'break;' in contents[guard.end():step.start()]:
            stop = step.end()
        else:
            stop = closing(contents, contents.index('{', guard.end()))
        contents = contents[:guard.start()] + contents[stop:]
        indent = step['indent']
        outer = indent[:-4]
        var = step['var']
        declarations = [f'unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>({var}));']
        if indexed:
            if raw:
                declarations += ['int rowIndex = 0;']
            offset = ('rowIndex' if raw else '(y - sy)') + ' * dpitch'
        else:
            declarations += ['int rowOffset = 0;']
            offset = 'rowOffset'
        binding = f'{var} = static_cast<unsigned short*>(static_cast<void*>(rowBase {step["sign"]} {offset}));'
        head, tail = [binding], []
        if raw:
            declarations += ['const unsigned char* sourceRowBase = line;']
            if not indexed:
                declarations += ['int sourceRowOffset = 0;']
            head += ['line = sourceRowBase + ' + ('rowIndex * m_pitch' if indexed else 'sourceRowOffset') + ';']
            tail += ['++rowIndex;'] if indexed else ['sourceRowOffset += m_pitch;']
        if not indexed:
            tail += ['rowOffset += dpitch;']
        contents = '\n' + '\n'.join(indent + line for line in head) + '\n' + contents.strip('\n')
        contents += '\n' + '\n'.join(indent + line for line in tail) + '\n' + outer
        begin = original.rfind('\n', 0, match.start()) + 1
        replacement = ''.join(outer + line + '\n' for line in declarations)
        replacement += original[begin:start+1] + contents + '}'
        edits.append((begin, end, replacement))
    assert len(edits) == (8 if 'void CSpriteFrame::drawTile(' in original else
                          4 if 'void CSpriteFrame::drawTileShadow(' in original else 2)
    for start, end, replacement in reversed(edits):
        original = original[:start] + replacement + original[end:]
    return '\n'.join(line.rstrip() for line in original.splitlines())


axes = []
for name in names:
    start = source.index('void CSpriteFrame::' + name + '(', source.index('VA(0x0047c570'))
    original = source[start:source.index('\n}', start)+2]
    axes.append({'name': name, 'find': original, 'options': [
        {'name': 'adopted-safe-control'},
        {'name': 'relative-byte-offset', 'replace': variant(original, False)},
        {'name': 'relative-row-index', 'replace': variant(original, True)}]})
Path(sys.argv[1]).write_text(json.dumps({'schema': 1, 'source': 'src/cspriteframe.cpp',
    'units': ['cspriteframe'], 'axes': axes}, indent=2) + '\n')
print('Wrote', math.prod(len(axis['options']) for axis in axes), 'sprite offset forms')
