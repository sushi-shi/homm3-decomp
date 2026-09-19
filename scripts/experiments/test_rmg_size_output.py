#!/usr/bin/env python3
"""Native contracts for actual map-size bodies and output-reference callers.

The fixture isolates size dispatch, not vtable layout or VC6 ABI. It imports
actual coordinate declarations and method bodies. Ordinary maps test dimension
conversion and value copies; reference-model mocks additionally return storage
other than the supplied output, as permitted by the recovered interface.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def program(files, name, broken=None):
    header, source, terrain = (files[p] for p in ('include/rmg.h', 'src/rmg.cpp', 'src/rmg_terrain.cpp'))
    extract = generator('generate-rmg-position-family.py').definition
    types = []
    for label in ('TRmgVector', 'TPoint'):
        start = header.index('struct ' + label + ' {')
        types.append(header[start:header.index('\n};', start)+3])
    start = header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    end = header.index('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    end += len('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    types.append(header[start:end])
    owners = ('type_random_map', 'TRmgMapAdapter', 'TRmgRoadMapAdapter')
    methods = {owner: extract(source, owner + '::getSize') for owner in owners}
    reference = 'TRmgGridPoint& type_random_map::getSize(' in methods[owners[0]]
    start = header.index('class TRmgMapInterface {')
    interface = header[start:header.index('\n};', start)]
    helper = ''
    if '    TRmgGridPoint getSize()\n' in interface:
        helper = extract(interface, 'getSize', parameters='')
        assert reference and '    using TRmgMapInterface::getSize;' in header
    if broken == 'dimensions':
        methods[owners[0]] = methods[owners[0]].replace('m_mapWidth, m_mapHeight', 'm_mapHeight, m_mapWidth')
    if broken == 'returned_reference':
        assert reference
        if helper:
            helper = helper.replace('return getSize(size);', 'getSize(size); return size;')
        else:
            for owner in owners[1:]:
                methods[owner] = methods[owner].replace('return m_map->getSize(size);',
                    'm_map->getSize(size); return size;')
    if broken == 'extra_query':
        assert reference
        if helper:
            helper = helper.replace('return getSize(size);', 'getSize(size); return getSize(size);')
        else:
            for owner in owners[1:]:
                methods[owner] = methods[owner].replace('return m_map->getSize(size);',
                    'm_map->getSize(size); return m_map->getSize(size);')
    constructor = extract(terrain, 'rmgTerrainPainter::rmgTerrainPainter')
    opening = constructor.index('\n{')
    member_initialization = 'm_size(m_adapter->getSize())' in constructor[:opening]
    painter = constructor[opening + 2:constructor.rindex('}')]
    resize_helper = ''
    if 'resize(m_adapter->getSize());' in painter:
        resize_helper = extract(terrain, 'rmgTerrainPainter::resize').replace('rmgTerrainPainter::', '')
    terrain_header = files.get('include/rmg_terrain.h', (HOMM3_DIR/'include/rmg_terrain.h').read_text())
    for label in ('rmgTerrainTile', 'TRmgPackedTerrainCell'):
        start = terrain_header.index('struct ' + label + ' {')
        types.append(terrain_header[start:terrain_header.index('\n};', start)+3])
    if broken == 'default_cell':
        assert 'm_initialized(0)' in types[-1]
        types[-1] = types[-1].replace('m_initialized(0)', 'm_initialized(1)')
    if broken == 'storage_count':
        if resize_helper:
            resize_helper = resize_helper.replace('getWidth() * getHeight()', 'getWidth() * getHeight() + 1')
        else:
            assert 'm_packedCells.resize(' in painter
            painter = painter.replace('m_packedCells.resize(', 'm_packedCells.resize(1 + ')
    if broken == 'painter_reference':
        assert reference
        if helper:
            if member_initialization:
                member_initialization = False
                painter = 'm_adapter->getSize(m_size);\n' + painter
            else:
                # Mutate the consumed value, independently of whether its owner
                # is a member assignment, named snapshot or resize argument.
                assert painter.count('m_adapter->getSize()') == 1
                painter = ('TRmgGridPoint wrongOutput; m_adapter->getSize(wrongOutput);\n'
                           + painter.replace('m_adapter->getSize()', 'wrongOutput'))
        else:
            painter = painter.replace('m_size = m_adapter->getSize(size);',
                'm_adapter->getSize(size); m_size = size;')
    declarations = []
    if reference:
        declarations.append('struct TRmgMapInterface { virtual TRmgGridPoint& getSize(TRmgGridPoint& output) = 0;\n' + helper + '\n};')
    for owner in owners:
        signature = methods[owner].split('\n{', 1)[0].replace(owner + '::', '')
        members = 'int m_mapWidth; int m_mapHeight;' if owner == owners[0] else 'type_random_map* m_map;'
        inheritance = ' : TRmgMapInterface' if reference and owner == owners[0] else ''
        using = ' using TRmgMapInterface::getSize; ' if helper and owner == owners[0] else ''
        declarations.append('struct ' + owner + inheritance + ' { ' + members + using + ' virtual ' + signature + '; };')
    text = 'namespace ' + name + ' {\n' + '\n'.join(types + declarations + list(methods.values()))
    adapter_type = 'TRmgMapInterface' if reference else 'type_random_map'
    initializer = ', m_size(m_adapter->getSize())' if member_initialization else ''
    # Record the full unsigned count without allocating for signed dimension
    # conversions. Materialize only bounded validity bits: the actual cell's
    # other fields are deliberately uninitialized and must never be inspected.
    text += '''
struct PackedCells {
    unsigned count, calls, defaultInitialized;
    std::vector<unsigned char> validity;
    PackedCells() : count(0), calls(0), defaultInitialized(9) {}
    void resize(unsigned size, const TRmgPackedTerrainCell& cell) {
        ++calls; count = size; defaultInitialized = cell.m_initialized;
        if (size <= 4096) validity.resize(size, cell.m_initialized);
        else validity.clear();
    }
};
'''
    getters = '\n'.join(extract(terrain, 'rmgTerrainPainter::' + method).replace('rmgTerrainPainter::', '')
                        for method in ('getWidth', 'getHeight'))
    text += '\nstruct Painter { ' + adapter_type + '* m_adapter; TRmgGridPoint m_size; PackedCells m_packedCells; ' + getters + '\n' + resize_helper + '\nPainter(' + adapter_type + '* adapter) : m_adapter(adapter)' + initializer + ' {' + painter + '} };\n'
    if reference:
        text += '''struct Alternate : type_random_map {
    TRmgGridPoint alternate;
    int queries;
    virtual TRmgGridPoint& getSize(TRmgGridPoint& output) {
        ++queries;
        output.m_x = 991; output.m_y = 997; return alternate;
    }
};
'''
    query = ('TRmgGridPoint output; TRmgGridPoint& actual = map.getSize(output);\n'
             'if (&actual != &output) return false;') if reference else 'TRmgGridPoint actual = map.getSize();'
    text += '''bool check() {
    const int values[] = {(-2147483647 - 1), -1, 0, 1, 36, 72, 144, 2147483647};
    for (unsigned i = 0; i < 8; ++i) for (unsigned j = 0; j < 8; ++j) {
        type_random_map map; map.m_mapWidth = values[i]; map.m_mapHeight = values[j];
        const unsigned x = static_cast<unsigned>(values[i]);
        const unsigned y = static_cast<unsigned>(values[j]);
''' + query + '''
        if (actual.m_x != x || actual.m_y != y) return false;
        TRmgMapAdapter adapter; adapter.m_map = &map;
        TRmgRoadMapAdapter road; road.m_map = &map;
        TRmgGridPoint a = adapter.getSize(), b = road.getSize();
        Painter painter(&map);
        if (painter.m_packedCells.count != x * y || painter.m_packedCells.calls != 1
            || painter.m_packedCells.defaultInitialized != 0) return false;
        for (unsigned k = 0; k < painter.m_packedCells.validity.size(); ++k)
            if (painter.m_packedCells.validity[k] != 0) return false;
        map.m_mapWidth = 7; map.m_mapHeight = 11;
        if (a.m_x != x || a.m_y != y || b.m_x != x || b.m_y != y
            || painter.m_size.m_x != x || painter.m_size.m_y != y) return false;
'''
    if reference:
        text += '''        Alternate different; different.queries = 0; different.alternate = TRmgGridPoint(x, y);
        adapter.m_map = &different; road.m_map = &different;
        a = adapter.getSize(); b = road.getSize(); Painter alternatePainter(&different);
        if (different.queries != 3) return false;
        if (alternatePainter.m_packedCells.count != x * y
            || alternatePainter.m_packedCells.calls != 1
            || alternatePainter.m_packedCells.defaultInitialized != 0) return false;
        different.alternate.m_x = 7; different.alternate.m_y = 11;
        if (a.m_x != x || a.m_y != y || b.m_x != x || b.m_y != y
            || alternatePainter.m_size.m_x != x || alternatePainter.m_size.m_y != y) return false;
'''
    text += '    }\n'
    if resize_helper:
        text += '''
    type_random_map small; small.m_mapWidth = 2; small.m_mapHeight = 3;
    Painter p(&small);
    p.m_packedCells.validity[0] = 1;
    TRmgGridPoint size(3, 4); p.resize(size);
    size.m_x = 99; size.m_y = 101;
    if (p.m_size.m_x != 3 || p.m_size.m_y != 4 || p.m_packedCells.count != 12
        || p.m_packedCells.validity[0] != 1) return false;
    for (unsigned k = 1; k < 12; ++k) if (p.m_packedCells.validity[k] != 0) return false;
    p.resize(TRmgGridPoint(1, 2));
    if (p.m_packedCells.count != 2 || p.m_packedCells.validity.size() != 2
        || p.m_packedCells.validity[0] != 1) return false;
    p.resize(TRmgGridPoint(0, 0));
    if (p.m_size.m_x || p.m_size.m_y || p.m_packedCells.count
        || !p.m_packedCells.validity.empty() || p.m_packedCells.calls != 4) return false;
'''
    text += '    return true;\n}\n}\n'
    return text, reference


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path)
    args = parser.parse_args()
    names = ('include/rmg.h', 'src/rmg.cpp', 'src/rmg_terrain.cpp', 'include/rmg_terrain.h')
    states = [{name: (HOMM3_DIR/name).read_text() for name in names}]
    if args.manifest:
        _, originals, axes = source_families.load_manifest(args.manifest, HOMM3_DIR)
        states += [dict(states[0], **source_families.render(originals, axes, (i,))) for i in range(len(axes[0].options))]
    distinct = {tuple(state[name] for name in names): state for state in states}.values()
    cpp = '#include <cstdio>\n#include <vector>\n#define VA(address, size)\n'
    checks = []
    controls = 0
    for index, state in enumerate(distinct):
        name = 'Candidate' + str(index)
        code, reference = program(state, name)
        cpp += code
        checks.append('if (!' + name + '::check()) return 1;')
        for broken in (('dimensions', 'returned_reference', 'painter_reference', 'extra_query', 'storage_count', 'default_cell') if reference else ('dimensions', 'storage_count', 'default_cell')):
            name = 'Wrong' + str(controls)
            code, _ = program(state, name, broken)
            cpp += code
            checks.append('if (' + name + '::check()) return 2;')
            controls += 1
    cpp += '\nint main() { ' + '\n'.join(checks) + '\nreturn 0; }\n'
    with tempfile.TemporaryDirectory(prefix='rmg-size-output-oracle-') as directory:
        path = Path(directory)/'oracle.cpp'; path.write_text(cpp)
        binary = Path(directory)/'oracle'
        subprocess.run(['g++', '-std=c++98', '-O1', str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print(json.dumps(dict(models=len(distinct), dimension_pairs=64, rejected_controls=controls,
        coverage='actual size/default-cell/getter/resize bodies; returned-reference snapshots; unsigned storage counts; bounded validity growth/shrink')))


if __name__ == '__main__':
    main()
