#!/usr/bin/env python3
"""Refine the verified value-wrapper parent at candidate-vector boundaries.

Retail 0x542080 retains range erase when a better gate score clears earlier
candidates. The current clear expands to copy/_Destroy; final placeGuard
expansions also omit two retained map lookups. Keep the entire vector
lifetime (retail destroys it on the later return paths) and test public
clear/erase/resize and append overloads with score/position initialization.
The natural inline decisions, not aggregate call counts, are the verdict.
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
        raise ValueError('unfinished parent')
    for file in (context / 'snapshot').rglob('*'):
        if file.is_file() and file.read_bytes() != (HOMM3_DIR / file.relative_to(context / 'snapshot')).read_bytes():
            raise ValueError('stale snapshot: ' + str(file))
    entry = next(row for row in checkpoint['elites'] if row['labels']['subterranean_bounds'] ==
                 'value_wrappers+copy+intersection+destination_first+assigned')
    folder = context / 'candidates' / entry['id']
    first, repeat = [json.loads((folder / kind / 'result.json').read_text()) for kind in ('first', 'repeat')]
    if first['object_hash'] != repeat['object_hash'] or first['object_hash'] != entry['object_hash']:
        raise ValueError('parent did not reproduce')
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    files = render(originals, axes, tuple(entry['choices']))
    if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
        raise ValueError('rendered parent differs')
    return originals['src/rmg.cpp'], files['src/rmg.cpp']


def variants(parent):
    for clear, append, score, position in itertools.product(
            ('clear', 'erase', 'resize'), ('push_back', 'insert', 'count_insert'),
            ('after_vector', 'before_vector'), ('assigned', 'copy')):
        body = parent
        clear_form = {'clear': 'candidates.clear();',
                      'erase': 'candidates.erase(candidates.begin(), candidates.end());',
                      'resize': 'candidates.resize(0);'}[clear]
        assert body.count('candidates.clear();') == 1
        body = body.replace('candidates.clear();', clear_form)
        append_form = {'push_back': 'candidates.push_back(position);',
                       'insert': 'candidates.insert(candidates.end(), position);',
                       'count_insert': 'candidates.insert(candidates.end(), 1, position);'}[append]
        assert body.count('candidates.push_back(position);') == 1
        body = body.replace('candidates.push_back(position);', append_form)
        if score == 'before_vector':
            old = '    std::vector<TRmgMapPosition> candidates;\n    int bestScore = 0;'
            new = '    int bestScore = 0;\n    std::vector<TRmgMapPosition> candidates;'
            assert body.count(old) == 1
            body = body.replace(old, new)
        if position == 'copy':
            old = '    TRmgMapPosition position;\n    position = source->getLevelPosition();'
            new = '    TRmgMapPosition position = source->getLevelPosition();'
            assert body.count(old) == 1
            body = body.replace(old, new)
        yield '+'.join((clear, append, score, position)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    original_source, parent_source = checked_parent(args.parent)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::createSubterraneanGate'
    original, parent = (helper.definition(value, name) for value in (original_source, parent_source))
    axis = helper.axis('subterranean_candidates', 'src/rmg.cpp', original,
                       [('wrapped_parent', parent), *variants(parent)])
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'subterranean vector-boundary controls')


if __name__ == '__main__':
    main()
