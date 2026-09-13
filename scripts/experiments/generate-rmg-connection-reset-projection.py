#!/usr/bin/env python3
"""Pair retail's terrain projection with reset-coordinate ownership.

The reproduced masked parent changes the generator's receiver to EDI before
the reset pass and spills one predecessor component, despite the first mask
occurring in the later zone pass. Test real construction and reset-only
lifetimes with the required unsigned projection. Keep resetMovement and the
ordinary coordinate constructor, all reset writes and their ordering.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def checked_parent(context):
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    if checkpoint.get('generation', 0) < 1:
        raise ValueError('unfinished parent')
    for file in (context / 'snapshot').rglob('*'):
        if file.is_file() and file.read_bytes() != (HOMM3_DIR / file.relative_to(context / 'snapshot')).read_bytes():
            raise ValueError('stale parent snapshot: ' + str(file))
    entry = next(row for row in checkpoint['elites']
                 if row['labels']['connection_terrain_lifetimes'] == 'function+int+direct+field')
    folder = context / 'candidates' / entry['id']
    first, repeat = [json.loads((folder / kind / 'result.json').read_text()) for kind in ('first', 'repeat')]
    if first['object_hash'] != repeat['object_hash'] or first['object_hash'] != entry['object_hash']:
        raise ValueError('parent did not reproduce')
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    files = render(originals, axes, tuple(entry['choices']))
    if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
        raise ValueError('parent source mismatch')
    return originals['src/rmg.cpp'], files['src/rmg.cpp']


def variants(parent):
    original = ('        TRmgMapPosition previous;\n'
                '        previous.m_x = -1;\n'
                '        previous.m_y = -1;\n'
                '        previous.m_z = -1;\n')
    for construction, lifetime, pass_scope in itertools.product(
            ('stores', 'direct', 'copy', 'assignment'),
            ('iteration', 'reset_call', 'pass'), ('function', 'block')):
        body = parent
        ctor = 'TRmgMapPosition(-1, -1, -1)'
        forms = {
            'stores': original,
            'direct': '        TRmgMapPosition previous(-1, -1, -1);\n',
            'copy': '        TRmgMapPosition previous = ' + ctor + ';\n',
            'assignment': '        TRmgMapPosition previous;\n        previous = ' + ctor + ';\n',
        }
        replacement = forms[construction]
        if lifetime == 'pass':
            body = body.replace('    while (count--) {',
                                ''.join(line[4:] for line in replacement.splitlines(keepends=True)) + '    while (count--) {', 1)
            replacement = ''
        if lifetime == 'reset_call':
            old = original + '        item->resetMovement(previous);\n'
            new = '        {\n' + ''.join('    ' + line for line in (replacement + '        item->resetMovement(previous);\n').splitlines(keepends=True)) + '        }\n'
            assert body.count(old) == 1
            body = body.replace(old, new)
        else:
            assert body.count(original) == 1
            body = body.replace(original, replacement)
        if pass_scope == 'block':
            start = body.index('    int count =')
            end = body.index('    for (unsigned int zoneIndex', start)
            body = body[:start] + '    {\n' + ''.join('    ' + line for line in body[start:end].splitlines(keepends=True)) + '    }\n' + body[end:]
        yield '+'.join((construction, lifetime, pass_scope)), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    source, parent_source = checked_parent(args.parent)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::buildZoneConnectionPaths'
    original, parent = (helper.definition(value, name) for value in (source, parent_source))
    axis = helper.axis('connection_reset_projection', 'src/rmg.cpp', original,
                       [('masked_parent', parent), *variants(parent)])
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'masked reset-construction controls')


if __name__ == '__main__':
    main()
