#!/usr/bin/env python3
"""Recover gate-scan coordinates from the retail level-value projections.

Retail preserves the initial returned level-position's unused x/y in a
temporary while only z feeds the scanned coordinate; the destination query
likewise retains a separate temporary. Model z-only projection versus a
whole returned position, and copied source coordinates versus overwritten
destination coordinates, keeping every query at the same callback boundary.
The last destination coordinate is also reconstructed after each placement.
Cross the canonical clear and explicit range erase without changing helpers.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(parent):
    for initial, scanned, placed, cleared in itertools.product(
            ('whole', 'level_only'), ('whole', 'copy_level', 'components_level'),
            ('whole', 'copy_level', 'components_level'), ('clear', 'erase')):
        body = parent
        if initial == 'level_only':
            old = '    position = source->getLevelPosition();'
            assert body.count(old) == 1
            body = body.replace(old, '    position.m_z = source->getLevelPosition().m_z;')
        for indentation, choice in (('            ', scanned), ('    ', placed)):
            if choice == 'whole':
                continue
            old = indentation + 'TRmgMapPosition otherPosition = destination->getLevelPosition();\n' + indentation + 'otherPosition.m_x = position.m_x;\n' + indentation + 'otherPosition.m_y = position.m_y;'
            if choice == 'copy_level':
                new = indentation + 'TRmgMapPosition otherPosition = position;\n'
            else:
                new = indentation + 'TRmgMapPosition otherPosition;\n' + indentation + 'otherPosition.m_x = position.m_x;\n' + indentation + 'otherPosition.m_y = position.m_y;\n'
            new += indentation + 'otherPosition.m_z = destination->getLevelPosition().m_z;'
            assert body.count(old) == 1
            body = body.replace(old, new)
        if cleared == 'erase':
            assert body.count('candidates.clear();') == 1
            body = body.replace('candidates.clear();', 'candidates.erase(candidates.begin(), candidates.end());')
        yield '+'.join((initial, scanned, placed, cleared)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    original_source, parent_source = generator('generate-rmg-subterranean-candidates.py').checked_parent(args.parent)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::createSubterraneanGate'
    original, parent = (helper.definition(value, name) for value in (original_source, parent_source))
    axis = helper.axis('subterranean_level_projection', 'src/rmg.cpp', original,
                       [('wrapped_parent', parent), *variants(parent)])
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'gate level-projection controls')


if __name__ == '__main__':
    main()
