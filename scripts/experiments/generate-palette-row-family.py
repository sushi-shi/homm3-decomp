"""DC palette.cpp:95..109: bounded RGB-row and object-representation views.

The old first-row byte cursor is the control, not an eligible adopted form.
Keep the established conversion order, mask lifetimes and destination walk.
"""
import json
from pathlib import Path
import sys

source = Path('src/palette.cpp').read_text()
typed = '    const unsigned char (*src)[3] = p24->m_colors.m_data;'
flat = '    const unsigned char* src = p24->m_colors.m_data[0];'
start = source.index(typed if typed in source else flat)
end = source.index('\n    }', start) + len('\n    }')
current = source[start:end]
original = current.replace(typed, flat).replace('++src;', 'src += 3;')
for channel in range(3):
    original = original.replace(f'(*src)[{channel}]', f'src[{channel}]')
indexed = original.replace('    const unsigned char* src = p24->m_colors.m_data[0];\n', '')
indexed = indexed.replace('    for (int index = 0; index < 256; ++index) {',
    '    for (int index = 0; index < 256; ++index) {\n        const unsigned char* src = p24->m_colors.m_data[index];')
indexed = indexed.replace('        src += 3;\n', '')
rows = original.replace('const unsigned char* src = p24->m_colors.m_data[0]',
                        'const unsigned char (*src)[3] = p24->m_colors.m_data')
for channel in range(3):
    rows = rows.replace(f'src[{channel}]', f'(*src)[{channel}]')
rows = rows.replace('src += 3;', '++src;')
bytes_view = original.replace('p24->m_colors.m_data[0]',
    'static_cast<const unsigned char*>(static_cast<const void*>(&p24->m_colors))')
forms = [('first-row-byte-control', original), ('indexed-triplet', indexed),
         ('typed-row-cursor', rows), ('complete-object-bytes', bytes_view)]
forms.sort(key=lambda form: form[1] != current)
assert forms[0][1] == current
payload = {'schema': 1, 'source': 'src/palette.cpp', 'units': ['palette'],
    'axes': [{'name': 'rgb-row-owner', 'find': current, 'options': [
        {'name': name, **({'replace': body} if i else {})}
        for i, (name, body) in enumerate(forms)]}]}
Path(sys.argv[1]).write_text(json.dumps(payload, indent=2) + '\n')
print('Wrote four palette-row forms')
