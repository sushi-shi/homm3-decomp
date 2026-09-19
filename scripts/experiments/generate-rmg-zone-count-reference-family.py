#!/usr/bin/env python3
"""Joint zone-count lvalue ownership at registration, removal and limit reads.

Retail increments/decrements zone counts directly in memory; removal's global
count instead loads/decrements/stores. Test one ordinary reference-returning
zone accessor at all three consumers. The fixed array and global accesses are
unchanged. No Dreamcast name, inline qualifier or retained VA is inferred.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator

FUNCTIONS = ('type_random_map_generator::addObject',
             'type_random_map_generator::removeObject',
             'type_random_map_generator::createTreasureObject')
DECLARATION = '    int& objectCount(int objectType);\n'
HELPER = '''int& TRmgZone::objectCount(int objectType)
{
    return m_objectCountByType[objectType];
}

'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    extract = generator('generate-rmg-position-family.py').definition
    bodies = [extract(source, name) for name in FUNCTIONS]
    accesses = ('m_zones[zoneIndex]->m_objectCountByType[objectType]',
                'm_zones[zone]->m_objectCountByType[objectType]',
                'zone->m_objectCountByType[objectType]')
    edited = []
    for body, old in zip(bodies, accesses):
        assert body.count(old) == 1, old
        edited.append(body.replace(old, old.replace('m_objectCountByType[objectType]', 'objectCount(objectType)')))
    extra = [dict(source='src/rmg.cpp', find=old, replace=new)
             for old, new in zip(bodies[1:], edited[1:])]
    extra += [dict(source='include/rmg.h', insert_before='    int getTerrain() const\n', text=DECLARATION),
              dict(source='src/rmg.cpp', insert_before='// Both the level-occupancy pass and the bounds pass in FilterZonePositions\n', text=HELPER)]
    options = [dict(name='direct_zone_array', replace=bodies[0]),
               dict(name='ordinary_zone_count_lvalue', replace=edited[0], extra_edits=extra)]
    payload = dict(schema=1, units=generator('generate-rmg-map-accessor-family.py').UNITS,
                   evidence=__doc__, axes=[dict(name='zone_count_owner', source='src/rmg.cpp', find=bodies[0], options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, originals, axes = load_manifest(args.output, HOMM3_DIR)
    assert render(originals, axes, (0,)) == originals
    print('two whole-contract zone-count states; all three consumers and seven header consumers')


if __name__ == '__main__':
    main()
