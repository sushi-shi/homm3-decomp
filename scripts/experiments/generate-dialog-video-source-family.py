#!/usr/bin/env python3
"""Build bounded families for dialog result homes and video map access."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('function', choices=('dialog', 'video'))
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
unit = 'kb' if args.function == 'dialog' else 'smackmgr'
source = (root / ('src/' + unit + '.cpp')).read_text()
axes = []

if args.function == 'dialog':
    anchor = '                g_windowManager->m_dialogReturn = g_normalDialogSelection;'
    options = [{'name': 'control'}]
    for name, declaration, assignment in (
        ('selection-value', 'int selection = g_normalDialogSelection;',
         'g_windowManager->m_dialogReturn = selection;'),
        ('selection-const-value', 'const int selection = g_normalDialogSelection;',
         'g_windowManager->m_dialogReturn = selection;'),
        ('result-reference', 'int& dialogReturn = g_windowManager->m_dialogReturn;',
         'dialogReturn = g_normalDialogSelection;'),
        ('manager-pointer', 'heroWindowManager* manager = g_windowManager;',
         'manager->m_dialogReturn = g_normalDialogSelection;'),
        ('manager-reference', 'heroWindowManager& manager = *g_windowManager;',
         'manager.m_dialogReturn = g_normalDialogSelection;'),
    ):
        options.append({'name': name, 'replace':
                        '                ' + declaration + '\n                ' + assignment})
    axes.append({'name': 'answer-result-lifetime', 'find': anchor, 'options': options})
    for choice in (1, 2):
        first, second = (1, 0) if choice == 1 else (0, 1)
        anchor = f'''                widget* first =
                    getCurrentNormalDialog()->getWidget(DIALOG_RETURN_CHOICE_1);
                first->setVisible({first});
                widget* second =
                    getCurrentNormalDialog()->getWidget(DIALOG_RETURN_CHOICE_2);
                second->setVisible({second});'''
        direct = f'''                getCurrentNormalDialog()->getWidget(DIALOG_RETURN_CHOICE_1)->setVisible({first});
                getCurrentNormalDialog()->getWidget(DIALOG_RETURN_CHOICE_2)->setVisible({second});'''
        scoped = f'''                {{
                    widget* first = getCurrentNormalDialog()->getWidget(DIALOG_RETURN_CHOICE_1);
                    first->setVisible({first});
                }}
                {{
                    widget* second = getCurrentNormalDialog()->getWidget(DIALOG_RETURN_CHOICE_2);
                    second->setVisible({second});
                }}'''
        axes.append({'name': 'choice-' + str(choice) + '-widget-results', 'find': anchor,
                     'options': [{'name': 'control'}, {'name': 'direct-receivers', 'replace': direct},
                                 {'name': 'scoped-receivers', 'replace': scoped}]})
    evidence = [
        'DC 0xe206c names getCurrentNormalDialog and widget set_visible; retain both boundaries and source call order.',
        'Retail 0x4f0fc0 differs at the selection-to-dialogReturn assignment register homes; answer paths and deadlines stay intact.',
        '54 finite source combinations; no helper or inliner padding is introduced.',
    ]
else:
    anchor = '''    g_binkBuffer = 2 * g_binkX + g_windowManager->m_screenBitmap->m_pitch * g_binkY
        + static_cast<unsigned char*>(
              static_cast<void*>(g_windowManager->m_screenBitmap->m_map));
    g_binkPitch = g_windowManager->m_screenBitmap->m_pitch;
    g_binkHeight = g_windowManager->m_screenBitmap->m_height;'''
    options = [{'name': 'control'}]
    for binding in ('direct', 'pointer', 'reference'):
        receiver = 'g_windowManager->m_screenBitmap' if binding == 'direct' else 'screen'
        op = '->' if binding in ('direct', 'pointer') else '.'
        prefix = {
            'direct': '',
            'pointer': '    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;\n',
            'reference': '    Bitmap16Bit& screen = *g_windowManager->m_screenBitmap;\n',
        }[binding]
        for map_access in ('fields', 'get-map'):
            for dimensions in ('fields', 'accessors'):
                if (binding, map_access, dimensions) == ('direct', 'fields', 'fields'):
                    continue
                if map_access == 'get-map':
                    expr = f'static_cast<unsigned char*>(static_cast<void*>({receiver}{op}getMap(g_binkX, g_binkY)))'
                else:
                    expr = f'2 * g_binkX + {receiver}{op}m_pitch * g_binkY + static_cast<unsigned char*>(static_cast<void*>({receiver}{op}m_map))'
                pitch, height = ('getPitch()', 'getHeight()') if dimensions == 'accessors' else ('m_pitch', 'm_height')
                replacement = prefix + f'''    g_binkBuffer = {expr};
    g_binkPitch = {receiver}{op}{pitch};
    g_binkHeight = {receiver}{op}{height};'''
                options.append({'name': '-'.join((binding, map_access, dimensions)), 'replace': replacement})
    axes.append({'name': 'bitmap-receiver-and-accessors', 'find': anchor, 'options': options})
    evidence = [
        'DC videoRealignBuffers is a platform stub; no DC body is asserted for Complete.',
        'Retail 0x5971f0 computes y*pitch + 2*x + map; canonical Bitmap16Bit::getMap has those same semantics.',
        'The current five-row residual is add/load scheduling. Test actual bitmap interfaces and receiver lifetimes, preserving all SDK calls.',
        '12 finite source combinations; shared accessor bodies remain untouched.',
    ]

for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('source anchor is stale or ambiguous: ' + axis['name'])
args.output.write_text(json.dumps({'schema': 1, 'source': 'src/' + unit + '.cpp',
                                  'units': [unit], 'evidence': evidence, 'axes': axes}, indent=2) + '\n')
