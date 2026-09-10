"""DrawBolt's DC-proven helpers and palette-row lifetimes.

DC spells.cpp:3755 calls InCombatArea, 3781/3788 compute a palette row
before the following RGBto16/pixel statements, and all stores use GetMap(0,0).
Retail keeps a fixed 800-pixel stride; GetMap(x,y) is not the same operation.
The two pixel spellings also have an exact HoMM2 sibling in SOURCE/SPELLS.cpp.
No declaration permutations, fake calls, or inlining pins are generated.

Usage: python scripts/experiments/generate-drawbolt-family.py OUTPUT.json
       [PRE_EDIT_SPELLS.cpp]
The optional file permits regeneration against the retained experiment snapshot.
When using the adopted source, the first option preserves it as the control.
"""
import itertools
import json
from pathlib import Path
import sys

source = Path(sys.argv[2] if len(sys.argv) > 2 else 'src/spells.cpp').read_text()
start = source.index('void combatManager::drawBolt(')
end = source.index('\n}\n', start) + 2
original = source[start:end]
control = original
# Undo only the adopted, known row bindings to derive the historical axes.
if 'unsigned char* rgb = g_boltWhiteSpanColors[fromEdge];' in original:
    for name, table, index in (
        ('BOLT_COLOR_4', 'g_boltWhiteSpanColors', 'fromEdge'),
        ('BOLT_COLOR_2', 'g_boltGreenSpanColors', 'fromEdge'),
        ('BOLT_COLOR_0', 'g_boltSpectrumColors', 'k - spanFirst'),
        ('BOLT_COLOR_3', 'g_boltSpectrumColors', '14 - (k - spanFirst)'),
    ):
        begin = original.index('                    case ' + name + ': {')
        finish = original.index('\n                    }', begin) + len('\n                    }')
        arm = original[begin:finish].replace('case ' + name + ': {', 'case ' + name + ':')
        arm = arm.replace(f'                        unsigned char* rgb = {table}[{index}];\n', '')
        for channel in range(3):
            arm = arm.replace(f'rgb[{channel}]', f'{table}[{index}][{channel}]')
        original = original[:begin] + arm.removesuffix('\n                    }') + original[finish:]
    original = original.replace('!inCombatArea(x, y)', 'x < 0 || x >= 800 || y < 0 || y >= 556')
    original = original.replace('(g_windowManager->m_screenBitmap->getMap(0, 0) + y * 800)[x]',
                                'g_windowManager->m_screenBitmap->m_map[y * 800 + x]')


def palette_rows(body, mode):
    if mode == 'subscripts':
        return body
    if mode == 'shared-row':
        body = body.replace('    unsigned short color;',
                            '    unsigned short color;\n    unsigned char* rgb;')
    for name, table, index in (
        ('BOLT_COLOR_4', 'g_boltWhiteSpanColors', 'fromEdge'),
        ('BOLT_COLOR_2', 'g_boltGreenSpanColors', 'fromEdge'),
        ('BOLT_COLOR_0', 'g_boltSpectrumColors', 'k - spanFirst'),
        ('BOLT_COLOR_3', 'g_boltSpectrumColors', '14 - (k - spanFirst)'),
    ):
        begin = body.index('                    case ' + name + ':')
        finish = body.index('                        break;', begin) + len('                        break;')
        arm = body[begin:finish]
        if mode == 'shared-row':
            declaration = f'                        rgb = {table}[{index}];'
        else:
            qualifier = 'const ' if mode == 'const-row' else ''
            declaration = f'                        {qualifier}unsigned char* rgb = {table}[{index}];'
        arm = arm.replace('case ' + name + ':',
                          'case ' + name + ': {\n' + declaration)
        for channel in range(3):
            arm = arm.replace(f'{table}[{index}][{channel}]', f'rgb[{channel}]')
        arm += '\n                    }'
        body = body[:begin] + arm + body[finish:]
    return body


options = [] if control == original else [{'name': 'adopted-control'}]
for row, combat, getmap, pixel in itertools.product(
    ('subscripts', 'local-row', 'const-row', 'shared-row'),
    (False, True), (False, True), ('flat', 'row-index'),
):
    body = palette_rows(original, row)
    if combat:
        body = body.replace('x < 0 || x >= 800 || y < 0 || y >= 556', '!inCombatArea(x, y)')
    if getmap:
        body = body.replace('g_windowManager->m_screenBitmap->m_map',
                            'g_windowManager->m_screenBitmap->getMap(0, 0)')
    if pixel == 'row-index':
        base = ('g_windowManager->m_screenBitmap->getMap(0, 0)' if getmap else
                'g_windowManager->m_screenBitmap->m_map')
        body = body.replace(base + '[y * 800 + x]', '(' + base + ' + y * 800)[x]')
    name = f'{row}/bounds-{int(combat)}/map-{int(getmap)}/{pixel}'
    options.append({'name': name, **({'replace': body} if options else {})})
payload = {'schema': 1, 'source': 'src/spells.cpp', 'units': ['spells'],
           'axes': [{'name': 'dc-helper-row-shape', 'find': control, 'options': options}]}
Path(sys.argv[1]).write_text(json.dumps(payload, indent=2) + '\n')
print('Wrote', len(options), 'DrawBolt forms')
