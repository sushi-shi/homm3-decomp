#!/usr/bin/env python3
"""Test selector input ownership at the terrain/range register exchange.

Retail 0x546040 keeps terrain in EDI and the prototype range in ESI; the
candidate reverses them while retaining all 17 blocks and ten branches.
Earlier range/record controls did not bind all three incoming values.
Cross direct parameter use, immutable copies and const references to the
actual value parameters, before/after candidate-vector construction. Keep
the public ABI, checked mask, ordered insertion and sole random draw.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    head, body = original.split('\n{\n', 1)
    vector = '    std::vector<TRmgObjectPropertiesRef*> candidates;\n'
    if body.count(vector) != 1:
        raise ValueError('review changed candidate vector lifetime')
    for choices in itertools.product(('direct', 'copy', 'reference'), repeat=3):
        for position in ('before', 'after'):
            if choices == ('direct',) * 3 and position == 'after':
                continue
            result = body
            declarations = []
            for name, replacement, choice in zip(
                    ('terrain', 'objectType', 'subtype'),
                    ('selectedTerrain', 'selectedObjectType', 'selectedSubtype'), choices):
                if choice == 'direct':
                    continue
                result = re.sub(r'\b' + name + r'\b', replacement, result)
                kind = 'const int&' if choice == 'reference' else 'const int'
                declarations.append('    ' + kind + ' ' + replacement + ' = ' + name + ';\n')
            declarations = ''.join(declarations)
            result = result.replace(vector, declarations + vector if position == 'before' else vector + declarations)
            yield '+'.join(choices + (position,)), head + '\n{\n' + result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::selectObjectPrototype')
    axis = helper.axis('selector_input_bindings', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'selector input binding states')


if __name__ == '__main__':
    main()
