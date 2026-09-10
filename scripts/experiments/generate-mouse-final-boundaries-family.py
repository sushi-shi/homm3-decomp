"""Ctor initialization and hotspot receiver lifetimes under canonical helpers.

DC stores the lock's section before EnterCriticalSection; it does not settle
member-initializer versus body assignment. The nested HidePointer expansion
still calls Enter directly where retail calls this ordinary constructor.
Update's remaining operand is the hotspot Y field, at the same retail address
as owner+4. Compare real receiver bindings without introducing data aliases.
"""
import json
from pathlib import Path
import sys

source = Path('src/mousemgr.cpp').read_text()
start = source.index('TCSLock::TCSLock(CRITICAL_SECTION* criticalSection)')
ctor = source[start:source.index('\n}', start)+2]
assert ': m_section(criticalSection)' in ctor and 'EnterCriticalSection(m_section);' in ctor
assignment = ctor.replace('    : m_section(criticalSection)\n', '').replace(
    '{\n', '{\n    m_section = criticalSection;\n')
ctor_options = [{'name':'ordinary-initializer-control'},
    {'name':'body-assignment','replace':assignment},
    {'name':'initializer-parameter-call','replace':ctor.replace('EnterCriticalSection(m_section)', 'EnterCriticalSection(criticalSection)')},
    {'name':'assignment-parameter-call','replace':assignment.replace('EnterCriticalSection(m_section)', 'EnterCriticalSection(criticalSection)')}]
start = source.index('void mouseManager::update(')
original = source[start:source.index('\n}', start)+2]
receiver = 'g_mouseHotSpots[m_set][m_frame]'
assert original.count(receiver) == 4
update_options = [{'name':'indexed-control'}]
for name,declaration,expression in [
    ('point-reference', 'const POINT& hotSpot = ' + receiver + ';', 'hotSpot'),
    ('point-pointer', 'const POINT* hotSpot = &' + receiver + ';', '(*hotSpot)'),
    ('set-row-pointer', 'const POINT* hotSpots = g_mouseHotSpots[m_set];', 'hotSpots[m_frame]')]:
    body = original.replace(receiver, expression).replace('    m_busy++;\n',
        '    m_busy++;\n    ' + declaration + '\n')
    update_options.append({'name':name,'replace':body})
Path(sys.argv[1]).write_text(json.dumps({'schema':1,'source':'src/mousemgr.cpp',
    'units':['mousemgr'],'axes':[
        {'name':'lock-initialization','find':ctor,'options':ctor_options},
        {'name':'hotspot-receiver','find':original,'options':update_options}]},indent=2)+'\n')
print('Wrote 16 canonical lock/hotspot forms')
