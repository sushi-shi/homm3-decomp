"""Test ordinary TCSLock definitions at the DC source-order boundary.

The class remains canonical in its header. DC attributes both bodies to
mousemgr.cpp:291/298; retail retains both standalone bodies and selectively
expands their calls. The existing in-class definitions are controls, not proof
that the original declarations requested inlining. No pragma or dummy use.
"""
import json
from pathlib import Path
import sys

header = Path('include/mousemgr.h').read_text()
start = header.index('class TCSLock {')
original = header[start:header.index('\n};', start) + 3]
constructor = '''    TCSLock(CRITICAL_SECTION* criticalSection)
        : m_section(criticalSection) {
        EnterCriticalSection(m_section);
    }'''
destructor = '    ~TCSLock() { LeaveCriticalSection(m_section); }'
assert constructor in original and destructor in original
anchor = '// E:\\gamedcs\\mousemgr.cpp:315\n'
assert Path('src/mousemgr.cpp').read_text().count(anchor) == 1
options = [{'name': 'in-class-control'}]
for ctor, dtor in [(True, False), (False, True), (True, True)]:
    declaration = original
    definitions = ''
    if ctor:
        declaration = declaration.replace(constructor, '    TCSLock(CRITICAL_SECTION* criticalSection);')
        definitions += '''TCSLock::TCSLock(CRITICAL_SECTION* criticalSection)
    : m_section(criticalSection)
{
    EnterCriticalSection(m_section);
}

'''
    if dtor:
        declaration = declaration.replace(destructor, '    ~TCSLock();')
        definitions += '''TCSLock::~TCSLock()
{
    LeaveCriticalSection(m_section);
}

'''
    options.append({'name': 'ordinary-' + ('ctor' if ctor else '') + ('-dtor' if dtor else ''),
        'replace': declaration, 'extra_edits': [{'source': 'src/mousemgr.cpp',
            'find': anchor, 'replace': definitions + anchor}]})
Path(sys.argv[1]).write_text(json.dumps({'schema': 1, 'source': 'include/mousemgr.h',
    'units': ['mousemgr'], 'axes': [{'name': 'canonical-lock-boundaries',
        'find': original, 'options': options}]}, indent=2) + '\n')
