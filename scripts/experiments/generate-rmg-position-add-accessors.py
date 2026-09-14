#!/usr/bin/env python3
"""Extend reproduced addition-helper parents with existing TPoint accessors.

The translated-coordinate input family leaves both coastal constructor calls
expanded. TPoint already owns getX/getY, used by the terrain consumers. Test
their source calls independently in the canonical addition helper, preserving
its by-value argument, single coordinate construction, and every caller. Keep
all reproduced parents as controls; no declaration or inline policy changes.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    root = args.checkpoint.parent
    checkpoint = json.loads(args.checkpoint.read_text())
    if checkpoint['generation'] != 1 or len(checkpoint['records']) != 55 or len(checkpoint['elites']) != 10:
        raise ValueError('expected the completed 55-form input family with ten reproduced parents')
    _, originals, axes = source_families.load_manifest(root / 'input.json', root / 'snapshot')
    for source, text in originals.items():
        if (HOMM3_DIR / source).read_text() != text:
            raise ValueError('parent source snapshot differs from current ' + source)
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition(originals['src/rmg.cpp'], 'TRmgMapPosition::operator+')
    parents = [('current', original)]
    for elite in checkpoint['elites']:
        folder = root / 'candidates' / elite['id']
        first = json.loads((folder / 'first/result.json').read_text())
        repeat = json.loads((folder / 'repeat/result.json').read_text())
        for field in ('id', 'choices', 'scores', 'object_hash', 'source_hashes'):
            if first[field] != repeat[field] or first[field] != elite[field]:
                raise ValueError('parent reproduction failed: ' + elite['id'] + ' ' + field)
        rendered = source_families.render(originals, axes, tuple(elite['choices']))
        if rendered['src/rmg.cpp'] != (folder / 'first/tree/src/rmg.cpp').read_text():
            raise ValueError('parent rendered body differs from compiled source')
        parents.append((elite['id'], helper.definition(rendered['src/rmg.cpp'], 'TRmgMapPosition::operator+')))
    alternatives = []
    for (name, body), getters in itertools.product(parents, ('none', 'x', 'y', 'xy')):
        result = body
        for field in getters if getters != 'none' else '':
            anchor = 'offset.m_' + field
            if result.count(anchor) != 1:
                raise ValueError('review addition input expression')
            result = result.replace(anchor, 'offset.get' + field.upper() + '()')
        alternatives.append((name + '+' + getters, result))
    axis = helper.axis('position_add_accessors', 'src/rmg.cpp', original, alternatives)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'addition accessor forms with verified parent controls')


if __name__ == '__main__':
    main()
