#!/usr/bin/env python3
"""Refine reproduced mask read interfaces with their canonical size bounds.

All six retail packing loops compare unsigned indices against the bitset's
extent. Compare literal extents with bitset::size(), preserving the reads,
mask buffers, stores and stream order. The size method is a real loop-bound
query, not an extra unused helper invocation. Carry all ten reproduced read
interface parents and verify complete snapshots and repeated identities.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator

LOOPS = (
    ('heroBit', 'availableHeroes', 156), ('roeHeroBit', 'availableHeroes', 128),
    ('artifactBit', 'disabledArtifacts', 144), ('legacyArtifactBit', 'legacyDisabledArtifacts', 129),
    ('spell', 'disabledSpells', 70), ('skill', 'disabledSkills', 28),
)
PATTERNS = (('literal', ()), ('all', tuple(range(6))), ('heroes', (0, 1)),
            ('artifacts', (2, 3)), ('magic', (4, 5)), ('legacy', (1, 3)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    context = args.parent
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    assert checkpoint.get('generation', 0) >= 1 and len(checkpoint['elites']) == 10
    for file in (context / 'snapshot').rglob('*'):
        if file.is_file() and file.read_bytes() != (HOMM3_DIR / file.relative_to(context / 'snapshot')).read_bytes():
            raise ValueError('stale parent snapshot: ' + str(file))
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::writeMapHeader'
    original = helper.definition(originals['src/rmg.cpp'], name)
    variants = []
    for entry in checkpoint['elites']:
        folder = context / 'candidates' / entry['id']
        first, repeat = [json.loads((folder / kind / 'result.json').read_text()) for kind in ('first', 'repeat')]
        if first['object_hash'] != repeat['object_hash'] or first['object_hash'] != entry['object_hash']:
            raise ValueError('parent did not reproduce')
        files = render(originals, axes, tuple(entry['choices']))
        if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
            raise ValueError('parent render differs')
        parent = helper.definition(files['src/rmg.cpp'], name)
        for label, selected in PATTERNS:
            body = parent
            for i in selected:
                index, mask, size = LOOPS[i]
                old = index + ' < ' + str(size)
                assert body.count(old) == 1
                body = body.replace(old, index + ' < ' + mask + '.size()', 1)
            variants.append((entry['labels']['mask_read_surfaces'] + '+size:' + label, body))
    axis = helper.axis('mask_size_bounds', 'src/rmg.cpp', original, variants)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'size-bound controls from ten reproduced parents')


if __name__ == '__main__':
    main()
