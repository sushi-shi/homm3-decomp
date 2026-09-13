#!/usr/bin/env python3
"""Test a provisional ordinary zone-count decrement boundary.

Retail removeObject B8 directly decrements one zone-owned count in memory.
Direct counter expressions and zone/vector ownership retain a split update.
An ordinary zone method is a source-boundary hypothesis, not a recovered
Dreamcast name. Test the real enum/int index and copied/borrowed arguments,
with its definition among zone methods or beside the removal caller. No
inline keyword, diagnostic pragma, layout change or discarded dummy call.
The unchanged flattened caller is the negative control for this hypothesis.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    original = generator('generate-rmg-object-removal-family.py').definition(source)
    update = ('            int* counts = m_zones[zone]->m_objectCountByType;\n'
              '            --counts[objectType];')
    if original.count(update) != 1:
        raise ValueError('review changed count update')
    caller = original.replace(update, '            m_zones[zone]->decrementObjectCount(objectType);')
    options = [dict(name='flattened', replace=original)]
    for kind, location in itertools.product(
            ('TAdventureObjectType', 'const TAdventureObjectType&', 'int', 'const int&'),
            ('zone_methods', 'caller')):
        declaration = '    void decrementObjectCount(' + kind + ' objectType);\n'
        helper = ('// Provisional ordinary count helper: removeObject 0x54bc50 has no\n'
                  '// retained call here; the flattened control is 98.4000%.\n'
                  'void TRmgZone::decrementObjectCount(' + kind + ' objectType)\n'
                  '{\n    --m_objectCountByType[objectType];\n}\n\n')
        anchor = ('// FilterZonePositions calls this predicate at 0x53b4b7 and 0x53b5ae.'
                  if location == 'zone_methods' else
                  '// Complete-only removal helper; callers retain ownership of the object.')
        options.append(dict(name=kind + '+' + location, replace=caller, extra_edits=[
            dict(source='include/rmg.h', insert_before='    void chooseTerrain();', text=declaration),
            dict(source='src/rmg.cpp', insert_before=anchor, text=helper)]))
    payload = dict(schema=1, units=generator('generate-rmg-map-accessor-family.py').UNITS,
                   evidence=__doc__, axes=[dict(name='removal_count_helper', source='src/rmg.cpp',
                                              find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'ordinary-helper controls across seven consumers')


if __name__ == '__main__':
    main()
