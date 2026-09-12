#!/usr/bin/env python3
"""Project connection terrain to retail's six unsigned bits with real lifetimes.

Retail 0x5405d0 +0x131 masks the terrain field, unlike the signed six-bit
getter used by other callers. The previous seven masked expressions changed
code generation substantially. Cross projection ownership and declaration
scope, preserving the signed field, ordered queries, helpers and loop exits.
This Complete-only method has no Dreamcast counterpart. No inline pins.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

FUNCTION = 'type_random_map_generator::buildZoneConnectionPaths'
READ = '                        unsigned terrain = current->m_tile.m_landType;'


def variants(original):
    assert original.count(READ) == 1
    for scope, kind, owner, query in itertools.product(
            ('guard', 'scan', 'zone', 'function'),
            ('unsigned', 'int', 'unsigned char'),
            ('direct', 'snapshot'), ('field', 'accessor')):
        if owner == 'snapshot' and query == 'accessor':
            # A ground-tile value has no map-cell accessor; do not invent one.
            continue
        expression = 'current->m_tile.m_landType' if query == 'field' else 'current->getLandType()'
        prefix = ''
        if owner == 'snapshot':
            prefix = '                        TRmgGroundTile tile = current->m_tile;\n'
            expression = 'tile.m_landType'
        declaration = kind + ' terrain'
        statement = (declaration if scope == 'guard' else 'terrain') + ' = ' + expression + ' & 0x3f;'
        body = original.replace(READ, prefix + '                        ' + statement)
        if scope != 'guard':
            anchor, indent = {
                'scan': ('                    TRmgMapItem* current = m_map.getMapItem(x,', '                    '),
                'zone': ('        unsigned char found = 0;', '        '),
                'function': ('    int count =', '    '),
            }[scope]
            assert body.count(anchor) == 1
            body = body.replace(anchor, indent + declaration + ';\n' + anchor, 1)
        yield '+'.join((scope, kind, owner, query)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), FUNCTION)
    axis = helper.axis('connection_terrain_lifetimes', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'terrain projection/lifetime controls')


if __name__ == '__main__':
    main()
