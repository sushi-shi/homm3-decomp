#!/usr/bin/env python3
"""Recover roster constructor expansion with the canonical member initialization.

The reproduced section-lifetime parents remove the extra vector begin but
retain an early treasure constructor absent from retail. The retained base
constructor at 0x534160 proves four ordered field stores and its value ABI.
Compare prefixes of member initializers with the remaining stores in the body,
preserving field order, its ordinary declaration and every source call. Carry
all ten reproduced roster parents and the unchanged caller. Measure the
constructor itself and every RMG consumer before accepting an inline decision.
"""
import argparse
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
    if len(checkpoint['records']) != 38 or len(checkpoint['elites']) != 10:
        raise ValueError('expected completed section family and ten reproduced parents')
    _, originals, axes = source_families.load_manifest(folder / 'input.json', folder / 'snapshot')
    if originals['src/rmg.cpp'] != (HOMM3_DIR / 'src/rmg.cpp').read_text():
        raise ValueError('parent snapshot differs from current source')
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::initializeObjectGenerators'
    original = helper.definition(originals['src/rmg.cpp'], name)
    constructor = helper.definition(originals['src/rmg.cpp'], 'type_treasure_def::type_treasure_def')
    parents = [('current', original)]
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
        parents.append((row['id'], helper.definition(rendered, name)))
    fields = ('ObjectType', 'Subtype', 'Value', 'Density')
    head = constructor[:constructor.index('{')].rstrip()
    constructors = []
    for count in range(5):
        initializers = ', '.join('m_' + field[0].lower() + field[1:] + '(new' + field + ')'
                                 for field in fields[:count])
        stores = ''.join('    m_' + field[0].lower() + field[1:] + ' = new' + field + ';\n'
                         for field in fields[count:])
        constructors.append(head + ('\n    : ' + initializers if count else '') + '\n{\n' + stores + '}')
    if constructors[0] != constructor:
        raise ValueError('review canonical constructor before generating')
    options = []
    for label, body in parents:
        for count, definition in enumerate(constructors):
            option = dict(name=label + '+initializers_' + str(count), replace=body)
            if count:
                option['extra_edits'] = [dict(source='src/rmg.cpp', find=constructor, replace=definition)]
            options.append(option)
    payload = dict(schema=1, units=['rmg'], evidence=__doc__,
                   axes=[dict(name='roster_constructor_initializers', source='src/rmg.cpp', find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'canonical constructor/roster states with reproduced parent controls')


if __name__ == '__main__':
    main()
