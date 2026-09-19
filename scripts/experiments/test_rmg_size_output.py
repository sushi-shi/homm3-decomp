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
    if broken == 'dimensions':
        methods[owners[0]] = methods[owners[0]].replace('m_mapWidth, m_mapHeight', 'm_mapHeight, m_mapWidth')
    if broken == 'returned_reference':
        assert reference
        for owner in owners[1:]:
            methods[owner] = methods[owner].replace('return m_map->getSize(size);',
                'm_map->getSize(size); return size;')
    if broken == 'extra_query':
        assert reference
        for owner in owners[1:]:
            methods[owner] = methods[owner].replace('return m_map->getSize(size);',
                'm_map->getSize(size); return m_map->getSize(size);')
    start = terrain.index('rmgTerrainPainter::rmgTerrainPainter(')
    start = terrain.index('\n{', start) + 2
    end = terrain.index('    m_packedCells.resize', start)
    painter = terrain[start:end]
    if broken == 'painter_reference':
        assert reference
        painter = painter.replace('m_size = m_adapter->getSize(size);',
            'm_adapter->getSize(size); m_size = size;')
    declarations = []
    for owner in owners:
        signature = methods[owner].split('\n{', 1)[0].replace(owner + '::', '')
        members = 'int m_mapWidth; int m_mapHeight;' if owner == owners[0] else 'type_random_map* m_map;'
        declarations.append('struct ' + owner + ' { ' + members + ' virtual ' + signature + '; };')
    text = 'namespace ' + name + ' {\n' + '\n'.join(types + declarations + list(methods.values()))
    text += '\nstruct Painter { type_random_map* m_adapter; TRmgGridPoint m_size; void readSize() {' + painter + '} };\n'
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
        Painter painter; painter.m_adapter = &map; painter.readSize();
        map.m_mapWidth = 7; map.m_mapHeight = 11;
        if (a.m_x != x || a.m_y != y || b.m_x != x || b.m_y != y
            || painter.m_size.m_x != x || painter.m_size.m_y != y) return false;
'''
    if reference:
        text += '''        Alternate different; different.queries = 0; different.alternate = TRmgGridPoint(x, y);
        adapter.m_map = &different; road.m_map = &different; painter.m_adapter = &different;
        a = adapter.getSize(); b = road.getSize(); painter.readSize();
        if (different.queries != 3) return false;
        different.alternate.m_x = 7; different.alternate.m_y = 11;
        if (a.m_x != x || a.m_y != y || b.m_x != x || b.m_y != y
            || painter.m_size.m_x != x || painter.m_size.m_y != y) return false;
'''
    text += '    }\n    return true;\n}\n}\n'
    return text, reference


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path)
    args = parser.parse_args()
    names = ('include/rmg.h', 'src/rmg.cpp', 'src/rmg_terrain.cpp')
    states = [{name: (HOMM3_DIR/name).read_text() for name in names}]
    if args.manifest:
        _, originals, axes = source_families.load_manifest(args.manifest, HOMM3_DIR)
        states += [source_families.render(originals, axes, (i,)) for i in range(len(axes[0].options))]
    distinct = {tuple(state[name] for name in names): state for state in states}.values()
    cpp = '#include <cstdio>\n#define VA(address, size)\n'
    checks = []
    controls = 0
    for index, state in enumerate(distinct):
        name = 'Candidate' + str(index)
        code, reference = program(state, name)
        cpp += code
        checks.append('if (!' + name + '::check()) return 1;')
        for broken in (('dimensions', 'returned_reference', 'painter_reference', 'extra_query') if reference else ('dimensions',)):
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
        coverage='actual size bodies, both adapter copies, painter query, returned-reference alias source')))


if __name__ == '__main__':
    main()
