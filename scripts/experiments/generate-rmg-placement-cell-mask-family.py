#!/usr/bin/env python3
"""Retain nine verified joint parents; test four consumed per-cell mask points.

Retail carries y*8 across rows. Whole-loop mask owners recompute that product;
row-local owners restore induction but spill additional coordinates. Keep
scalar x/y loop counters and construct the canonical unsigned coordinate only
for the visited cell's two mask queries. Cross the four existing world owners,
without caching live prototype extents or changing the map lookup overload.
"""
import argparse
import hashlib
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('parent', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    context = args.parent
    for folder in ('src', 'include', 'vendor'):
        for path in (context / 'snapshot' / folder).rglob('*'):
            if path.is_file():
                assert path.read_bytes() == (HOMM3_DIR / path.relative_to(context / 'snapshot')).read_bytes(), path
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    parents = sorted(checkpoint['elites'], key=lambda r: r['choices'])
    assert [r['choices'] for r in parents] == [[i] for i in range(9)]
    options = []
    for row in parents:
        candidate = context / 'candidates' / row['id']
        first = json.loads((candidate / 'first/result.json').read_text())
        repeat = json.loads((candidate / 'repeat/result.json').read_text())
        for key in ('scores', 'object_hash', 'source_hashes', 'choices'):
            assert first[key] == repeat[key] == row[key], (row['id'], key)
        sources = render(originals, axes, row['choices'])
        for name, value in sources.items():
            assert value == (candidate / 'first/tree' / name).read_text()
            assert value == (candidate / 'repeat/tree' / name).read_text()
            assert hashlib.sha256(value.encode()).hexdigest() == row['source_hashes'][name]
        options.append(dict(name='parent:' + row['id'], replace=sources['src/rmg.cpp']))
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    assert options[0]['replace'] == source
    extract = generator('generate-rmg-position-family.py').definition
    world = generator('generate-rmg-placement-joint-cursor-family.py').world_lifetime
    for owner in ('whole_copy', 'row_copy', 'row_ctor', 'cell_ctor'):
        edited = source
        for name in ('isPlacementBlocked', 'addObject'):
            original = extract(source, 'type_random_map::' + name)
            body = world(original, owner)
            query = '            TRmgMapItem* item = getMapItem(nearby);'
            assert body.count(query) == 1
            body = body.replace(query, '            TRmgGridPoint maskPoint(x, y);\n' + query)
            body = body.replace('CObjectType::getBitPos(x, y)',
                'CObjectType::getBitPos(maskPoint.m_x, maskPoint.m_y)')
            for marker in ('prototype.getWidth()', 'prototype.getHeight()',
                           'getMapItem(nearby)', 'CObjectType::getBitPos('):
                assert body.count(marker) == original.count(marker)
            edited = edited.replace(original, body)
        options.append(dict(name='cell_mask+' + owner, replace=edited))
    assert len({o['replace'] for o in options}) == 13
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain'], evidence=__doc__,
        parent_context=context.name,
        reproduced_parents=[{k: row[k] for k in ('id','choices','source_hashes','object_hash')}
                            for row in parents],
        axes=[dict(name='cell_mask_ownership', source='src/rmg.cpp', find=source, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, loaded, new_axes = load_manifest(args.output, HOMM3_DIR)
    held = extract(source, 'type_random_map::canPlaceObject')
    for index in range(13):
        generated = render(loaded, new_axes, (index,))['src/rmg.cpp']
        assert extract(generated, 'type_random_map::canPlaceObject') == held
    print('13 finite states: nine verified parents and four consumed per-cell mask owners')

if __name__ == '__main__':
    main()
