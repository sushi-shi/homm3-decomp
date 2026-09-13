#!/usr/bin/env python3
"""Combine reproduced roster layouts with single-reference spell-box inputs.

One borrowed spell-box input removes the current early base-constructor call
but brings back a late constructor/insertion mismatch. Recombine those inputs
with the ten reproduced roster-section parents, the original direct roster and
the adopted current body. All constructor stores, factory arguments and public
push_back calls remain canonical. Previously verified parent field/trace
oracles cover these component forms; the seven-unit compiler search checks
their combined inlining and every tracked consumer.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def verified_elites(checkpoint_path, expected):
    folder = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if len(checkpoint['records']) != expected:
        raise ValueError('unexpected parent population size')
    _, originals, axes = source_families.load_manifest(folder / 'input.json', folder / 'snapshot')
    rendered = []
    for row in checkpoint['elites']:
        parent = folder / 'candidates' / row['id']
        first = json.loads((parent / 'first/result.json').read_text())
        repeat = json.loads((parent / 'repeat/result.json').read_text())
        for key in ('id', 'choices', 'scores', 'object_hash', 'source_hashes'):
            if first[key] != repeat[key] or first[key] != row[key]:
                raise ValueError('parent reproduction mismatch: ' + key)
        files = source_families.render(originals, axes, tuple(row['choices']))
        for filename, text in files.items():
            if text != (parent / 'first/tree' / filename).read_text():
                raise ValueError('rendered parent differs from compiled source')
        rendered.append((row['id'], files))
    return originals, rendered


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('roster_checkpoint', type=Path)
    parser.add_argument('input_checkpoint', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    roster_originals, roster_elites = verified_elites(args.roster_checkpoint, 38)
    input_originals, input_elites = verified_elites(args.input_checkpoint, 16)
    if len(roster_elites) != 10 or len(input_elites) != 8:
        raise ValueError('expected ten roster and eight input elites')
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    if input_originals['include/rmg.h'] != header or (args.input_checkpoint.parent / 'snapshot/src/rmg.cpp').read_text() != source:
        raise ValueError('current source differs from input-family baseline')
    if (args.roster_checkpoint.parent / 'snapshot/include/rmg.h').read_text() != header:
        raise ValueError('the old roster parents used a different constructor interface')
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::initializeObjectGenerators'
    original = helper.definition(source, name)
    parents = [('current', original), ('original_direct', helper.definition(roster_originals['src/rmg.cpp'], name))]
    parents.extend((label, helper.definition(files['src/rmg.cpp'], name)) for label, files in roster_elites)
    # The adopted parent differs only in indentation and if/for brace placement.
    # Keep its current spelling once; no original-source/DC layout is compared.
    seen = set()
    unique = []
    for label, body in parents:
        key = ' '.join(body.split())
        if key not in seen:
            unique.append((label, body))
            seen.add(key)
    spellbox = generator('generate-rmg-spellbox-initialization.py').constructor(header)
    names = ('value', 'minimumLevel', 'maximumLevel', 'schoolMask')
    arguments = ', '.join('int ' + name for name in names)
    if spellbox.count(arguments) != 1:
        raise ValueError('review the current input declaration')
    options = []
    for label, body in unique:
        for reference in (None,) + names:
            declared = ', '.join(('const int& ' if name == reference else 'int ') + name for name in names)
            option = dict(name=label + '+' + (reference or 'all_values'), replace=body)
            if reference is not None:
                option['extra_edits'] = [dict(source='include/rmg.h', find=spellbox,
                                              replace=spellbox.replace(arguments, declared))]
            options.append(option)
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain', 'tiles',
                                  'singleselectionpopups', 'singleselectionwindow', 'scenarioinfo'],
                   evidence=__doc__, axes=[dict(name='spellbox_roster_frontier', source='src/rmg.cpp',
                                               find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(unique), 'distinct roster parents x 5 input forms =', len(options), 'states')


if __name__ == '__main__':
    main()
