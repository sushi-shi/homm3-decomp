#!/usr/bin/env python3
"""Finite retail-backed rectangle-storage and frame-readiness families."""
import argparse
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('function', choices=('rects', 'frame', 'play'))
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/smackmgr.cpp').read_text()
axes = []

if args.function == 'rects':
    start = source.index('void videoDrawRects()\n{')
    body = source[start:source.index('\n}\n', start) + 2]
    # Use whole-function alternatives because storage and field assignments
    # are coupled. This is a finite semantic family, not random text edits.
    field = {'x': 'm_left', 'y': 'm_top', 'w': 'm_width', 'h': 'm_height'}
    first_smack = '''        x = smk->m_lastRectx;
        y = smk->m_lastRecty;
        w = smk->m_lastRectw;
        h = smk->m_lastRecth;'''
    first_bink = '''            w = bnk->m_frameRects[0].m_width;
            h = bnk->m_frameRects[0].m_height;
            x = bnk->m_frameRects[0].m_left;
            y = bnk->m_frameRects[0].m_top;'''
    smack_choice = '''        smk = 0;
        if (g_smackVideo)
            smk = g_smackVideo;
        else if (g_smackVideo2)
            smk = g_smackVideo2;'''
    bink_choice = '''        bnk = g_binkVideo2;
        if (g_binkVideo)
            bnk = g_binkVideo;'''
    options = [{'name': 'control'}]
    for storage in ('scalars', 'shared-rectangle', 'arm-rectangles', 'rectangle-references'):
        for order in ('control', 'xywh', 'whxy', 'hwyx'):
            for selection in ('control', 'ternary', 'if-else'):
                if (storage, order, selection) == ('scalars', 'control', 'control'):
                    continue
                changed = body
                if order != 'control':
                    changed = changed.replace(first_smack, '\n'.join('        ' + name + ' = smk->m_lastRect' + name + ';' for name in order))
                    changed = changed.replace(first_bink, '\n'.join('            ' + name + ' = bnk->m_frameRects[0].' + field[name] + ';' for name in order))
                if selection == 'ternary':
                    changed = changed.replace(smack_choice, '        smk = g_smackVideo ? g_smackVideo : g_smackVideo2;')
                    changed = changed.replace(bink_choice, '        bnk = g_binkVideo ? g_binkVideo : g_binkVideo2;')
                elif selection == 'if-else':
                    changed = changed.replace(smack_choice, '        if (g_smackVideo)\n            smk = g_smackVideo;\n        else\n            smk = g_smackVideo2;')
                    changed = changed.replace(bink_choice, '        if (g_binkVideo)\n            bnk = g_binkVideo;\n        else\n            bnk = g_binkVideo2;')
                if storage != 'scalars':
                    changed = changed.replace('    int x, y, w, h;\n', '')
                    if storage == 'rectangle-references':
                        declaration = '    BinkRect bounds;\n' + ''.join('    long& ' + name + ' = bounds.' + member + ';\n' for name, member in field.items())
                        changed = changed.replace('void videoDrawRects()\n{\n', 'void videoDrawRects()\n{\n' + declaration)
                    else:
                        # These names occur only as standalone scalar tokens
                        # in this function; the SDK fields and POINT x/y uses
                        # are deliberately excluded from the replacements.
                        changed = re.sub(r'(?<![\w.])\b([xywh])\b', lambda m: 'bounds.' + field[m[1]], changed)
                        if storage == 'shared-rectangle':
                            changed = changed.replace('void videoDrawRects()\n{\n', 'void videoDrawRects()\n{\n    BinkRect bounds;\n')
                        else:
                            changed = changed.replace('        Smack* smk;', '        BinkRect bounds;\n        Smack* smk;')
                            changed = changed.replace('            int i;\n', '            BinkRect bounds;\n            int i;\n')
                options.append({'name': '-'.join((storage, order, selection)), 'replace': changed})
    axes.append({'name': 'rectangle-storage-and-capture', 'find': body, 'options': options})
    evidence = [
        'DC 0x14ac60 is a video platform stub; Complete retail 0x5979d0 supplies the merge semantics and signed bounds.',
        'Retail homes three bounds across SmackToBufferRect and promotes the fourth; test the existing BinkRect representation and real field references.',
        'Preserve the original ordered minimum/extent updates, SDK calls, dirty stores and DirectDraw early return.',
        '48 finite combinations; only initial pure-field captures are reordered.',
    ]
elif args.function == 'play':
    start = source.index('int videoPlay(int id, int x, int y, int w, int h)\n{')
    body = source[start:source.index('\n}\n', start) + 2]
    buffer_call = '''            _SmackToBuffer(g_smackVideo, pos.x, pos.y,
                g_windowManager->m_screenBitmap->m_pitch,
                g_windowManager->m_screenBitmap->m_height,
                g_windowManager->m_screenBitmap->m_map, g_smackBufferFlags);'''
    assert buffer_call in body
    options = [{'name': 'control'}]
    for lifetime in ('parent', 'branch-point', 'branch-dimensions', 'branch-geometry', 'parameter-dimensions'):
        for mask in range(8):
            if (lifetime, mask) == ('parent', 0):
                continue
            changed = body
            call = buffer_call
            for bit, field, accessor in ((1, 'm_pitch', 'getPitch()'), (2, 'm_height', 'getHeight()'), (4, 'm_map', 'getMap(0, 0)')):
                if mask & bit:
                    call = call.replace('->' + field, '->' + accessor)
            changed = changed.replace(buffer_call, call)
            if lifetime in ('branch-point', 'branch-geometry'):
                changed = changed.replace('    POINT pos;\n', '')
                changed = changed.replace('            g_mouseManager->hidePointer();', '            POINT pos;\n            g_mouseManager->hidePointer();')
            if lifetime in ('branch-dimensions', 'branch-geometry'):
                changed = changed.replace('    int vw, vh;\n', '')
                changed = changed.replace('        vh = h;\n        vw = w;', '        int vh = h;\n        int vw = w;')
            elif lifetime == 'parameter-dimensions':
                changed = changed.replace('    int vw, vh;\n', '').replace('        vh = h;\n        vw = w;\n', '')
                changed = re.sub(r'\bvw\b', 'w', changed)
                changed = re.sub(r'\bvh\b', 'h', changed)
            options.append({'name': lifetime + '-accessors-' + str(mask), 'replace': changed})
    axes.append({'name': 'play-geometry-and-bitmap-interfaces', 'find': body, 'options': options})
    evidence = [
        'Complete 0x5972d0 supplies the video geometry and event-loop semantics; the DC video body is a platform stub.',
        'The exact sibling showVideo already calls Bitmap16Bit getPitch/getHeight/getMap at SmackToBuffer; preserve those canonical accessor bodies.',
        'Restoring getMap closed videoRealignBuffers through receiver/argument capture. Test that actual interface at this caller too.',
        '40 finite combinations preserve call order and the original unrotated wait loop; vary only real geometry lifetimes and bitmap access.',
    ]
else:
    head = '''    Smack* smk = g_smackVideo;
    if (!smk)
        smk = g_smackVideo2;'''
    axes.append({'name': 'track-selection', 'find': head, 'options': [
        {'name': 'control'},
        {'name': 'conditional-result', 'replace': '    Smack* smk = g_smackVideo ? g_smackVideo : g_smackVideo2;'},
        {'name': 'separate-initialization', 'replace': '    Smack* smk;\n    if (g_smackVideo)\n        smk = g_smackVideo;\n    else\n        smk = g_smackVideo2;'},
    ]})
    guard = '''    if (!smk || !g_smackFrameReady || _SmackWait(smk) || !g_smackAdvance) {
        g_smackDirty = 0;
        return;
    }
    g_smackDirty = 1;'''
    ready = 'smk && g_smackFrameReady && !_SmackWait(smk) && g_smackAdvance'
    options = [{'name': 'control'}]
    options.append({'name': 'global-readiness-result', 'replace': '    g_smackDirty = ' + ready + ';\n    if (!g_smackDirty)\n        return;'})
    for typ in ('bool', 'unsigned char', 'int'):
        options.append({'name': typ.replace(' ', '-') + '-readiness-result', 'replace':
                        '    const ' + typ + ' ready = ' + ready + ';\n    g_smackDirty = ready;\n    if (!ready)\n        return;'})
    axes.append({'name': 'frame-readiness-result', 'find': guard, 'options': options})
    evidence = [
        'DC 0x14adfc is a platform stub; Complete retail 0x598eb0 supplies the ordered four-test readiness predicate.',
        'Retail stores dirty=0 on the combined early exit and dirty=1 before the pause gate.',
        'Readiness-result variants preserve short circuit order and perform no dirty store before SmackWait.',
        '15 finite combinations; frame advance, close/fade, and auto-draw paths remain intact.',
    ]

for axis in axes:
    if source.count(axis['find']) != 1:
        raise ValueError('stale or ambiguous anchor: ' + axis['name'])
args.output.write_text(json.dumps({'schema': 1, 'source': 'src/smackmgr.cpp',
                                  'units': ['smackmgr'], 'axes': axes, 'evidence': evidence}, indent=2) + '\n')
