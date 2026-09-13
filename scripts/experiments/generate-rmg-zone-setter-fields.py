#!/usr/bin/env python3
"""Test the canonical by-value zone setter's scalar field ownership.

appendZonePositions now reproduces both rings and the vector call stream,
but its middle whole-structure assignment uses ECX as a destination and
adds two register moves. Retail uses direct X/Y/Z member stores there.
Keep the ordinary setter, its by-value ABI and every source call; test its
one canonical body as whole assignment or each scalar store order. Score
the complete rmg TU, including all other callers of the setter.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'TRmgZone::setLevelPosition')
    anchor = '    m_levelPosition = position;\n'
    if original.count(anchor) != 1:
        raise ValueError('review canonical setter body')
    forms = [(''.join(order), original.replace(anchor, ''.join(
        '    m_levelPosition.m_' + c + ' = position.m_' + c + ';\n' for c in order)))
        for order in itertools.permutations('xyz')]
    axis = helper.axis('zone_setter_fields', 'src/rmg.cpp', original, forms)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'canonical setter bodies')


if __name__ == '__main__':
    main()
