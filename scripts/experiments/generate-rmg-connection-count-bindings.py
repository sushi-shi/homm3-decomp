#!/usr/bin/env python3
"""Recover the two expanded connection-count loops inside filterZonePositions.

Retail 0x53b2f0 agrees with the current 110-block CFG and retains the size
calls in both counting passes. It binds destination-slot/index loads and
the spilled zone-vector receiver differently. Preserve the canonical
ordinary counting helper and live vector sizes; test its real slot/vector
receivers, destination-object/scalar bindings and guarded/continue flow.
Do not paste the helper into either caller or modify the library bodies.
Neither this Complete-only filter nor its helper has a Dreamcast counterpart.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    prefix = original[:original.index('\n{\n') + 3]
    for receiver, destination, scalar, flow in itertools.product(
            ('slot_pointer', 'slot_reference', 'vector_reference'),
            ('direct', 'pointer', 'reference'), ('int', 'const int', 'const int&'),
            ('guard', 'continue')):
        lines = ['    int result = 0;\n']
        if receiver == 'slot_pointer':
            lines.append('    TRmgTownSlot* slot = zone->m_slot;\n')
            vector = 'slot->m_connections'
        elif receiver == 'slot_reference':
            lines.append('    const TRmgTownSlot& slot = *zone->m_slot;\n')
            vector = 'slot.m_connections'
        else:
            lines.append('    const std::vector<TRmgZoneConnection>& connections = zone->m_slot->m_connections;\n')
            vector = 'connections'
        lines.append('    for (int connection = 0; connection < ' + vector + '.size(); ++connection) {\n')
        target = vector + '[connection].m_destination'
        if destination == 'pointer':
            lines.append('        TRmgTownSlot* destinationSlot = ' + target + ';\n')
            index = 'destinationSlot->m_zoneIndex'
        elif destination == 'reference':
            lines.append('        const TRmgTownSlot& destinationSlot = *' + target + ';\n')
            index = 'destinationSlot.m_zoneIndex'
        else:
            index = target + '->m_zoneIndex'
        lines.append('        ' + scalar + ' destination = ' + index + ';\n')
        if flow == 'guard':
            lines.append('        if (destination < m_zones.size() && m_zones[destination]->canConnect(zone))\n            ++result;\n')
        else:
            lines.append('        if (destination >= m_zones.size())\n            continue;\n'
                         '        if (m_zones[destination]->canConnect(zone))\n            ++result;\n')
        lines.append('    }\n    return result;\n}')
        yield '+'.join((receiver, destination, scalar, flow)), prefix + ''.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::countPlacedZoneConnections')
    axis = helper.axis('connection_count_bindings', 'src/rmg.cpp', original, variants(original))
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'connection-count receiver/binding controls')


if __name__ == '__main__':
    main()
