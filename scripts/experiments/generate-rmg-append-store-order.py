#!/usr/bin/env python3
"""Test independent first/last ring coordinate writes in appendZonePositions.

Retail converts Y before X, then writes X/Z/Y in the first ring and X/Y/Z
in the last. The current canonical constructor path restores all vector
call positions but converts X first and allocates twelve extra frame bytes.
Test all six field-write orders and ordinary whole-coordinate assignment
independently at these two sites. Preserve constructor/helper definitions,
the center snapshot, late radius reads and the post-predicate getter.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def stores(order):
    if order == 'assignment':
        return '        zone->m_levelPosition = candidate;\n'
    return ''.join('        zone->m_levelPosition.m_' + field +
                   ' = candidate.m_' + field + ';\n' for field in order)


def variants(original):
    anchor = stores('xyz')
    if original.count(anchor) != 2:
        raise ValueError('review ring store sites')
    first, middle, last = original.split(anchor)
    orders = [''.join(p) for p in itertools.permutations('xyz')] + ['assignment']
    for first_order, last_order in itertools.product(orders, repeat=2):
        yield first_order + '+' + last_order, (
            first + stores(first_order) + middle + stores(last_order) + last)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::appendZonePositions')
    axis = helper.axis('append_store_order', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'independent ring write forms')


if __name__ == '__main__':
    main()
