#!/usr/bin/env python3
"""Recover the gate scan's source-score value across the destination query.

Retail extracts the first unsigned 16-bit score immediately after source-zone
admission; the current candidate saves the whole packed zone word and masks
it only after the destination query. Test a separate source score and sum,
their actual value types and declaration lifetimes, plus the evidenced source
zone query order and whole/Z-only source-position capture. Preserve both
map queries, signed accumulated score and original candidate tie handling.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    for kind, sum_scope, zone_order, position in itertools.product(
            ('int', 'unsigned int', 'unsigned short', 'const int'),
            ('after_destination', 'before_destination'),
            ('destination_first', 'source_first'), ('whole', 'level', 'borrowed_level')):
        body = original
        old = '            int score = sourceItem->m_zoneState.m_score;'
        new = '            ' + kind + ' sourceScore = sourceItem->m_zoneState.m_score;'
        if sum_scope == 'before_destination':
            new += '\n            int score;'
        assert body.count(old) == 1
        body = body.replace(old, new)
        old = '            score += destinationItem->m_zoneState.m_score;'
        new = '            ' + ('int ' if sum_scope == 'after_destination' else '') + 'score = sourceScore + destinationItem->m_zoneState.m_score;'
        assert body.count(old) == 1
        body = body.replace(old, new)
        if zone_order == 'source_first':
            old = ('    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];\n'
                   '    int sourceZone = source->m_slot->m_zoneIndex;')
            new = ('    int sourceZone = source->m_slot->m_zoneIndex;\n'
                   '    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];')
            assert body.count(old) == 1
            body = body.replace(old, new)
        if position != 'whole':
            old = '    position = source->getLevelPosition();'
            new = '    position.m_z = source->getLevelPosition().m_z;'
            if position == 'borrowed_level':
                new = '    const TRmgMapPosition& sourcePosition = source->getLevelPosition();\n    position.m_z = sourcePosition.m_z;'
            assert body.count(old) == 1
            body = body.replace(old, new)
        yield '+'.join((kind, sum_scope, zone_order, position)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::createSubterraneanGate')
    axis = helper.axis('subterranean_score_values', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'subterranean score-value controls')


if __name__ == '__main__':
    main()
