"""Follow-up: integral row displacements, materialized only on visited rows.

Keep the DC pixel loops, locals and canonical GetMap/GetPitch calls. Unlike
an integerized address, each displacement is relative to a real bitmap row
pointer and is never dereferenced past the last visit. Compare tail/header
updates and a before-first integral index with the adopted safe control.
"""
import argparse
import json
import math
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument('unit', choices=['bitmap16', 'bitmap24'])
parser.add_argument('output', type=Path)
args = parser.parse_args()
path = 'src/' + args.unit + '.cpp'
source = Path(path).read_text()


def closing(text, start):
    depth = 1
    end = start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return end


if args.unit == 'bitmap16':
    specs = [
        ('draw', 'void Bitmap16Bit::draw(', 'row', 'srcHeight',
         [('source.m_bytes', 'm_pitch', True), ('target.m_bytes', 'dstPitch', False)]),
        ('grab', 'void Bitmap16Bit::grab(', 'row', 'h',
         [('dst.m_bytes', 'm_pitch', False), ('source.m_bytes', 'srcPitch', True)]),
        ('fill', 'void Bitmap16Bit::fillRect(', 'row', 'h', [('dst.m_bytes', 'm_pitch', False)]),
        ('frame', 'void Bitmap16Bit::frameRect(', 'row', 'h', [('dst.m_bytes', 'm_pitch', False)]),
        ('darken', 'void Bitmap16Bit::darken(int x, int y, int w, int h)\n', 'iy', 'h',
         [('row.m_bytes', 'm_pitch', False)]),
        ('mask', 'void Bitmap16Bit::darken(int x, int y, int w, int h, Bitmap816*', 'iy', 'h',
         [('maskRow', 'mask->getPitch()', False), ('row.m_bytes', 'm_pitch', False)]),
        ('colorize', 'void Bitmap16Bit::colorize(int x, int y, int w, int h, float', 'iy', 'h',
         [('row.m_bytes', 'm_pitch', False)]),
    ]
else:
    specs = [
        ('draw', 'void Bitmap24Bit::draw(int sx, int sy, int sw, int sh, unsigned short*', 'y', 'sh',
         [('dst', 'dpitch', False), ('src', 'getPitch()', False)]),
        ('adjust-hsv', 'void Bitmap24Bit::adjustHSV(int x, int y, int w, int h, float hue,\n', 'row', 'h',
         [('src', 'getPitch()', False)]),
    ]


def variant(original, name, counter, height, cursors, choice):
    if args.unit == 'bitmap16' and name == 'draw':
        # Undo only the horizontal-origin representation; integral row
        # displacement makes the nonzero-origin endpoint safe again.
        original = original.replace('getMap(0, srcY)', 'getMap(srcX, srcY)').replace(
            'target.m_bytes += dstY * dstPitch;',
            'target.m_bytes += dstY * dstPitch + dstX * sizeof(unsigned short);').replace(
            'source.m_pixels[srcX + col]', 'source.m_pixels[col]').replace(
            'target.m_pixels[dstX + col]', 'target.m_pixels[col]').replace(
            'memcpy(target.m_pixels + dstX, source.m_pixels + srcX,',
            'memcpy(target.m_pixels, source.m_pixels,')
    loop = f'for (int {counter} = 0; {counter} < {height}; ++{counter}) {{'
    matches = list(re.finditer(re.escape(loop), original))
    assert len(matches) == (2 if name == 'draw' and args.unit == 'bitmap16' else 1)
    declarations, bindings, updates = [], [], []
    for cursor, pitch, const in cursors:
        stem = cursor.split('.')[0]
        base = stem + 'RowBase'
        offset = stem + 'RowOffset'
        pointer = cursor if cursor != 'dst' else (
            'static_cast<unsigned char*>(static_cast<void*>(dst))')
        declarations += [f'{"const " if const else ""}unsigned char* {base} = {pointer};',
                         f'int {offset} = ' + (f'-({pitch})' if choice == 3 else '0') + ';']
        binding = f'{base} + {offset}'
        if cursor == 'dst':
            binding = 'static_cast<unsigned short*>(static_cast<void*>(' + binding + '))'
        bindings.append(f'{cursor} = {binding};')
        updates.append(f'{offset} += {pitch};')
    edits = []
    for match in matches:
        start = original.index('{', match.start())
        end = closing(original, start)
        contents = original[start+1:end-1]
        indent = original[original.rfind('\n', 0, match.start())+1:match.start()] + '    '
        # Remove the adopted row-only guard (never the pixel/outline guards).
        guard = re.search(r'^[ \t]*if \(' + re.escape(counter)
            + r'(?: \+ 1 < ' + re.escape(height) + r')?\)\s*', contents, re.M)
        if guard:
            statement = guard.end()
            stop = closing(contents, statement) if contents[statement] == '{' else contents.index(';', statement)+1
            guarded = contents[guard.start():stop]
            assert all(cursor in guarded for cursor, _, _ in cursors)
            contents = contents[:guard.start()] + contents[stop:]
        for cursor, pitch, _ in cursors:
            contents = re.sub(r'^[ \t]*' + re.escape(cursor) + r' \+= ' + re.escape(pitch) + r';\n', '', contents, flags=re.M)
        contents = contents.strip('\n')
        head = (updates if choice == 3 else []) + bindings
        contents = '\n' + '\n'.join(indent + line for line in head) + '\n' + contents
        if choice == 1:
            contents += '\n' + '\n'.join(indent + line for line in updates)
        contents += '\n' + indent[:-4]
        header = original[match.start():start+1]
        if choice == 2:
            increment = ', '.join(line[:-1] for line in updates) + ', ++' + counter
            header = header.replace('++' + counter, increment)
        edits.append((match.start(), end, header + contents + '}'))
    for start, end, text in reversed(edits):
        original = original[:start] + text + original[end:]
    anchor = '        if (flipped) {' if args.unit == 'bitmap16' and name == 'draw' else original[original.rfind('\n', 0, original.index(loop.split(';')[0]))+1:original.index(loop.split(';')[0])]
    if args.unit == 'bitmap16' and name == 'draw':
        insertion = original.index(anchor)
        indent = '        '
    else:
        insertion = original.index('for (int ' + counter + ' = 0;')
        insertion = original.rfind('\n', 0, insertion) + 1
        indent = re.match(r'[ \t]*', original[insertion:]).group()
    return original[:insertion] + ''.join(indent + line + '\n' for line in declarations) + original[insertion:]


axes = []
for name, signature, counter, height, cursors in specs:
    start = source.index(signature)
    original = source[start:source.index('\n}', start)+2]
    axes.append({'name': name, 'find': original, 'options': [
        {'name': 'adopted-safe-control'},
        *[{'name': label, 'replace': variant(original, name, counter, height, cursors, choice)}
          for choice, label in [(1, 'relative-offset-tail'), (2, 'relative-offset-header'),
                                (3, 'relative-offset-preincrement')]]
    ]})
args.output.write_text(json.dumps({'schema': 1, 'source': path, 'units': [args.unit],
    'axes': axes}, indent=2) + '\n')
print('Wrote', math.prod(len(a['options']) for a in axes), 'relative-offset forms')
