"""Bound the fill row address to rows actually visited, without ABI changes.

DC bitmap16.cpp:679 and retail 0x44e4c0 prove the clipped row/column loops.
The original pointer update is the control. Other forms express row indexing,
a byte offset, or the last-row boundary; no artificial integer addresses.
"""
import json
from pathlib import Path
import sys

source = Path('src/bitmap16.cpp').read_text()
start = source.index('    if (w && h) {', source.index('void Bitmap16Bit::fillRect('))
end = source.index('\n}', start)
original = source[start:end]
indexed = '''    if (w && h) {
        for (int row = 0; row < h; ++row) {
            unsigned short* dst = getMap(x, y + row);
            for (int col = 0; col < w; ++col)
                dst[col] = color;
        }
    }'''
offset = '''    if (w && h) {
        Bitmap16MapPointer base;
        base.m_pixels = getMap(x, y);
        int offset = 0;
        for (int row = 0; row < h; ++row) {
            Bitmap16MapPointer dst;
            dst.m_bytes = base.m_bytes + offset;
            for (int col = 0; col < w; ++col)
                dst.m_pixels[col] = color;
            offset += m_pitch;
        }
    }'''
product = offset.replace('        int offset = 0;\n', '').replace(
    'base.m_bytes + offset', 'base.m_bytes + row * m_pitch').replace(
    '            offset += m_pitch;\n', '')
guarded = original.replace('            dst.m_bytes += m_pitch;',
    '            if (row + 1 < h)\n                dst.m_bytes += m_pitch;')
forms = [('retail-pointer-control', original), ('get-map-per-row', indexed),
         ('byte-offset-cursor', offset), ('byte-offset-product', product),
         ('last-row-guard', guarded)]
payload = {'schema': 1, 'source': 'src/bitmap16.cpp', 'units': ['bitmap16'],
    'axes': [{'name': 'bounded-row-address', 'find': original, 'options': [
        {'name': name, **({'replace': body} if i else {})}
        for i, (name, body) in enumerate(forms)]}]}
Path(sys.argv[1]).write_text(json.dumps(payload, indent=2) + '\n')
print('Wrote five fill-row forms')
