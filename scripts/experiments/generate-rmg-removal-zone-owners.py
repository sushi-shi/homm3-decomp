#!/usr/bin/env python3
"""Test signed zone-index and owner lifetimes in the recovered removal body.

The canonical zone field is signed eight bits. Retail sign-extends it, tests
for a negative zone, obtains the zone pointer, and decrements one indexed
count. Current B8 retains a split decrement despite identical surrounding
instructions. Earlier narrow-index and owner-pointer probes preceded the
recovered trigger-copy context. Test the four signed index representations,
copied/const/borrowed results and actual pointer/reference zone owners, while
preserving the counter array, trigger scope and canonical map query.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    query = '        int zone = m_map.getMapItem(position.m_x - trigger.m_x,'
    counts = '            int* counts = m_zones[zone]->m_objectCountByType;'
    if original.count(query) != 1 or original.count(counts) != 1:
        raise ValueError('review changed zone query/count ownership')
    for scalar, ownership, receiver in itertools.product(
            ('int', 'long', 'short', 'signed char'), ('value', 'const', 'reference'),
            ('direct', 'pointer', 'reference', 'pointer_reference')):
        kind = scalar if ownership == 'value' else 'const ' + scalar
        if ownership == 'reference':
            kind += '&'
        body = original.replace(query, query.replace('int zone', kind + ' zone', 1))
        prefix = ''
        array = 'm_zones[zone]->m_objectCountByType'
        if receiver == 'pointer':
            prefix = '            TRmgZone* owner = m_zones[zone];\n'
            array = 'owner->m_objectCountByType'
        elif receiver == 'reference':
            prefix = '            TRmgZone& owner = *m_zones[zone];\n'
            array = 'owner.m_objectCountByType'
        elif receiver == 'pointer_reference':
            prefix = '            TRmgZone*& owner = m_zones[zone];\n'
            array = 'owner->m_objectCountByType'
        body = body.replace(counts, prefix + '            int* counts = ' + array + ';')
        yield '+'.join((scalar, ownership, receiver)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-object-removal-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    axis = generator('generate-rmg-position-family.py').axis(
        'removal_zone_owners', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'signed zone/owner states')


if __name__ == '__main__':
    main()
