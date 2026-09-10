"""Bound the two ends of bitmap row traversal without changing pixel work.

DC/retail evidence: Bitmap16 Draw 541..619, Grab 625..674, FrameRect
705..736, Darken 742..810, Colorize 873..930; Bitmap24 Draw 280..344 and
AdjustHSV 349..440. Source/destination/mask strides remain separate and the
canonical accessors stay calls. The unchecked form is only a control.
"""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('unit', choices=['bitmap16', 'bitmap24'])
parser.add_argument('output', type=Path)
parser.add_argument('--selection', action='append', default=[],
                    help='comma-separated choices to reproduce as whole-TU parents')
args = parser.parse_args()
path = 'src/' + args.unit + '.cpp'
source = Path(path).read_text()
if args.unit == 'bitmap16':
    specs = [
        ('draw', 'void Bitmap16Bit::draw(', 'row', 'srcHeight',
         '                source.m_bytes += m_pitch;\n                target.m_bytes += dstPitch;'),
        ('grab', 'void Bitmap16Bit::grab(', 'row', 'h',
         '            dst.m_bytes += m_pitch;\n            source.m_bytes += srcPitch;'),
        ('frame', 'void Bitmap16Bit::frameRect(', 'row', 'h',
         '            dst.m_bytes += m_pitch;'),
        ('darken', 'void Bitmap16Bit::darken(int x, int y, int w, int h)\n', 'iy', 'h',
         '            row.m_bytes += m_pitch;'),
        ('masked-darken', 'void Bitmap16Bit::darken(int x, int y, int w, int h, Bitmap816*', 'iy', 'h',
         '            maskRow += mask->getPitch();\n            row.m_bytes += m_pitch;'),
        ('colorize', 'void Bitmap16Bit::colorize(int x, int y, int w, int h, float', 'iy', 'h',
         '            row.m_bytes += m_pitch;'),
    ]
else:
    specs = [
        ('draw', 'void Bitmap24Bit::draw(int sx, int sy, int sw, int sh, unsigned short*', 'y', 'sh',
         '            dst = static_cast<unsigned short*>(static_cast<void*>(\n'
         '                static_cast<unsigned char*>(static_cast<void*>(dst))\n'
         '                + dpitch));\n            src += getPitch();'),
        ('adjust-hsv', 'void Bitmap24Bit::adjustHSV(int x, int y, int w, int h, float hue,\n', 'row', 'h',
         '        src += getPitch();'),
    ]


def guard(step, condition):
    indent = step[:len(step) - len(step.lstrip())]
    if '\n' not in step:
        return indent + 'if (' + condition + ')\n    ' + step
    return indent + 'if (' + condition + ') {\n' + '\n'.join(
        '    ' + line for line in step.splitlines()) + '\n' + indent + '}'


axes = []
for name, signature, counter, height, step in specs:
    start = source.index(signature)
    original = source[start:source.index('\n}', start) + 2]
    loop = f'for (int {counter} = 0; {counter} < {height}; ++{counter}) {{'
    count = original.count(step)
    assert count and original.count(loop) == count, name
    last_guard = original.replace(step, guard(step, f'{counter} + 1 < {height}'))
    first_guard = original.replace(step + '\n', '').replace(
        loop, loop + '\n' + guard(step, counter))
    # Integer row displacement can advance past the final row without forming
    # a pointer there. Bind actual cursors only on visited rows, retaining the
    # existing GetMap/GetPitch calls, pixel work and independent pitches.
    offset_form = None
    if args.unit == 'bitmap16':
        import re
        cursors = re.findall(r'(\w+)\.m_bytes \+= (\w+);', step)
        if name != 'masked-darken':
            offset_form = original.replace(step + '\n', '')
            declarations = []
            bindings = []
            for cursor, pitch in cursors:
                const = 'const ' if cursor == 'source' else ''
                declarations.append(f'{const}unsigned char* {cursor}Base = {cursor}.m_bytes;')
                bindings.append(f'{cursor}.m_bytes = {cursor}Base + {counter} * {pitch};')
            indent = step[:len(step) - len(step.lstrip())]
            offset_form = offset_form.replace(loop,
                ('\n' + indent[:-4]).join(declarations) + '\n' + indent[:-4] + loop
                + '\n' + indent + ('\n' + indent).join(bindings))
    axes.append({'name': name, 'find': original, 'options': [
        {'name': 'unchecked-control'},
        {'name': 'last-row-boundary', 'replace': last_guard},
        {'name': 'next-row-boundary', 'replace': first_guard},
    ]})
    if offset_form:
        axes[-1]['options'].append({'name': 'visited-row-offset', 'replace': offset_form})
    if args.unit == 'bitmap16' and name == 'draw':
        allocation_rows = original.replace('getMap(srcX, srcY)', 'getMap(0, srcY)').replace(
            'dstY * dstPitch + dstX * sizeof(unsigned short)', 'dstY * dstPitch').replace(
            'source.m_pixels[col]', 'source.m_pixels[srcX + col]').replace(
            'target.m_pixels[col]', 'target.m_pixels[dstX + col]').replace(
            'memcpy(target.m_pixels, source.m_pixels,',
            'memcpy(target.m_pixels + dstX, source.m_pixels + srcX,')
        axes[-1]['options'].append({'name': 'allocation-row-starts', 'replace': allocation_rows})
if args.selection:
    options = [{'name': 'unchanged-control'}]
    for selection in args.selection:
        choices = [int(value) for value in selection.split(',')]
        assert len(choices) == len(axes)
        result = source
        for axis, choice in zip(axes, choices):
            option = axis['options'][choice]
            result = result.replace(axis['find'], option.get('replace', axis['find']))
        options.append({'name': 'recombined-' + selection, 'replace': result})
    axes = [{'name': 'reproduced-row-parents', 'find': source, 'options': options}]
args.output.write_text(json.dumps({'schema': 1, 'source': path,
    'units': [args.unit], 'axes': axes}, indent=2) + '\n')
import math
print('Wrote', math.prod(len(axis['options']) for axis in axes),
      'bounded row combinations for', args.unit)
