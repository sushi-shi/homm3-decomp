#!/usr/bin/env python3
"""Keep the candidate counter alive across scoped filter passes.

The level/count block fixes bounds initialization and reload order at
99.7635%, but its newly separate ranking counter trades stack/parameter
homes. Test a single signed counter outside that block, retaining the
original forward and reverse traversal in both ranking phases. Both int
and long are signed 32-bit types in VC6. Keep the earlier assigned-position
and borrowed-destination parent; no extra expressions or helper calls.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    source, parent_source = generator('generate-rmg-filter-phase-scopes.py').checked_parent(args.parent)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::filterZonePositions'
    original, parent = helper.definition(source, name), helper.definition(parent_source, name)
    count_name = 'type_random_map_generator::countPlacedZoneConnections'
    count_original, count_parent = helper.definition(source, count_name), helper.definition(parent_source, count_name)
    edits = [dict(source='src/rmg.cpp', find=count_original, replace=count_parent)]
    options = [dict(name='baseline', replace=original)]
    start = parent.index('    int bestConnections = 0;')
    end = parent.index('    int bestSize = 32000;', start)
    scoped = parent[:start] + '    {\n' + parent[start:end] + '    }\n' + parent[end:]
    boundary = scoped.index('    int size = zone->getSize();')
    scoped = scoped[:boundary] + scoped[boundary:].replace('    for (candidate = 0;', '    for (int candidate = 0;', 1)
    options.append(dict(name='separate_counter_scoped_control', replace=scoped, extra_edits=edits))
    for kind, initialized in itertools.product(('int', 'long'), ('before_levels', 'before_count')):
        body = scoped.replace('    {\n', '    ' + kind + ' candidate;\n    {\n', 1)
        body = body.replace('    for (int candidate = 0;', '    for (candidate = 0;')
        if initialized == 'before_count':
            body = body.replace('    int bestConnections = 0;\n', '', 1)
            first = body.index('    for (candidate = 0;')
            body = body[:first] + '    int bestConnections = 0;\n' + body[first:]
        options.append(dict(name=kind + '+' + initialized, replace=body, extra_edits=edits))
    axis = dict(name='filter_shared_counter', source='src/rmg.cpp', find=original, options=options)
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'shared/separate filter-counter controls')


if __name__ == '__main__':
    main()
