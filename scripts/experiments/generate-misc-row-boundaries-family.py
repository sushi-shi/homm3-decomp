"""Finite boundary repairs for puzzle sampling, fizzle, and Victor rows.

DC proves puzzle's nested 32-pixel sampling and fizzle's bitmap/helper scopes.
Victor's DC functions are API-only stubs, so their five-mode PCX decoder and
paired flip-row lifetime come from retail. Unchecked options are controls.
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
original = source[start:source.index('\n}',start)+2]
options = [{'name': 'unchecked-control'}]
if args.unit == 'puzzlewindow':
    column = '            sourceBlock += 32;'
    rows = '        destination += 19;\n        source += m_pitch * 32;'
    last = original.replace(column, '            if (x + 32 < width)\n    '+column).replace(rows,
        '        if (y + 32 < height) {\n'+'\n'.join('    '+line for line in rows.splitlines())+'\n        }')
    first = original.replace(column+'\n','').replace(rows+'\n','').replace(
        'for (int y = 0; y < height; y += 32) {',
        'for (int y = 0; y < height; y += 32) {\n        if (y) {\n'
        +'\n'.join('    '+line for line in rows.splitlines())+'\n        }').replace(
        'for (int x = 0; x < width; x += 32) {',
        'for (int x = 0; x < width; x += 32) {\n            if (x)\n    '+column)
    indexed = original.replace('            if (*sourceBlock)', '            if (sourceBlock[x])').replace(
        column+'\n','').replace(rows+'\n','').replace(
        'unsigned char* sourceBlock = source;', 'unsigned char* sourceBlock = source + y * m_pitch;').replace(
        'unsigned char* destinationBlock = destination;', 'unsigned char* destinationBlock = destination + (y / 32) * 19;')
    options += [{'name': name,'replace': body} for name,body in [
        ('last-sample-guards',last),('next-sample-guards',first),('visited-sample-offsets',indexed)]]
elif args.unit == 'winmgr':
    step = '                    screen.m_bytes += m_screenBitmap->getPitch();'
    last = original.replace(step, '                    if (row + 1 < height)\n    '+step)
    first = original.replace(step+'\n','').replace('for (int row = 0; row < height; row++) {',
        'for (int row = 0; row < height; row++) {\n                    if (row)\n    '+step)
    origin = original.replace('m_screenBitmap->getMap(startX, startY)', 'm_screenBitmap->getMap(0, startY)').replace(
        'unsigned short* d = screen.m_pixels;', 'unsigned short* d = screen.m_pixels + startX;')
    options += [{'name': name,'replace': body} for name,body in [
        ('last-screen-row-guard',last),('next-screen-row-guard',first),('screen-allocation-row',origin)]]
elif args.unit == 'victor_flip':
    step = ('                        sourceTop -= source->m_buffwidth;\n'
        '                        destinationTop -= destination->m_buffwidth;\n'
        '                        sourceBottom += source->m_buffwidth;\n'
        '                        destinationBottom += destination->m_buffwidth;')
    assert original.count(step) == 2
    guarded = original.replace(step, '                        if (rows) {\n'
        +'\n'.join('    '+line for line in step.splitlines())+'\n                        }')
    stopped = original.replace(step, '                        if (!rows)\n                            break;\n'+step)
    options += [{'name': 'remaining-pair-guard', 'replace': guarded},
                {'name': 'break-after-last-pair', 'replace': stopped}]
else:
    step = '                destination -= image->m_buffwidth;\n                --rowsRemaining;'
    guarded = original.replace(step, '                if (rowsRemaining > 1)\n'
        '                    destination -= image->m_buffwidth;\n                --rowsRemaining;')
    decremented = original.replace(step, '                if (--rowsRemaining)\n'
        '                    destination -= image->m_buffwidth;')
    offset = original.replace('                destination -= image->m_buffwidth;\n','').replace(
        'while (rowsRemaining) {', 'while (rowsRemaining) {\n'
        '                destination = image->m_ibuff + offset\n'
        '                    - (height - rowsRemaining) * image->m_buffwidth;')
    options += [{'name': name,'replace': body} for name,body in [
        ('last-decoded-row-guard',guarded),('decrement-before-row-step',decremented),('visited-row-offset',offset)]]
args.output.write_text(json.dumps({'schema':1,'source':path,'units':[args.unit],
    'axes':[{'name':'row-boundary','find':original,'options':options}]},indent=2)+'\n')
print('Wrote',len(options),'boundary forms for',args.unit)
