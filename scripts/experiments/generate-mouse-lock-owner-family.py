"""Test the TU-private DC TCSLock class at its source-order boundary.

No false inline declarations or inline-depth overrides: the unchanged class
body moves from the shared header to mousemgr.cpp, immediately before its
first source function (DC constructor lines 291/298, mouse ctor line 315).
"""
import json
from pathlib import Path
import sys

header = Path('include/mousemgr.h').read_text()
source = Path('src/mousemgr.cpp').read_text()
start = header.index('// mousemgr.cpp\'s critical-section RAII guard')
end = header.index('// Bootstrap VIEW:', start)
guard = header[start:end]
anchor = '// E:\\gamedcs\\mousemgr.cpp:315\n'
assert source.count(anchor) == 1
Path(sys.argv[1]).write_text(json.dumps({'schema': 1,
    'source': 'include/mousemgr.h', 'units': ['mousemgr'], 'axes': [{
        'name': 'lock-owner', 'find': guard, 'options': [
            {'name': 'shared-header-control'},
            {'name': 'dc-tu-private-class', 'replace': '', 'extra_edits': [{
                'source': 'src/mousemgr.cpp', 'find': anchor,
                'replace': guard + anchor}]}]}]}, indent=2) + '\n')
