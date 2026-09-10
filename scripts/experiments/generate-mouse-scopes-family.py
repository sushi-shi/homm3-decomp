"""DC-proven rectangle scopes and ordinary helper visibility for Update.

The first option preserves the old reconstruction as a measurement control.
DC mousemgr.cpp:798/844 supplies ordinary definitions after MouseCoords,
const RECT references, and two separate rectangle constructions in SaveAndDraw.
Update:639/700 and 648/708 supply branch-local origins and surface descriptors.
Windows keeps the retail-proven v1 descriptor, not DC's v4 descriptor.
No dummy source, false inline declaration, or compiler pragma is introduced.
"""
import json
from pathlib import Path
import sys

source = Path('src/mousemgr.cpp').read_text()
start = source.index('__forceinline void mouseManager::saveAndDraw(')
end = source.index('// E:\\gamedcs\\mousemgr.cpp:526', start)
helpers = source[start:end]
init_start = helpers.index('        RECT saveRect;')
init_end = helpers.index('        OffsetRect(', init_start)
init = helpers[init_start:init_end]
separate = '''        RECT saveRect;
        saveRect.left = saveRect.top = 0;
        saveRect.right = dstRect.right - dstRect.left;
        saveRect.bottom = dstRect.bottom - dstRect.top;
        RECT sourceRect;
        sourceRect.left = sourceRect.top = 0;
        sourceRect.right = dstRect.right - dstRect.left;
        sourceRect.bottom = dstRect.bottom - dstRect.top;
'''
aggregate = '''        RECT saveRect = {0, 0, dstRect.right - dstRect.left,
                         dstRect.bottom - dstRect.top};
        RECT sourceRect = {0, 0, dstRect.right - dstRect.left,
                           dstRect.bottom - dstRect.top};
'''
copy = '''        RECT saveRect = {0, 0, dstRect.right - dstRect.left,
                         dstRect.bottom - dstRect.top};
        RECT sourceRect = saveRect;
'''
options = [{'name': 'forced-interleaved-control'}]
anchor = '// E:\\gamedcs\\mousemgr.cpp:774'
mouse_start = source.index('void mouseManager::mouseCoords(')
after_coords = source.index('\n}', mouse_start) + 2
insertion = source[after_coords:source.index('// E:\\gamedcs\\mousemgr.cpp:798', after_coords)]
for name, rect_init in [('separate', separate), ('aggregate', aggregate), ('copy', copy)]:
    body = helpers.replace('__forceinline ', '').replace(init, rect_init)
    options.append({'name': 'ordinary-before-' + name, 'replace': body})
    options.append({'name': 'ordinary-source-order-' + name, 'replace': '',
                    'extra_edits': [{'source': 'src/mousemgr.cpp',
                                     'find': source[mouse_start:after_coords],
                                     'replace': source[mouse_start:after_coords] + '\n\n' + body.rstrip()}]})
options.append({'name': 'ordinary-before-interleaved',
                'replace': helpers.replace('__forceinline ', '')})

start = source.index('void mouseManager::update(')
original = source[start:source.index('\n}', start) + 2]
update_options = [{'name': 'shared-locals-control'}]
for origins, descriptors in [(True, False), (False, True), (True, True)]:
    body = original
    tail_start = body.index('        UnionRect(')
    head, tail = body[:tail_start], body[tail_start:]
    if origins:
        tail = tail.replace('cursor', 'windowOrigin')
        tail = tail.replace('        windowOrigin.x = 0;',
                            '        POINT windowOrigin;\n        windowOrigin.x = 0;')
    if descriptors:
        head = head.replace('    DDSURFACEDESC surfaceDesc;\n', '')
        tail = tail.replace('        memset(&surfaceDesc',
                            '        DDSURFACEDESC surfaceDesc;\n        memset(&surfaceDesc')
    update_options.append({'name': 'branch-' + ('origins' if origins else '')
                           + ('-descriptors' if descriptors else ''), 'replace': head + tail})

Path(sys.argv[1]).write_text(json.dumps({'schema': 1, 'source': 'src/mousemgr.cpp',
    'units': ['mousemgr'], 'axes': [
        {'name': 'canonical-helpers', 'find': helpers, 'options': options},
        {'name': 'update-scopes', 'find': original, 'options': update_options},
    ]}, indent=2) + '\n')
print('Wrote 32 helper/lifetime combinations')
