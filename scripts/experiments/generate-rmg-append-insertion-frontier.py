#!/usr/bin/env python3
"""Test public vector insertion forms at the recovered geometry frontier.

Direct center X/Y stores and a late level computation recover retail's
middle instruction schedule, but the first vector size is expanded again.
The existing VC6 vector push_back delegates to single insert, which in turn
delegates to count insert. Test these public entry points independently at
each append, retaining all ten reproduced setter parents. Explore the
98.1270% setter parent and its independently checked direct-center variant.
Every source form still appends the post-predicate getter result exactly
once and preserves the canonical vector/helper declarations and bodies.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(original, policies):
    anchor = 'candidates.push_back(zone->getLevelPosition());'
    if original.count(anchor) != 3:
        raise ValueError('review three append sites')
    chunks = original.split(anchor)
    output = chunks[0]
    operations = {
        'push_back': anchor,
        'single_insert': 'candidates.insert(candidates.end(), zone->getLevelPosition());',
        'count_insert': 'candidates.insert(candidates.end(), 1, zone->getLevelPosition());',
    }
    for i, policy in enumerate(policies):
        output += operations[policy] + chunks[i + 1]
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('parent', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.parent / 'input.json', HOMM3_DIR)
    for name, text in originals.items():
        if (args.parent / 'snapshot' / name).read_text() != text:
            raise ValueError('parent snapshot no longer matches current source')
    helper = generator('generate-rmg-position-family.py')
    function = 'type_random_map_generator::appendZonePositions'
    baseline = helper.definition(originals['src/rmg.cpp'], function)
    rows = json.loads((args.parent / 'checkpoint.json').read_text())['elites']
    parents = []
    for row in rows:
        repeated = json.loads((args.parent / 'candidates' / row['id'] / 'repeat/result.json').read_text())
        if row['scores'] != repeated['scores'] or row['object_hash'] != repeated['object_hash']:
            raise ValueError('parent has not reproduced')
        rendered = render(originals, axes, tuple(row['choices']))
        for name, text in rendered.items():
            if (args.parent / 'candidates' / row['id'] / 'repeat/tree' / name).read_text() != text:
                raise ValueError('reproduced source differs from rendered parent')
        parents.append((row['labels']['append_reference_setters'], helper.definition(rendered['src/rmg.cpp'], function)))
    selected = next(body for label, body in parents if label == 'xyz+reference_int_y+both+fields+setter+fields')
    center = generator('generate-rmg-append-center-fields.py')
    models = [('setter_center', selected), ('direct_center', center.transform(selected, 'int', 'xy', 'late'))]
    forms = parents + [(label + '+' + '+'.join(policies), transform(body, policies))
                       for (label, body), policies in itertools.product(models,
                           itertools.product(('push_back', 'single_insert', 'count_insert'), repeat=3))]
    axis = helper.axis('append_insertion_frontier', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'public insertion forms with ten reproduced parents')


if __name__ == '__main__':
    main()
