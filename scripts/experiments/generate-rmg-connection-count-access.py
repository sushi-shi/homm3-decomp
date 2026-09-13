#!/usr/bin/env python3
"""Refine the reproduced borrowed-index connection-count parent.

The minimal const-int-reference parent fixes both expanded loops' receiver
loads and rises to 99.6892%, retaining all 110 retail blocks. Its remaining
element-address SIB operands are reversed. Compare standard vector indexed
and begin-plus-index access with named element pointers/references, keeping
the live size queries, scalar borrowing and ordinary helper boundary.
Require the measured parent source, input option and repeat object identity.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def checked_parent(context):
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::countPlacedZoneConnections'
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    original = helper.definition(source, name)
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    manifest = json.loads((context / 'input.json').read_text())
    if checkpoint.get('generation', 0) < 1:
        raise ValueError('parent search is unfinished')
    for relative in ('src/rmg.cpp', 'include/rmg.h'):
        if (context / 'snapshot' / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError('stale parent snapshot: ' + relative)
    parent_entry = next(entry for entry in checkpoint['elites']
                        if entry['labels']['connection_count_bindings'] == 'slot_pointer+direct+const int&+guard')
    candidate = context / 'candidates' / parent_entry['id']
    results = [json.loads((candidate / which / 'result.json').read_text()) for which in ('first', 'repeat')]
    if results[0]['object_hash'] != results[1]['object_hash'] or results[0]['object_hash'] != parent_entry['object_hash']:
        raise ValueError('parent object identity does not reproduce')
    parent = helper.definition((candidate / 'repeat/tree/src/rmg.cpp').read_text(), name)
    if manifest['axes'][0]['find'] != original or manifest['axes'][0]['options'][parent_entry['choices'][0]]['replace'] != parent:
        raise ValueError('parent input/body mismatch')
    return original, parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original, parent = checked_parent(args.parent)
    old = '        const int& destination = slot->m_connections[connection].m_destination->m_zoneIndex;\n'
    if parent.count(old) != 1:
        raise ValueError('review the borrowed-index parent')
    forms = [('borrowed_index_parent', parent)]
    for access, element, slot in itertools.product(
            ('subscript', 'begin_index', 'index_begin'),
            ('direct', 'pointer', 'reference'), ('pointer', 'reference')):
        item = {'subscript': 'slot->m_connections[connection]',
                'begin_index': '*(slot->m_connections.begin() + connection)',
                'index_begin': '*(connection + slot->m_connections.begin())'}[access]
        if element == 'pointer':
            lines = '        const TRmgZoneConnection* currentConnection = &(' + item + ');\n'
            target = 'currentConnection->m_destination'
        elif element == 'reference':
            lines = '        const TRmgZoneConnection& currentConnection = ' + item + ';\n'
            target = 'currentConnection.m_destination'
        else:
            lines = ''
            target = '(' + item + ').m_destination'
        lines += '        const int& destination = ' + target + '->m_zoneIndex;\n'
        body = parent.replace(old, lines)
        if slot == 'reference':
            body = body.replace('TRmgTownSlot* slot = zone->m_slot;', 'const TRmgTownSlot& slot = *zone->m_slot;').replace('slot->', 'slot.')
        forms.append(('+'.join((access, element, slot)), body))
    axis = helper.axis('connection_count_element_access', 'src/rmg.cpp', original, forms)
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'connection-count element-access controls')


if __name__ == '__main__':
    main()
