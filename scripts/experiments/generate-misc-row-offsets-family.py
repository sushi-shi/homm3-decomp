"""Integral row displacements for the remaining repaired image traversals.

Retain puzzle's 32-pixel samples, fizzle's canonical bitmap boundaries,
Victor's paired-copy scopes and PCX's completed-plane row transition.
Integer displacements are relative to real pointers, never integer addresses.
"""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('unit', choices=['puzzlewindow', 'winmgr', 'victor_flip', 'victor_loadpcx'])
parser.add_argument('output', type=Path)
args = parser.parse_args()
path = 'src/' + args.unit + '.cpp'
source = Path(path).read_text()
signatures = {'puzzlewindow': 'void Bitmap816::markPuzzle(',
    'winmgr': 'void heroWindowManager::fizzleForwardX(',
    'victor_flip': 'int __stdcall flipimage(', 'victor_loadpcx': 'int __stdcall loadpcx('}
start = source.index(signatures[args.unit])
original = source[start:source.index('\n}', start)+2]
options = [{'name': 'adopted-safe-control'}]
if args.unit == 'puzzlewindow':
    inner = '            if (x + 32 < width)\n                sourceBlock += 32;\n'
    outer = '        if (y + 32 < height) {\n            destination += 19;\n            source += m_pitch * 32;\n        }'
    assert inner in original and outer in original
    for indexed in [False, True]:
        body = original.replace(inner, '').replace('if (*sourceBlock)', 'if (sourceBlock[x])')
        declarations = '    unsigned char* sourceRowBase = source;\n    unsigned char* destinationRowBase = destination;\n'
        if indexed:
            head = '        source = sourceRowBase + y * m_pitch;\n        destination = destinationRowBase + (y / 32) * 19;\n'
            tail = ''
        else:
            declarations += '    int sourceRowOffset = 0;\n    int destinationRowOffset = 0;\n'
            head = '        source = sourceRowBase + sourceRowOffset;\n        destination = destinationRowBase + destinationRowOffset;\n'
            tail = '        destinationRowOffset += 19;\n        sourceRowOffset += m_pitch * 32;'
        loop = '    for (int y = 0; y < height; y += 32) {\n'
        body = body.replace(loop, declarations + loop + head).replace(outer, tail)
        options.append({'name': 'row-index' if indexed else 'row-displacement', 'replace': body})
elif args.unit == 'winmgr':
    guard = '                    if (row + 1 < height)\n                        screen.m_bytes += m_screenBitmap->getPitch();'
    loop = '                for (int row = 0; row < height; row++) {\n'
    assert guard in original and loop in original
    for indexed in [False, True]:
        declarations = '                unsigned char* screenRowBase = screen.m_bytes;\n'
        if indexed:
            offset = 'row * m_screenBitmap->getPitch()'
            tail = ''
        else:
            declarations += '                int screenRowOffset = 0;\n'
            offset = 'screenRowOffset'
            tail = '                    screenRowOffset += m_screenBitmap->getPitch();'
        body = original.replace(loop, declarations + loop + '                    screen.m_bytes = screenRowBase + ' + offset + ';\n').replace(guard, tail)
        options.append({'name': 'row-index' if indexed else 'row-displacement', 'replace': body})
elif args.unit == 'victor_flip':
    guard = '                        if (!rows)\n                            break;\n'
    steps = ''.join('                        ' + var + ' ' + sign + '= ' + owner + '->m_buffwidth;\n'
        for var,sign,owner in [('sourceTop','-','source'),('destinationTop','-','destination'),
                              ('sourceBottom','+','source'),('destinationBottom','+','destination')])
    assert original.count(guard + steps) == 2
    for indexed in [False, True]:
        declarations = ''.join('                unsigned char* ' + var + 'RowBase = ' + var + ';\n'
            for var in ['sourceTop','destinationTop','sourceBottom','destinationBottom'])
        if not indexed:
            declarations += '                unsigned int sourceRowOffset = 0;\n                unsigned int destinationRowOffset = 0;\n'
        body = original.replace('                if (depth >= victorIndexedColor) {', declarations + '                if (depth >= victorIndexedColor) {')
        head = ''
        for var,sign,owner in [('sourceTop','-','source'),('destinationTop','-','destination'),
                              ('sourceBottom','+','source'),('destinationBottom','+','destination')]:
            offset = '(((height + 1) >> 1) - rows - 1) * ' + owner + '->m_buffwidth' if indexed else owner + 'RowOffset'
            head += '                        ' + var + ' = ' + var + 'RowBase ' + sign + ' ' + offset + ';\n'
        body = body.replace('                    while (rows--) {\n', '                    while (rows--) {\n' + head)
        tail = '' if indexed else '                        sourceRowOffset += source->m_buffwidth;\n                        destinationRowOffset += destination->m_buffwidth;\n'
        body = body.replace(guard + steps, tail)
        options.append({'name': 'paired-row-index' if indexed else 'paired-row-displacement', 'replace': body})
else:
    guard = '                if (rowsRemaining > 1)\n                    destination -= image->m_buffwidth;'
    loop = '            while (rowsRemaining) {\n'
    assert guard in original and loop in original
    for indexed in [False, True]:
        declarations = '            unsigned char* destinationRowBase = destination;\n'
        if indexed:
            offset = '(height - rowsRemaining) * image->m_buffwidth'
            tail = ''
        else:
            declarations += '            unsigned int destinationRowOffset = 0;\n'
            offset = 'destinationRowOffset'
            tail = '                destinationRowOffset += image->m_buffwidth;'
        body = original.replace(loop, declarations + loop + '                destination = destinationRowBase - ' + offset + ';\n').replace(guard, tail)
        options.append({'name': 'completed-row-index' if indexed else 'completed-row-displacement', 'replace': body})
args.output.write_text(json.dumps({'schema':1,'source':path,'units':[args.unit],
    'axes':[{'name':'visited-row-offsets','find':original,'options':options}]},indent=2)+'\n')
print('Wrote',len(options),'relative-offset forms for',args.unit)
