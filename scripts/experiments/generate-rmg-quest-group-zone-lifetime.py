#!/usr/bin/env python3
"""Recover the quest-group worker's shared zone-pointer lifetime.

Retail 0x54b300 stores each phase's current zone at EBP-0x10: randomization,
sorted insertion (which passes that slot by reference), and final placement.
The current three independent locals instead keep the first zone in ESI,
spill this, and omit the last zone store. Test sharing all or pairs of these
locals and the shared declaration's lifetime around vector construction and
the distance call. Preserve all random calls, live loop bounds, stable tied
insertion, filtering, placement arguments and cleanup. No Dreamcast match.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    yield 'separate', original
    statements = ('        TRmgZone* zone = m_zones[index];\n',
                  '        TRmgZone* zone = m_zones[index];\n',
                  '        TRmgZone* zone = candidates[index];\n')
    starts = []
    offset = 0
    for statement in statements:
        at = original.index(statement, offset)
        starts.append(at)
        offset = at + len(statement)
    for group, where in itertools.product(((0, 1, 2), (0, 1), (1, 2), (0, 2)),
                                          ('before_vector', 'after_vector', 'after_distances')):
        changed = original
        for index in reversed(group):
            at = starts[index]
            replacement = statements[index].replace('TRmgZone* zone', 'sharedZone')
            stop = changed.index('\n    }', at)
            tail = changed[at + len(statements[index]):stop]
            # Ordinary pointer variable uses only, with names preserved in
            # the unchanged phases. The fields and interfaces are untouched.
            import re
            tail = re.sub(r'\bzone\b', 'sharedZone', tail)
            changed = changed[:at] + replacement + tail + changed[stop:]
        declaration = '    TRmgZone* sharedZone;\n'
        vector = '    std::vector<TRmgZone*> candidates;\n'
        distances = '    calculateQuestZoneDistances(origin);\n'
        if where == 'before_vector':
            changed = changed.replace(vector, declaration + vector)
        elif where == 'after_vector':
            changed = changed.replace(vector, vector + declaration)
        else:
            changed = changed.replace(distances, distances + declaration)
        yield 'phases_' + ''.join(str(i) for i in group) + '+' + where, changed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    body = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                             'type_random_map_generator::placeQuestGroup')
    axis = helper.axis('quest_group_zone_lifetime', 'src/rmg.cpp', body, variants(body))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'zone-pointer phase-lifetime forms')


if __name__ == '__main__':
    main()
