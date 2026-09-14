#!/usr/bin/env python3
"""Recombine pointer lifetimes by the roster's existing initialization sections.

The uniform scoped-base-pointer parent restores late registration expansion
and removes the extra vector begin, but retains two early constructors absent
from retail. Compare direct/scoped-base registration independently in the
initial roster, key-tent loop, ordinary middle roster, quest loop and final
roster. Preserve all eight reproduced pointer-family controls and every actual
constructor/push_back operation; no wrapper overloads or inline pins are added.
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
    folder = args.checkpoint.parent
    checkpoint = json.loads(args.checkpoint.read_text())
    if len(checkpoint['records']) != 8 or len(checkpoint['elites']) != 8:
        raise ValueError('expected the completed eight-form pointer family')
    _, originals, axes = source_families.load_manifest(folder / 'input.json', folder / 'snapshot')
    if originals['src/rmg.cpp'] != (HOMM3_DIR / 'src/rmg.cpp').read_text():
        raise ValueError('parent snapshot differs from current source')
    helper = generator('generate-rmg-position-family.py')
    pointer = generator('generate-rmg-roster-pointer-lifetimes.py')
    name = 'type_random_map_generator::initializeObjectGenerators'
    original = helper.definition(originals['src/rmg.cpp'], name)
    alternatives = []
    for row in checkpoint['elites']:
        root = folder / 'candidates' / row['id']
        first = json.loads((root / 'first/result.json').read_text())
        repeat = json.loads((root / 'repeat/result.json').read_text())
        for key in ('id', 'choices', 'scores', 'object_hash', 'source_hashes'):
            if first[key] != repeat[key] or first[key] != row[key]:
                raise ValueError('parent reproduction mismatch: ' + key)
        rendered = source_families.render(originals, axes, tuple(row['choices']))['src/rmg.cpp']
        if rendered != (root / 'first/tree/src/rmg.cpp').read_text():
            raise ValueError('rendered parent differs from compiled source')
        alternatives.append(('parent_' + row['id'], helper.definition(rendered, name)))
    markers = ('    {\n        int player =',
               '    m_objectGenerators.push_back(new type_treasure_def(7, 0, 8000, 20));',
               '    for (int quest = 0;',
               '    m_objectGenerators.push_back(new type_treasure_def(84, 0, 1000, 100));')
    if any(original.count(marker) != 1 for marker in markers):
        raise ValueError('review current roster section boundaries')
    positions = [0] + [original.index(marker) for marker in markers] + [len(original)]
    sections = [original[a:b] for a, b in zip(positions, positions[1:])]
    paired = [(section, dict(pointer.variants(section))['value_base']) for section in sections]
    for choices in itertools.product((0, 1), repeat=5):
        alternatives.append(('sections_' + ''.join(str(v) for v in choices),
                             ''.join(section[choice] for section, choice in zip(paired, choices))))
    axis = helper.axis('roster_pointer_sections', 'src/rmg.cpp', original, alternatives)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'section-lifetime forms with eight reproduced parent controls')


if __name__ == '__main__':
    main()
