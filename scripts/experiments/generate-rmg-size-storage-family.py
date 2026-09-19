#!/usr/bin/env python3
"""Canonical value query coupled with the painter's real size/storage phase.

The first value-helper family preserves the virtual query and its result copy,
but outlines packed-vector count insertion. Follow the same assignment parent
through existing coordinate accessors/direct dimensions and one ordinary
painter resize method owning size assignment plus storage resize. No artificial
constructor block, inline qualifier, copied vector body or unused operation.
"""
import argparse
import json
from pathlib import Path
import sys
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    parent = generator('generate-rmg-size-helper-family.py')
    with tempfile.TemporaryDirectory(prefix='rmg-size-storage-parent-') as directory:
        path = Path(directory) / 'parent.json'
        saved = sys.argv
        try:
            sys.argv = [str(parent.__file__), str(path)]
            parent.main()
        finally:
            sys.argv = saved
        _, originals, axes = load_manifest(path, HOMM3_DIR)
        named = {option.name: render(originals, axes, (index,))
                 for index, option in enumerate(axes[0].options)}
        control = named['scoped_diagnostic']
        assignment = named['canonical_value_helper_assignment']
        authored = render(originals, axes, (0,))
    header_path = 'include/rmg_terrain.h'
    control[header_path] = (HOMM3_DIR / header_path).read_text()
    assignment[header_path] = control[header_path]
    states = [('scoped_diagnostic', control), ('value_helper_assignment', assignment)]
    terrain_path = 'src/rmg_terrain.cpp'
    extract = generator('generate-rmg-position-family.py').definition
    ctor = extract(assignment[terrain_path], 'rmgTerrainPainter::rmgTerrainPainter')
    dimensions = 'getWidth() * getHeight()'
    assert ctor.count(dimensions) == 1
    for name, expression in [('coordinate_accessors', 'm_size.getX() * m_size.getY()'),
                             ('coordinate_fields', 'm_size.m_x * m_size.m_y')]:
        state = dict(assignment)
        state[terrain_path] = state[terrain_path].replace(ctor, ctor.replace(dimensions, expression))
        states.append((name, state))
    resized = dict(assignment)
    declaration = '    ~rmgTerrainPainter();\n'
    assert resized[header_path].count(declaration) == 1
    resized[header_path] = resized[header_path].replace(declaration,
        declaration + '    void resize(const TRmgGridPoint& size);\n')
    body = '''void rmgTerrainPainter::resize(const TRmgGridPoint& size)
{
    m_size = size;
    m_packedCells.resize(getWidth() * getHeight(), TRmgPackedTerrainCell());
}

'''
    initialization = '''    m_size = m_adapter->getSize();
    m_packedCells.resize(getWidth() * getHeight(), TRmgPackedTerrainCell());'''
    assert ctor.count(initialization) == 1
    changed_ctor = ctor.replace(initialization, '    resize(m_adapter->getSize());')
    resized[terrain_path] = resized[terrain_path].replace(ctor, changed_ctor)
    anchor = 'VA(0x005B45F0, 0x26D)\n'
    assert resized[terrain_path].count(anchor) == 1
    resized[terrain_path] = resized[terrain_path].replace(anchor, body + anchor)
    states.append(('ordinary_painter_size_storage', resized))
    authored[header_path] = control[header_path]
    states.sort(key=lambda item: item[1] != authored)
    assert states[0][1] == authored
    control = authored
    primary = 'include/rmg.h'
    payload = dict(schema=1, units=generator('generate-rmg-map-accessor-family.py').UNITS,
        evidence=__doc__, axes=[dict(name='size_storage_ownership', source=primary, find=control[primary],
            options=[dict(name=name, replace=state[primary], extra_edits=[
                dict(source=path, find=control[path], replace=state[path])
                for path in control if path != primary]) for name, state in states])])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, originals, axes = load_manifest(args.output, HOMM3_DIR)
    for i, (_name, state) in enumerate(states):
        assert render(originals, axes, (i,)) == state
    print('five states: scoped control, value helper, coordinate accessors/fields, ordinary painter resize')


if __name__ == '__main__':
    main()
