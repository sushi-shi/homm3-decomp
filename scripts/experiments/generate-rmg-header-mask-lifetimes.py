#!/usr/bin/env python3
"""Refine reproduced header array parents with availability-mask lifetimes.

Retail's header frame and nested bitset call decisions still differ after
the player geometry arrays expire before team assignment. End each hero
bitset's lifetime after packing while its byte buffer survives the write,
and independently scope the spell and skill serialization blocks. These
canonical bitsets have no user-defined destructor; preserve all construction,
test, packing and output statements and keep every real helper boundary.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def variants(parent):
    for expansion_heroes, original_heroes, magic in itertools.product((False, True), repeat=3):
        body = parent
        for bits, size, enabled in ((156, 20, expansion_heroes), (128, 16, original_heroes)):
            if not enabled:
                continue
            start = body.index('        std::bitset<' + str(bits) + '> availableHeroes;')
            end = body.index('        outfile->write(packedHeroes, sizeof(packedHeroes));', start)
            inner = body[start:end]
            declaration = '        unsigned char packedHeroes[' + str(size) + '];\n'
            assert inner.count(declaration) == 1
            inner = inner.replace(declaration, '')
            body = body[:start] + declaration + '        {\n' + ''.join('    ' + line if line.strip() else line for line in inner.splitlines(keepends=True)) + '        }\n' + body[end:]
        if magic:
            for bits, name, output in ((70, 'disabledSpells', 'packedSpells'), (28, 'disabledSkills', 'packedSkills')):
                start = body.index('        std::bitset<' + str(bits) + '> ' + name + ';')
                last = '        outfile->write(' + output + ', sizeof(' + output + '));\n'
                end = body.index(last, start) + len(last)
                inner = body[start:end]
                body = body[:start] + '        {\n' + ''.join('    ' + line if line.strip() else line for line in inner.splitlines(keepends=True)) + '        }\n' + body[end:]
        yield '+'.join(map(str, (expansion_heroes, original_heroes, magic))), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    context = args.parent
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    if checkpoint.get('generation', 0) < 1 or len(checkpoint['elites']) != 6:
        raise ValueError('expected six reproduced player-lifetime parents')
    for file in (context / 'snapshot').rglob('*'):
        if file.is_file() and file.read_bytes() != (HOMM3_DIR / file.relative_to(context / 'snapshot')).read_bytes():
            raise ValueError('stale parent snapshot: ' + str(file))
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::writeMapHeader'
    original = helper.definition(originals['src/rmg.cpp'], name)
    choices = []
    for entry in checkpoint['elites']:
        folder = context / 'candidates' / entry['id']
        first, repeat = [json.loads((folder / kind / 'result.json').read_text()) for kind in ('first', 'repeat')]
        if first['object_hash'] != repeat['object_hash'] or first['object_hash'] != entry['object_hash']:
            raise ValueError('parent did not reproduce')
        files = render(originals, axes, tuple(entry['choices']))
        if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
            raise ValueError('parent render differs')
        parent = helper.definition(files['src/rmg.cpp'], name)
        choices += [(entry['labels']['header_player_lifetimes'] + '+masks:' + label, body)
                    for label, body in variants(parent)]
    axis = helper.axis('header_mask_lifetimes', 'src/rmg.cpp', original, choices)
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'header mask-lifetime controls from six parents')


if __name__ == '__main__':
    main()
