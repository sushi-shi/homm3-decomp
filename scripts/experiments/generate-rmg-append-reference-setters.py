#!/usr/bin/env python3
"""Test existing setter calls after recovering append's Y-first conversion.

Two reproduced const-reference scalar-input parents recover the first-loop
bytes and frame but expand the first vector size that retail retains. Test
field writes, whole-coordinate assignment and the canonical ordinary zone
setter independently at all three placement sites. Preserve constructor
and setter definitions/declarations, accepted-position getter calls and
source snapshot timing. Carry all ten scalar-input elites as controls.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def transform(original, policies):
    fields = ''.join('        zone->m_levelPosition.m_' + c + ' = candidate.m_' + c + ';\n' for c in 'xyz')
    middle = fields.replace('        ', '    ')
    if original.count(fields) != 2 or original.count(middle) != 1:
        raise ValueError('review three coordinate write sites')
    first, rest = original.split(fields, 1)
    between, last = rest.rsplit(fields, 1)
    before_middle, after_middle = between.split(middle)
    chunks = [first, before_middle, after_middle, last]
    result = chunks[0]
    for index, policy in enumerate(policies):
        indent = '    ' if index == 1 else '        '
        if policy == 'fields':
            write = middle if index == 1 else fields
        elif policy == 'assignment':
            write = indent + 'zone->m_levelPosition = candidate;\n'
        elif policy == 'setter':
            write = indent + 'zone->setLevelPosition(candidate);\n'
        else:
            raise ValueError(policy)
        result += write + chunks[index + 1]
    return result


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
        parents.append((row['labels']['append_scalar_inputs'], helper.definition(rendered['src/rmg.cpp'], function)))
    selected = [p for p in parents if p[0] in ('xyz+reference_int_y+both', 'xyz+reference_int_yx+both')]
    if len(selected) != 2:
        raise ValueError('expected both reproduced reference-input parents')
    forms = parents + [(label + '+' + '+'.join(policies), transform(body, policies))
                       for (label, body), policies in itertools.product(selected,
                           itertools.product(('fields', 'assignment', 'setter'), repeat=3))]
    axis = helper.axis('append_reference_setters', 'src/rmg.cpp', baseline, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'reference-input setter forms with ten reproduced parents')


if __name__ == '__main__':
    main()
