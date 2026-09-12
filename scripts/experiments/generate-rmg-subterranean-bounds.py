#!/usr/bin/env python3
"""Recover subterranean-gate bounds selection and temporary ownership.

Retail 0x542080 repeatedly copies both min/max operands into shared stack
homes before reference selection, matching the recovered value wrappers in
homm3_minmax.h. The current direct STL selectors retain references to the
bound records. Cross that interface with actual copied-bound construction,
the destination snapshot's lifetime, initial zone-query order and the later
position value's construction. Keep both bound snapshots across callbacks,
all coordinate helpers and the canonical vector/guard operations.
No Dreamcast counterpart exists for this Complete-only function.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

FUNCTION = 'type_random_map_generator::createSubterraneanGate'


def variants(original):
    # Keep the authored form as the manifest baseline; reconstruct the
    # measured historical control after adopting the projection parent.
    if '    {\n        TRmgZoneBounds destinationBounds' in original:
        start = original.index('    {\n        TRmgZoneBounds destinationBounds')
        end = original.index('    }\n    if (sourceBounds.m_minimumX', start)
        inner = original[start + len('    {\n'):end]
        original = original[:start] + ''.join(line[4:] for line in inner.splitlines(keepends=True)) + original[end + len('    }\n'):]
    for selector in ('min', 'max'):
        original = original.replace(' = ' + selector + '(\n', ' = std::_cpp_' + selector + '(\n')
    original = original.replace('    TRmgMapPosition position;\n    position = source->getLevelPosition();',
                                '    TRmgMapPosition position = source->getLevelPosition();')
    original = original.replace(
        '            TRmgMapPosition otherPosition;\n            otherPosition.m_x = position.m_x;\n'
        '            otherPosition.m_y = position.m_y;\n            otherPosition.m_z = destination->getLevelPosition().m_z;',
        '            TRmgMapPosition otherPosition = destination->getLevelPosition();\n'
        '            otherPosition.m_x = position.m_x;\n            otherPosition.m_y = position.m_y;')
    original = original.replace(
        '    TRmgMapPosition otherPosition = position;\n    otherPosition.m_z = destination->getLevelPosition().m_z;',
        '    TRmgMapPosition otherPosition = destination->getLevelPosition();\n'
        '    otherPosition.m_x = position.m_x;\n    otherPosition.m_y = position.m_y;')
    for selectors, construction, scope, order, position in itertools.product(
            ('reference_selectors', 'value_wrappers'), ('copy', 'assigned', 'components'),
            ('function', 'intersection'), ('destination_first', 'source_first'),
            ('copy', 'assigned')):
        body = original
        if selectors == 'value_wrappers':
            assert body.count('std::_cpp_min(') == body.count('std::_cpp_max(') == 2
            body = body.replace('std::_cpp_min(', 'min(').replace('std::_cpp_max(', 'max(')
        if construction != 'copy':
            for name, owner in (('sourceBounds', 'source'), ('destinationBounds', 'destination')):
                old = '    TRmgZoneBounds ' + name + ' = ' + owner + '->m_bounds;\n'
                new = '    TRmgZoneBounds ' + name + ';\n'
                if construction == 'assigned':
                    new += '    ' + name + ' = ' + owner + '->m_bounds;\n'
                else:
                    for field in ('m_minimumX', 'm_minimumY', 'm_maximumX', 'm_maximumY'):
                        new += '    ' + name + '.' + field + ' = ' + owner + '->m_bounds.' + field + ';\n'
                assert body.count(old) == 1
                body = body.replace(old, new)
        if scope == 'intersection':
            start = body.index('    TRmgZoneBounds destinationBounds')
            end = body.index('    if (sourceBounds.m_minimumX >=', start)
            body = body[:start] + '    {\n' + ''.join('    ' + line for line in body[start:end].splitlines(keepends=True)) + '    }\n' + body[end:]
        if order == 'source_first':
            old = ('    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];\n'
                   '    int sourceZone = source->m_slot->m_zoneIndex;\n')
            new = ('    int sourceZone = source->m_slot->m_zoneIndex;\n'
                   '    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];\n')
            assert body.count(old) == 1
            body = body.replace(old, new)
        if position == 'assigned':
            old = '    TRmgMapPosition position = source->getLevelPosition();'
            new = '    TRmgMapPosition position;\n    position = source->getLevelPosition();'
            assert body.count(old) == 1
            body = body.replace(old, new)
        yield '+'.join((selectors, construction, scope, order, position)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), FUNCTION)
    axis = helper.axis('subterranean_bounds', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'subterranean bounds controls')


if __name__ == '__main__':
    main()
