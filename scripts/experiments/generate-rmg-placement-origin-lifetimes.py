#!/usr/bin/env python3
"""Prepare canPlaceObject coordinate ownership and first failure-join probes.

Retail 0x531cf0 keeps placement X/Y in EDI/EBX across its three retained
helpers, then reads trigger X/Y and translates the entrance. The current
candidate reverses those register homes. Retail also shares the passability
and rock failure epilogue: 23 blocks against the candidate's 24. canFitObject
was closed by jointly copying the prototype's own trigger point and retaining
a translated vector; apply that hypothesis to this distinct caller, without
assuming that its compiler result transfers. No Dreamcast counterpart is mapped.

Trigger reads stay after the helper calls and hasTrigger test. Only immutable
by-value placement coordinates may be captured earlier. Preserve the canonical
map-position lookup, original helper arguments, zone tests and water predicate.
This manifest is an unscored hypothesis until the VC6 evidence pass and search
run successfully; rendering and source checks do not establish retail matching.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    old = ('    int x = position.m_x - prototype.m_triggerCell.m_x;\n'
           '    int y = position.m_y - prototype.m_triggerCell.m_y;\n'
           '    ++y;\n'
           '    TRmgMapPosition entrance(x, y, position.m_z);\n'
           '    if (y >= m_size.m_y)\n'
           '        return 0;\n'
           '    TRmgMapItem* item = getMapItem(entrance);\n')
    separate = ('    if (!item->m_tileData.m_roadPassable)\n        return 0;\n'
                '    if (item->m_tile.m_landType == eTerrainRock)\n        return 0;\n')
    prototype = '    TObjectType& prototype = *properties->m_prototype;\n'
    for anchor in (old, separate, prototype):
        if original.count(anchor) != 1:
            raise ValueError('review changed placement source before rebasing')
    yield 'current', original
    for storage, trigger, early, combined in itertools.product(
            ('vector', 'point', 'position', 'scalars'), ('fields', 'copy', 'reference'),
            (False, True), (False, True)):
        declarations = {
            'vector': '    TRmgVector origin(position.m_x, position.m_y);\n',
            'point': '    TPoint origin(position.m_x, position.m_y);\n',
            'position': '    TRmgMapPosition entrance = position;\n',
            'scalars': '    int x = position.m_x;\n    int y = position.m_y;\n',
        }
        x, y = ('x', 'y') if storage == 'scalars' else (
            ('entrance.m_x', 'entrance.m_y') if storage == 'position' else ('origin.m_x', 'origin.m_y'))
        tx, ty = 'prototype.m_triggerCell.m_x', 'prototype.m_triggerCell.m_y'
        setup = ''
        if trigger != 'fields':
            kind = 'TObjectType::TPoint' if trigger == 'copy' else 'const TObjectType::TPoint&'
            setup = '    ' + kind + ' trigger = prototype.m_triggerCell;\n'
            tx, ty = 'trigger.m_x', 'trigger.m_y'
        if not early:
            setup += declarations[storage]
        setup += '    ' + x + ' -= ' + tx + ';\n    ' + y + ' -= ' + ty + ';\n    ++' + y + ';\n'
        if storage != 'position':
            setup += '    TRmgMapPosition entrance(' + x + ', ' + y + ', position.m_z);\n'
        setup += ('    if (' + y + ' >= m_size.m_y)\n        return 0;\n'
                  '    TRmgMapItem* item = getMapItem(entrance);\n')
        body = original.replace(old, setup)
        if early:
            body = body.replace(prototype, prototype + declarations[storage])
        if combined:
            body = body.replace(separate,
                '    if (!item->m_tileData.m_roadPassable || item->m_tile.m_landType == eTerrainRock)\n'
                '        return 0;\n')
        yield '+'.join((storage, trigger, 'early' if early else 'late',
                        'combined' if combined else 'separate')), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'type_random_map::canPlaceObject')
    axis = helper.axis('placement_origin_lifetimes', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, originals, axes = source_families.load_manifest(args.output, HOMM3_DIR)
    if source_families.render(originals, axes, (0,)) != originals:
        raise ValueError('unchanged-source control did not render unchanged')
    print(len(axis['options']), 'unscored placement origin/failure-join states')


if __name__ == '__main__':
    main()
