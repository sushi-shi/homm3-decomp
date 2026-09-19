#!/usr/bin/env python3
"""One value-size convenience overload owns the virtual query's temporary.

The helper is a genuine nonvirtual interface overload, defined in-class so both
RMG translation units see its body. Its returned value ends the output owner's
lifetime naturally; both adapters and the terrain painter consume that value.
The existing virtual output-reference contract and result-pointer semantics
remain unchanged. In-class source placement is inferred, not DC-proven.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator

HELPER = '''    TRmgGridPoint getSize()
    {
        TRmgGridPoint size;
        return getSize(size);
    }
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    paths = ('include/rmg.h', 'src/rmg.cpp', 'src/rmg_terrain.cpp')
    original = {p: (HOMM3_DIR / p).read_text() for p in paths}
    live = dict(original)
    adopted = HELPER in original[paths[0]]
    if adopted:
        original[paths[0]] = original[paths[0]].replace(HELPER, '').replace(
            '    using TRmgMapInterface::getSize;\n', '')
        original[paths[1]] = original[paths[1]].replace(
            '    return m_map->getSize();',
            '    TRmgGridPoint size;\n    return m_map->getSize(size);')
        query = '    m_size = m_adapter->getSize();'
        assert original[paths[2]].count(query) == 1
        original[paths[2]] = original[paths[2]].replace(query,
            '    {\n        TRmgGridPoint size;\n        m_size = m_adapter->getSize(size);\n    }')
    states = [('scoped_diagnostic', original)]
    changed = dict(original)
    anchor = '    virtual TRmgGridPoint& getSize(TRmgGridPoint& output) = 0;\n'
    assert changed[paths[0]].count(anchor) == 1
    changed[paths[0]] = changed[paths[0]].replace(anchor, anchor + HELPER)
    override = '    virtual TRmgGridPoint& getSize(TRmgGridPoint& output);\n'
    assert changed[paths[0]].count(override) == 1
    changed[paths[0]] = changed[paths[0]].replace(override, '    using TRmgMapInterface::getSize;\n' + override)
    extract = generator('generate-rmg-position-family.py').definition
    for owner in ('TRmgMapAdapter', 'TRmgRoadMapAdapter'):
        old = extract(changed[paths[1]], owner + '::getSize')
        body = old.replace('    TRmgGridPoint size;\n    return m_map->getSize(size);', '    return m_map->getSize();')
        assert body != old
        changed[paths[1]] = changed[paths[1]].replace(old, body)
    old = extract(changed[paths[2]], 'rmgTerrainPainter::rmgTerrainPainter')
    body_start = old.index('\n{') + 2
    resize = old.index('    m_packedCells.resize', body_start)
    body = old[:body_start] + '\n    m_size = m_adapter->getSize();\n' + old[resize:]
    changed[paths[2]] = changed[paths[2]].replace(old, body)
    if adopted:
        changed = dict(live)
        body = extract(changed[paths[2]], 'rmgTerrainPainter::rmgTerrainPainter')
    states.append(('canonical_value_helper_assignment', changed))
    initialized = dict(changed)
    constructor = body.replace('m_transitionStrength(strength)\n',
                               'm_transitionStrength(strength),\n      m_size(m_adapter->getSize())\n')
    constructor = constructor.replace('    m_size = m_adapter->getSize();\n', '')
    assert constructor != body
    initialized[paths[2]] = initialized[paths[2]].replace(body, constructor)
    states.append(('canonical_value_helper_member_initialization', initialized))
    if adopted:
        states = [states[1], states[0], states[2]]
    original = live
    payload = dict(schema=1, units=generator('generate-rmg-map-accessor-family.py').UNITS,
                   evidence=__doc__, axes=[dict(name='size_helper_ownership', source=paths[0], find=original[paths[0]],
                     options=[dict(name=name, replace=files[paths[0]], extra_edits=[dict(source=p, find=original[p], replace=files[p]) for p in paths[1:]]) for name, files in states])])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, originals, axes = load_manifest(args.output, HOMM3_DIR)
    for i, (_name, files) in enumerate(states):
        assert render(originals, axes, (i,)) == files
    print('three states: unchanged authored model first; scoped diagnostic; canonical helper assignment/initialization')


if __name__ == '__main__':
    main()
