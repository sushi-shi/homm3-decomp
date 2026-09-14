#!/usr/bin/env python3
"""Recover the two retained coastal coordinate constructors through lifetimes.

Current retail comparison at 0x548a40 shows two retained three-coordinate
constructors followed by loads from their returned EAX object. The current
caller expands both and uses a 0x10 frame instead of 0x20. Keep canonical
operator+ and constructor bodies/declarations; test working-coordinate
copies from named value/reference results, independently at both sites.
Also compare a copied step with a borrowed table step. Preserve all three
walks, the asymmetric bounds, final tile and packed direction/target bits.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    first_expression = 'position + g_rmgDirections[(direction + 2) & 7]'
    second_expression = 'position + g_rmgDirections[(direction + 1) & 7]'
    first_anchor = '    TRmgMapPosition point = ' + first_expression + ';\n'
    second_anchor = '    point = ' + second_expression + ';\n'
    step_anchor = '    TPoint step = g_rmgDirections[(direction - 2) & 7];\n'
    for anchor in (first_anchor, second_anchor, step_anchor):
        if original.count(anchor) != 1:
            raise ValueError('review coastal coordinate/step sites')
    policies = ('direct', 'value', 'const_value', 'reference', 'scoped_reference')
    for first, second, step in itertools.product(policies, policies, ('copy', 'reference')):
        body = original
        for index, (expression, anchor, policy) in enumerate((
                (first_expression, first_anchor, first), (second_expression, second_anchor, second))):
            if policy == 'direct':
                continue
            name = 'firstPoint' if index == 0 else 'secondPoint'
            if policy == 'scoped_reference':
                replacement = ('    TRmgMapPosition point;\n' if index == 0 else '')
                replacement += ('    {\n        const TRmgMapPosition& ' + name + ' = ' + expression + ';\n'
                                '        point = ' + name + ';\n    }\n')
            else:
                kind = {'value': 'TRmgMapPosition', 'const_value': 'const TRmgMapPosition',
                        'reference': 'const TRmgMapPosition&'}[policy]
                replacement = '    ' + kind + ' ' + name + ' = ' + expression + ';\n'
                replacement += ('    TRmgMapPosition point = ' if index == 0 else '    point = ') + name + ';\n'
            body = body.replace(anchor, replacement)
        if step == 'reference':
            body = body.replace(step_anchor, '    const TPoint& step = g_rmgDirections[(direction - 2) & 7];\n')
        yield '+'.join((first, second, step)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::markRiverCoastTarget')
    axis = helper.axis('coast_return_lifetimes', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'coastal returned-coordinate lifetime forms')


if __name__ == '__main__':
    main()
