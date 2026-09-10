"""Refine visited-row lifetimes after the reproduced integral-offset repair.

Retail Draw/Spell store the initial row home before the loop guard; the
candidate delays it past the guard. Keep the DC decoders/helper calls and
test direct pixel-cursor binding, counter lifetime and for-header updates.
"""
import json
from pathlib import Path
import re
import sys

source = Path('src/cspriteframe.cpp').read_text()
names = ['draw', 'drawCreatureImpl', 'drawAdvObjImpl', 'drawAdvObjWithFlagAlpha',
         'drawAdvObjShadowImpl', 'drawTile', 'drawTileShadow', 'drawSpellEffect']
loop_re = re.compile(r'for \(int y = sy; y < sy \+ sh; \+\+y\) \{')


def closing(text, start):
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return end


def direct(original):
    assignment = re.compile(r'(?P<indent>^[ ]*)(?P<var>lineDst|dst) = '
        r'(?P<value>static_cast<unsigned short\*>\(static_cast<void\*>\(rowBase [+-] rowOffset\)\));\n', re.M)
    edits = []
    for match in assignment.finditer(original):
        target = 'unsigned short* out = ' + match['var'] + ';'
        out = original.index(target, match.end())
        edits += [(match.start(), match.end(), ''),
                  (out, out+len(target), 'unsigned short* out = ' + match['value'] + ';')]
    assert len(edits) in [4, 8, 16]
    for start, end, replacement in sorted(edits, reverse=True):
        original = original[:start]+replacement+original[end:]
    return original


def loop_form(original, early):
    edits = []
    for match in loop_re.finditer(original):
        start = original.index('{', match.start())
        end = closing(original, start)
        if early:
            base = original.rfind('unsigned char* rowBase =', 0, match.start())
            begin = original.rfind('\n', 0, base)+1
            indent = original[begin:base]
            edits += [(begin, begin, indent+'int y = sy;\n'),
                      (match.start(), start, 'for (; y < sy + sh; ++y) ')]
        else:
            contents = original[start+1:end-1]
            contents, count = re.subn(r'^[ ]*rowOffset \+= dpitch;\n', '', contents, flags=re.M)
            assert count == 1
            edits += [(match.start(), end,
                'for (int y = sy; y < sy + sh; rowOffset += dpitch, ++y) {' + contents + '}')]
    assert edits
    for start, end, replacement in sorted(edits, reverse=True):
        original = original[:start]+replacement+original[end:]
    return original


axes = []
for name in names:
    start = source.index('void CSpriteFrame::' + name + '(', source.index('VA(0x0047c570'))
    original = source[start:source.index('\n}', start)+2]
    axes.append({'name':name,'find':original,'options':[
        {'name':'adopted-offset-control'},
        {'name':'direct-pixel-cursor','replace':direct(original)},
        {'name':'for-header-offset','replace':loop_form(original,False)},
        {'name':'early-row-counter','replace':loop_form(original,True)},
        {'name':'direct-cursor-early-counter','replace':loop_form(direct(original),True)}]})
Path(sys.argv[1]).write_text(json.dumps({'schema':1,'source':'src/cspriteframe.cpp',
    'units':['cspriteframe'],'axes':axes},indent=2)+'\n')
print('Wrote 390625 visited-row lifetime forms')
