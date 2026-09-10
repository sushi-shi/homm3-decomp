"""Consistent failure contract for retail-only required-definition searches.

No direct DC counterpart exists for these Complete per-class reverse searches.
Retail and all callers require a definition; a miss cannot safely return null.
Test checked access against an explicit throw, including a shared, ordinary
failure helper. The exception is a deliberate safety correction, not claimed
to reconstruct retail's invalid-index behavior. No inline steering is used.
"""
import json
from pathlib import Path
import sys

source = Path('src/mapcell.cpp').read_text()
start = source.index('CObjectType* NewfullMap::newfullMapFn00505EA0(')
last = source.index('void NewfullMap::newfullMapFn00505F20(', start)
original = source[start:source.index('\n}', last) + 2]
lookup = '    return &m_objectTypeIndex[objectType][i];'
select = '    if (static_cast<short>(m_objectTypeIndex[objectType][i].m_objectTypeIndex) < 0) {'
checked = original.replace(lookup, '    return &m_objectTypeIndex[objectType].at(i);').replace(
    select, '    if (static_cast<short>(m_objectTypeIndex[objectType].at(i).m_objectTypeIndex) < 0) {')
error = 'throw std::out_of_range("Map object definition not found");'
direct = original.replace(lookup, '    if (i < 0)\n        ' + error + '\n' + lookup).replace(
    select, '    if (i < 0)\n        ' + error + '\n\n' + select)
shared = direct.replace(error, 'missingMapObjectDefinition();')
helper = '''// Safety failure shared by the two required-definition searches below.
// Their callers immediately consume the result; throwing leaves them no null
// or invalid record to dereference. This path is absent in retail.
static void missingMapObjectDefinition()
{
    throw std::out_of_range("Map object definition not found");
}

'''
anchor = '// 0x505ea0 (game::ConvertObject'
payload = {'schema': 1, 'source': 'src/mapcell.cpp', 'units': ['mapcell'],
    'axes': [{'name': 'required-definition-failure', 'find': original, 'options': [
        {'name': 'unchecked-retail-control'},
        {'name': 'checked-vector-access', 'replace': checked},
        {'name': 'explicit-throw', 'replace': direct},
        {'name': 'shared-failure', 'replace': shared, 'extra_edits': [
            {'source': 'src/mapcell.cpp', 'find': anchor, 'replace': helper + anchor}]},
    ]}]}
Path(sys.argv[1]).write_text(json.dumps(payload, indent=2) + '\n')
print('Wrote four consistent required-definition failure forms')
