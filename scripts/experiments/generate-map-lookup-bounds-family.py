"""Bound the two retail reverse-search results using VC6's real vector::at.

Both helpers require an existing object definition: their callers dereference
or publish the selected type immediately. A miss must not return a null pointer
that those callers subsequently dereference. The checked accessor throws before
any invalid element access; successful searches preserve last-match precedence.

There is no direct DC counterpart for these Complete per-class index helpers.
DC game::InsertObject (game.cpp:9149..9158) confirms definition selection before
publication, but its older flat forward search is not the retail algorithm.
The original retail loops, signed reverse-index form, and a named vector owner
are the bounded source family. The control deliberately retains the old defect.
"""
import json
from pathlib import Path
import sys

source = Path('src/mapcell.cpp').read_text()
axes = []
for signature, name in (
    ('CObjectType* NewfullMap::newfullMapFn00505EA0(', 'lookup'),
    ('void NewfullMap::newfullMapFn00505F20(', 'select'),
):
    start = source.index(signature)
    original = source[start:source.index('\n}', start) + 2]
    if name == 'lookup':
        checked = original.replace('return &m_objectTypeIndex[objectType][i];',
                                   'return &m_objectTypeIndex[objectType].at(i);')
    else:
        checked = original.replace(
            'if (static_cast<short>(m_objectTypeIndex[objectType][i].m_objectTypeIndex) < 0)',
            'if (static_cast<short>(m_objectTypeIndex[objectType].at(i).m_objectTypeIndex) < 0)')
    assert checked != original
    indexed = checked.replace('int i = m_objectTypeIndex[objectType].size();\n    while (i--) {',
                              'int i;\n    for (i = m_objectTypeIndex[objectType].size() - 1; i >= 0; --i) {')
    owner = checked.replace('m_objectTypeIndex[objectType]', 'types')
    owner = owner.replace('{\n    int i',
                          '{\n    std::vector<CObjectType>& types = m_objectTypeIndex[objectType];\n    int i', 1)
    axes.append({'name': name, 'find': original, 'options': [
        {'name': 'retail-unchecked-control'},
        {'name': 'checked-result', 'replace': checked},
        {'name': 'checked-indexed-reverse', 'replace': indexed},
        {'name': 'checked-vector-owner', 'replace': owner},
    ]})
Path(sys.argv[1]).write_text(json.dumps({'schema': 1, 'source': 'src/mapcell.cpp',
                                      'units': ['mapcell'], 'axes': axes}, indent=2) + '\n')
print('Wrote 16 reverse-search boundary forms')
