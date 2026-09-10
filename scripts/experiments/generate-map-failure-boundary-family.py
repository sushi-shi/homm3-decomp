"""Reuse the search exit for a required-definition failure, never a null result.

These Complete-only helpers have no direct DC counterpart. Retail establishes
the signed reverse scan; the shared throw is the requested safety correction.
Compare its placement at the loop exit and at the pre-decrement boundary.
"""
import json
from pathlib import Path
import sys

source = Path('src/mapcell.cpp').read_text()
axes = []
for signature,name in [('CObjectType* NewfullMap::newfullMapFn00505EA0(', 'lookup'),
                       ('void NewfullMap::newfullMapFn00505F20(', 'select')]:
    start = source.index(signature)
    original = source[start:source.index('\n}', start)+2]
    loop = '    int i = m_objectTypeIndex[objectType].size();\n    while (i--) {\n'
    failure = '    if (i < 0)\n        missingMapObjectDefinition();\n'
    assert loop in original and failure in original
    indexed = original.replace(loop, '    int i = m_objectTypeIndex[objectType].size() - 1;\n    for (; i >= 0; --i) {\n')
    boundary = original.replace(failure, '').replace(loop,
        '    int i = m_objectTypeIndex[objectType].size();\n    for (;;) {\n'
        '        if (i-- == 0)\n            missingMapObjectDefinition();\n')
    before = original.replace(failure, '').replace(loop,
        '    int i = m_objectTypeIndex[objectType].size();\n    for (;;) {\n'
        '        if (i == 0)\n            missingMapObjectDefinition();\n        --i;\n')
    axes.append({'name':name,'find':original,'options':[
        {'name':'adopted-safe-control'}, {'name':'signed-index-exit','replace':indexed},
        {'name':'predecrement-failure-boundary','replace':boundary},
        {'name':'before-decrement-failure-boundary','replace':before}]})
Path(sys.argv[1]).write_text(json.dumps({'schema':1,'source':'src/mapcell.cpp',
    'units':['mapcell'],'axes':axes},indent=2)+'\n')
print('Wrote 16 required-definition boundary forms')
