#!/usr/bin/env python3
"""Distinguish signed 32-bit int/long RMG counter ownership under VC6.

The zone constructor clears 232 dwords; 0x546190 uses signed JGE for both
map and zone limits. This excludes unsigned and narrow counters, but does
not distinguish Win32 int from long. Removal 0x54bc50 still splits the zone
decrement after recovering its other instruction streams. Test only the
two signed types, preserving member extents/offsets and updating the one
authored borrowed count pointer atomically. Score all seven header consumers.
These are target-ABI compiler probes; host LP64 long is not a layout oracle.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    zone = '    int m_objectCountByType[232];      // +0x44'
    global_counts = '    int m_objectCountByType[232];                      // +0x1110'
    pointer = '            int* counts = m_zones[zone]->m_objectCountByType;'
    if header.count(zone) != 1 or header.count(global_counts) != 1:
        raise ValueError('review changed counter declarations')
    if (HOMM3_DIR / 'src/rmg.cpp').read_text().count(pointer) != 1:
        raise ValueError('review changed borrowed counter pointer')
    axes = [dict(name='zone_count_type', source='include/rmg.h', find=zone,
                 options=[dict(name='int'), dict(name='long', replace=zone.replace('int ', 'long ', 1),
                     extra_edits=[dict(source='src/rmg.cpp', find=pointer,
                                       replace=pointer.replace('int*', 'long*', 1))])]),
            dict(name='map_count_type', source='include/rmg.h', find=global_counts,
                 options=[dict(name='int'), dict(name='long', replace=global_counts.replace('int ', 'long ', 1))])]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1,
        units=generator('generate-rmg-map-accessor-family.py').UNITS, evidence=__doc__, axes=axes), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print('4 signed counter type states across seven consumers')


if __name__ == '__main__':
    main()
