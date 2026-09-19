#!/usr/bin/env python3
"""Actual three consumer bodies against existing independent native fixtures.

Reuse the registration fixed-point, removal destination-cell enumerator and
treasure selection oracle, without modifying their owning files. Reduced host
owners model behavior, not retail layout/EH. Counter arithmetic is bounded;
all 232 reference identities are checked separately on the actual accessor.
"""
import argparse
import ast
from pathlib import Path
import re
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

EXTRACT = generator('generate-rmg-position-family.py').definition
FAMILY = generator('generate-rmg-zone-count-reference-family.py')


def block(text, prefix):
    start = text.index(prefix + ' {')
    return text[start:text.index('\n};', start) + 3]


def field(text, name):
    return next(line.split('//')[0].strip() for line in text.splitlines() if name + ';' in line)


def literal(filename, marker):
    tree = ast.parse((HOMM3_DIR / 'scripts/experiments' / filename).read_text())
    matches = [n.value for n in ast.walk(tree) if isinstance(n, ast.Constant)
               and isinstance(n.value, str) and marker in n.value]
    assert len(matches) == 1, (filename, marker, len(matches))
    return matches[0]


def common(source, header, removal=False):
    objects = (HOMM3_DIR / 'include/objecttype.h').read_text()
    mapcell = (HOMM3_DIR / 'include/mapcell.h').read_text()
    text = '#include <vector>\n#include <bitset>\n#include <algorithm>\n#include <cstring>\n#include <cstdio>\n#define VA(a,b)\n'
    for name in ('TRmgVector', 'TPoint', 'TRmgMapPosition', 'TRmgMovementCost',
                 'TRmgZoneCellState', 'TRmgGroundTile', 'TRmgGroundTileData', 'TRmgConnectionDecoration'):
        text += block(header, 'struct ' + name) + '\n'
    start = header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    end = header.index('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;') + len('typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;')
    text += header[start:end] + '\n' + EXTRACT(source, 'TRmgMapPosition::TRmgMapPosition') + '\n'
    text += EXTRACT(source, 'TRmgMapPosition::operator+=') + '\n'
    text += block(mapcell, 'enum TAdventureObjectType') + '\n'
    start = objects.index('    struct TPoint {')
    end = objects.index('\n    };', start) + 7
    text += 'struct TObjectType {\n' + objects[start:end] + '\n'
    if removal:
        start = objects.index('    struct TImageInfo {')
        end = objects.index('\n    };', start) + 7
        text += objects[start:end] + '\n'
    names = ('m_objectType', 'm_hasTrigger', 'm_triggerCell')
    if removal:
        names += ('m_subtype', 'm_imageInfo', 'm_passableMask', 'm_triggerMask')
    text += '\n'.join(field(objects, name) for name in names)
    if removal:
        text += '\n' + EXTRACT(objects, 'getWidth') + '\n' + EXTRACT(objects, 'getHeight')
    text += '\n};\nstruct TRmgObjectPropertiesRef { ' + field(header, 'm_prototype') + ' };\n'
    text += 'struct type_object { ' + field(header, 'm_properties') + ' ' + field(header, 'm_position') + ' };\n'
    if removal:
        text += 'struct CObjectType {\n' + EXTRACT(mapcell, 'getBitPos', parameters='unsigned x, unsigned y') + '\n};\n'
    return text


def zone(source):
    accessor = 'int& TRmgZone::objectCount(' in source
    text = 'struct TRmgZone { int m_objectCountByType[232]; ' + (FAMILY.DECLARATION if accessor else '') + ' };\n'
    if accessor:
        text += EXTRACT(source, 'TRmgZone::objectCount') + '\n'
    return text


def registration(source, header):
    text = common(source, header)
    fixture = literal('test_rmg_add_object.py', 'struct TRmgMapItem {')
    # Keep the opaque host boundary consistent with the recovered map contract.
    fixture = fixture.replace('void addObject(type_object* object,TRmgMapPosition position)', 'void addObject(type_object& object,TRmgMapPosition position)').replace('object->', 'object.')
    text += fixture + EXTRACT(header, 'getMapItem', parameters='int x, int y, int z') + '\n};\n'
    text += EXTRACT(source, 'type_random_map::getMapItem', parameters='TRmgMapPosition point') + '\n'
    text += literal('test_rmg_add_object.py', 'struct TRmgGeneratorBase {')
    text += EXTRACT(source, 'TRmgGeneratorBase::addObject') + '\n' + zone(source)
    text += 'struct GeneratorFixture : TRmgGeneratorBase { int m_objectCountByType[232]; std::vector<TRmgZone*> m_zones; };\n'
    start = source.index('TPoint g_rmgDirections[RMG_DIRECTION_COUNT] = {')
    text += 'enum { RMG_DIRECTION_COUNT=8 };\n' + source[start:source.index('\n};', start) + 3] + '\n'
    text += EXTRACT(source, 'insertRmgWorkItem', parameters='std::vector<TRmgMapPosition>& positions, std::vector<int>& costs, TRmgMapPosition position, int cost') + '\n'
    text += literal('test_rmg_add_object.py', 'void reference(GeneratorFixture& owner')
    text += 'struct type_random_map_generator : GeneratorFixture { void addObject(type_object*,TRmgMapPosition); };\n'
    text += EXTRACT(source, FAMILY.FUNCTIONS[0])
    return text + '\nint main(){return check<type_random_map_generator>()?0:1;}\n'


def removal(source, header):
    text = common(source, header, True)
    text += 'struct TRmgMapItem { std::vector<type_object*> m_objects; TRmgZoneCellState m_zoneState; TRmgGroundTileData m_tileData; };\n'
    text += 'struct type_random_map { int m_mapWidth,m_mapHeight; TRmgMapItem* m_mapItems; TRmgMapItem* getMapItem(TRmgMapPosition point);\n'
    text += EXTRACT(header, 'getMapItem', parameters='int x, int y, int z') + '\n};\n'
    text += EXTRACT(source, 'type_random_map::getMapItem', parameters='TRmgMapPosition point') + '\n' + zone(source)
    text += 'struct GeneratorFixture { type_random_map m_map; std::vector<TRmgZone*> m_zones; int m_objectCountByType[232]; std::vector<type_object*> m_positions; std::vector<unsigned char> m_disabledKeyTents; int m_nextKeyTentColor; };\n'
    text += literal('test_rmg_object_removal.py', 'static void removeFirst(')
    text += 'struct type_random_map_generator : GeneratorFixture { void removeObject(type_object*); };\n'
    body = EXTRACT(source, FAMILY.FUNCTIONS[1])
    # Dinkumware's pointer iterators have a null test; host iterators expose base().
    body = body.replace('if (found)', 'if (found.base())').replace('if (entry)', 'if (entry.base())')
    return text + body + '\nint main(){return check<type_random_map_generator>()?0:1;}\n'


def treasure(source, header):
    text = (HOMM3_DIR / 'scripts/experiments/rmg-treasure-create-oracle.cpp').read_text()
    # Wrong-slot controls can read a noncandidate counter. Give the reduced
    # host owner fully initialized storage without changing the game body.
    marker = '        for (int i = 0; i < s.m_count; ++i) {'
    assert text.count(marker) == 1
    text = text.replace(marker, '        for (int i=0;i<232;++i) { root->m_objectCountByType[i]=0; m_zone.m_objectCountByType[i]=0; }\n' + marker)
    objects = (HOMM3_DIR / 'include/objecttype.h').read_text()
    mapcell = (HOMM3_DIR / 'include/mapcell.h').read_text()
    types = '\n'.join(block(header, 'struct ' + name) for name in ('TRmgVector', 'TPoint', 'TRmgMapPosition'))
    accessors = '\n'.join(line.strip() for line in objects.splitlines() if re.match(r'\s*int get(?:Width|Height)\(\) const \{', line))
    body = EXTRACT(source, FAMILY.FUNCTIONS[2])
    signature = body[:body.index('\n{')].replace('type_random_map_generator::', '')
    methods = 'struct type_random_map_generator : CreateRoot { ' + signature + '; };\n' + body
    for marker, value in [('VALUE_TYPES', types), ('VALUE_HELPERS', EXTRACT(source, 'TRmgMapPosition::TRmgMapPosition')),
                          ('ACCESSORS', accessors), ('BIT_POSITION', EXTRACT(mapcell, 'getBitPos', parameters='unsigned x, unsigned y')),
                          ('CANDIDATES', methods), ('CHECKS', 'if(!check<type_random_map_generator>()) return 1;')]:
        text = text.replace('// @' + marker + '@', value)
    if 'int& TRmgZone::objectCount(' in source:
        marker = 'struct TRmgZone { Slot* m_slot; int m_terrain; int m_objectCountByType[232]; };'
        assert text.count(marker) == 1
        text = text.replace(marker, marker[:-2] + FAMILY.DECLARATION + '};\n' + EXTRACT(source, 'TRmgZone::objectCount'))
    assert '// @' not in text
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    _, originals, axes = source_families.load_manifest(args.manifest, HOMM3_DIR)
    models = [source_families.render(originals, axes, (i,)) for i in range(len(axes[0].options))]
    tasks = []
    for i, model in enumerate(models):
        for label, builder in [('registration', registration), ('removal', removal), ('treasure', treasure)]:
            tasks.append((f'{i}-{label}', builder(model['src/rmg.cpp'], model['include/rmg.h']), True))
    model = models[1];src = model['src/rmg.cpp'];header = model['include/rmg.h']
    for label, builder, old, new in [
        ('wrong-registration', registration, '++m_zones[zoneIndex]->objectCount(objectType);', '--m_zones[zoneIndex]->objectCount(objectType);'),
        ('wrong-removal', removal, '--m_zones[zone]->objectCount(objectType);', '++m_zones[zone]->objectCount(objectType);'),
        ('wrong-limit', treasure, 'zone->objectCount(objectType) >=', 'zone->objectCount(objectType) >'),
    ]:
        assert src.count(old) == 1
        tasks.append((label, builder(src.replace(old, new), header), False))
    for label, builder in [('registration', registration), ('removal', removal), ('treasure', treasure)]:
        tasks.append(('wrong-slot-' + label, builder(src.replace('return m_objectCountByType[objectType];', 'return m_objectCountByType[(objectType + 1) % 232];'), header), False))
    identity = '#include <cassert>\n' + zone(src) + '''
int main(){TRmgZone first,second;
 for(int i=0;i<232;++i){first.m_objectCountByType[i]=i;second.m_objectCountByType[i]=-i;}
 for(int i=0;i<232;++i){int& value=first.objectCount(i);assert(&value==&first.m_objectCountByType[i]);
   ++value;assert(first.objectCount(i)==i+1);assert(second.objectCount(i)==-i);--value;}
}'''
    tasks.append(('reference-identity', identity, True))
    with tempfile.TemporaryDirectory(prefix='rmg-zone-count-reference-') as folder:
        for label, text, positive in tasks:
            path = Path(folder) / label;path.with_suffix('.cpp').write_text(text)
            run = subprocess.run(['g++', '-std=c++11', '-O1', '-fsanitize=undefined', '-fno-sanitize-recover=all', str(path.with_suffix('.cpp')), '-o', str(path)], capture_output=True, text=True)
            assert run.returncode == 0, (label, run.stderr[-6000:])
            run = subprocess.run([str(path)], capture_output=True, text=True)
            assert (run.returncode == 0) == positive, (label, run.returncode, run.stdout, run.stderr)
            if positive:
                assert not run.stderr, (label, run.stderr)
    print(f'{len(models)} complete three-consumer models; independent registration/removal/treasure fixtures; 232 reference identities; six wrong controls; UBSan clean')


if __name__ == '__main__':
    main()
