#!/usr/bin/env python3
"""Consumed zone accessor results and constructed seeds across two search phases.

Retail snapshots bounds/level position before seed selection, then rereads the
level position after the first flood. Preserve those snapshots and timing while
testing the actual existing getters and the visible position constructor. Cross
raw and six-bit-projected terrain reads: the masked parent keeps this in EDI
and introduces a zone-latch block which reset-owner controls did not explain.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    extract = generator('generate-rmg-position-family.py').definition
    original = extract(source, 'type_random_map_generator::buildZoneConnectionPaths')
    read = 'unsigned terrain = current->m_tile.m_landType;'
    snapshots = (
        ('TRmgZoneBounds bounds = zone->m_bounds;',
         'TRmgZoneBounds bounds = zone->getBounds();'),
        ('TRmgMapPosition position = zone->m_levelPosition;',
         'TRmgMapPosition position = zone->getLevelPosition();'),
        ('pathPosition = zone->m_levelPosition;',
         'pathPosition = zone->getLevelPosition();'),
    )
    seed = '''                            seed.m_x = x;
                            seed.m_y = pathPosition.m_y;
                            seed.m_z = position.m_z;'''
    assert original.count(read) == original.count(seed) == 1
    for old, _new in snapshots:
        assert original.count(old) == 1, old
    options = []
    for mask, accessors, construct in itertools.product(range(2), repeat=3):
        body = original
        if mask:
            body = body.replace(read, 'unsigned terrain = current->m_tile.m_landType & 0x3f;')
        if accessors:
            for old, new in snapshots:
                body = body.replace(old, new)
        if construct:
            body = body.replace(seed, '                            seed = TRmgMapPosition(x, pathPosition.m_y, position.m_z);')
        options.append(dict(name=f'mask_{mask}+accessors_{accessors}+seed_ctor_{construct}', replace=body))
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain'], evidence=__doc__,
                   axes=[dict(name='connection_snapshots', source='src/rmg.cpp', find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, originals, axes = load_manifest(args.output, HOMM3_DIR)
    assert render(originals, axes, (0,))['src/rmg.cpp'] == source
    print('eight projection/accessor-result/constructed-seed states')


if __name__ == '__main__':
    main()
