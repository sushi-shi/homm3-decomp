#!/usr/bin/env python3
"""Nine meaningful joint mask-construction/world-lifetime models.

Scalar unchanged control plus whole/row mask constructors crossed with the
existing whole-function world copy, row-local copy, row-local three-int ctor,
and cell-local three-int ctor. The latter models keep signed row/column
induction and delay actual position ownership until needed. They do not add
bounds, cache prototype dimensions, change getMapItem overloads or add helpers.
Earlier scalar-mask families tested world cursors separately; this finite cross
specifically tests the constructor state that restored the third vector::size.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def world_lifetime(body, ownership):
    if ownership == 'whole_copy':
        return body
    old = '    TRmgMapPosition nearby = position;'
    assert body.count(old) == 1
    body = body.replace(old, '    int row = position.m_y;')
    body = body.replace('nearby.m_y', 'row')
    reset = '        nearby.m_x = position.m_x;'
    assert body.count(reset) == 1
    if ownership == 'row_copy':
        body = body.replace(reset,
            '        TRmgMapPosition nearby = position;\n'
            '        nearby.m_y = row;')
    elif ownership == 'row_ctor':
        body = body.replace(reset,
            '        TRmgMapPosition nearby(position.m_x, row, position.m_z);')
    elif ownership == 'cell_ctor':
        body = body.replace(reset, '        int column = position.m_x;')
        body = body.replace('nearby.m_x', 'column')
        query = '            TRmgMapItem* item = getMapItem(nearby);'
        assert body.count(query) == 1
        body = body.replace(query,
            '            TRmgMapPosition nearby(column, row, position.m_z);\n' + query)
    else:
        raise ValueError(ownership)
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    extract = generator('generate-rmg-position-family.py').definition
    mask = generator('generate-rmg-placement-mask-construction-family.py').transform
    names = ('isPlacementBlocked', 'addObject')
    originals = {name: extract(source, 'type_random_map::' + name) for name in names}
    states = [('scalar', 'unchanged')]
    states += [(scope, owner) for scope in ('whole', 'row')
               for owner in ('whole_copy', 'row_copy', 'row_ctor', 'cell_ctor')]
    options = []
    for scope, owner in states:
        edited = source
        if scope != 'scalar':
            for name, original in originals.items():
                body = world_lifetime(mask(original, scope, 'ctor'), owner)
                for marker in ('prototype.getWidth()', 'prototype.getHeight()',
                               'getMapItem(nearby)', 'CObjectType::getBitPos('):
                    assert body.count(marker) == original.count(marker), (name, marker)
                edited = edited.replace(original, body)
        options.append(dict(name=scope + '+' + owner, replace=edited))
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain'], evidence=__doc__,
        axes=[dict(name='joint_coordinate_ownership', source='src/rmg.cpp',
                   find=source, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, loaded, axes = load_manifest(args.output, HOMM3_DIR)
    held = extract(source, 'type_random_map::canPlaceObject')
    for index in range(len(states)):
        generated = render(loaded, axes, (index,))['src/rmg.cpp']
        assert extract(generated, 'type_random_map::canPlaceObject') == held
    print('9 finite joint coordinate owners; two constructor parents and unchanged control')


if __name__ == '__main__':
    main()
