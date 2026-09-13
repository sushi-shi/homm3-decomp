#!/usr/bin/env python3
"""Refine the verified 99.7061% filter parent using real pass lifetimes.

The parent's assigned position fixes maximum-X addition. Retail still
schedules the bound-loop initialization and final counter reload differently;
both count loops retain a reversed element-address SIB. Keep the borrowed
count index and canonical min/max calls. Vary boundaries between level
preference, connection ranking and bounds accumulation, plus the actual
bound-loop counter declaration/type. No padding operations or fake helpers.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def checked_parent(context):
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    if checkpoint.get('generation', 0) < 1:
        raise ValueError('parent search is unfinished')
    for relative in ('src/rmg.cpp', 'include/rmg.h'):
        if (context / 'snapshot' / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError('stale parent snapshot: ' + relative)
    entry = next(item for item in checkpoint['elites']
                 if item['labels']['filter_bound_values'] == 'assigned+value_wrapper+scalars+position_first')
    folder = context / 'candidates' / entry['id']
    results = [json.loads((folder / which / 'result.json').read_text()) for which in ('first', 'repeat')]
    if results[0]['object_hash'] != results[1]['object_hash'] or results[0]['object_hash'] != entry['object_hash']:
        raise ValueError('parent object identity does not reproduce')
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    files = render(originals, axes, tuple(entry['choices']))
    if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
        raise ValueError('rendered parent does not match the reproduced source')
    return originals['src/rmg.cpp'], files['src/rmg.cpp']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    source, parent_source = checked_parent(args.parent)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::filterZonePositions'
    original, parent = helper.definition(source, name), helper.definition(parent_source, name)
    count_name = 'type_random_map_generator::countPlacedZoneConnections'
    count_original, count_parent = helper.definition(source, count_name), helper.definition(parent_source, count_name)
    options = [dict(name='baseline', replace=original)]
    for scope, bounds_scope, counter_type, declaration in itertools.product(
            ('original', 'later_count', 'counting', 'level_and_count'),
            ('original', 'block'), ('int', 'unsigned int', 'long'), ('for', 'before')):
        body = parent
        if scope in ('later_count', 'counting'):
            body = body.replace('    int bestConnections = 0;\n', '', 1)
            start = body.index('    for (int candidate = 0;')
            body = body[:start] + '    int bestConnections = 0;\n' + body[start:]
        if scope in ('counting', 'level_and_count'):
            start = body.index('    int bestConnections = 0;')
            end = body.index('    int bestSize = 32000;', start)
            body = body[:start] + '    {\n' + body[start:end] + '    }\n' + body[end:]
            boundary = body.index('    int size = zone->getSize();')
            body = body[:boundary] + body[boundary:].replace('    for (candidate = 0;', '    for (int candidate = 0;', 1)
        start = body.index('    for (int other = 0;', body.index('    int bestSize = 32000;'))
        end = body.index('    int size = zone->getSize();', start)
        loop = body[start:end]
        if declaration == 'before':
            loop = loop.replace('    for (int other = 0;', '    ' + counter_type + ' other;\n    for (other = 0;', 1)
        else:
            loop = loop.replace('    for (int other = 0;', '    for (' + counter_type + ' other = 0;', 1)
        if bounds_scope == 'block':
            loop = '    {\n' + loop + '    }\n'
        body = body[:start] + loop + body[end:]
        options.append(dict(name='+'.join((scope, bounds_scope, counter_type, declaration)), replace=body,
                            extra_edits=[dict(source='src/rmg.cpp', find=count_original, replace=count_parent)]))
    axis = dict(name='filter_phase_scopes', source='src/rmg.cpp', find=original, options=options)
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'filter pass/counter lifetime controls')


if __name__ == '__main__':
    main()
