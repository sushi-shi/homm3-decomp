#!/usr/bin/env python3
"""Extend reproduced gate projections through the entrance-coordinate update.

After trigger subtraction retail again copies the source Z before asking
for the destination level, preserving that returned value in a separate
temporary. Test the same projection at this last update over all ten
reproduced scan/placement parents, without moving callbacks or shared helpers.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    context = args.parent
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    if checkpoint.get('generation', 0) < 1 or len(checkpoint['elites']) != 10:
        raise ValueError('expected ten completed, reproduced parents')
    for file in (context / 'snapshot').rglob('*'):
        if file.is_file() and file.read_bytes() != (HOMM3_DIR / file.relative_to(context / 'snapshot')).read_bytes():
            raise ValueError('stale parent snapshot: ' + str(file))
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::createSubterraneanGate'
    original = helper.definition(originals['src/rmg.cpp'], name)
    choices = []
    old = ('    otherPosition = destination->getLevelPosition();\n'
           '    otherPosition.m_x = position.m_x;\n'
           '    otherPosition.m_y = position.m_y;')
    for entry in checkpoint['elites']:
        folder = context / 'candidates' / entry['id']
        first, repeat = [json.loads((folder / kind / 'result.json').read_text()) for kind in ('first', 'repeat')]
        if first['object_hash'] != repeat['object_hash'] or first['object_hash'] != entry['object_hash']:
            raise ValueError('parent did not reproduce')
        files = render(originals, axes, tuple(entry['choices']))
        if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
            raise ValueError('parent render differs')
        parent = helper.definition(files['src/rmg.cpp'], name)
        assert parent.count(old) == 1
        forms = [
            ('whole', old),
            ('copy_level', '    otherPosition = position;\n    otherPosition.m_z = destination->getLevelPosition().m_z;'),
            ('components_level', '    otherPosition.m_x = position.m_x;\n    otherPosition.m_y = position.m_y;\n    otherPosition.m_z = destination->getLevelPosition().m_z;'),
        ]
        for label, new in forms:
            choices.append((entry['labels']['subterranean_level_projection'] + '+entrance_' + label,
                            parent.replace(old, new)))
    axis = helper.axis('subterranean_entrance_projection', 'src/rmg.cpp', original, choices)
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'gate entrance-projection controls from ten parents')


if __name__ == '__main__':
    main()
