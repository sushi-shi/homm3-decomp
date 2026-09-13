#!/usr/bin/env python3
"""Test canonical bitset read interfaces in the map-header packing loops.

Retail retains _Xran in the hero and artifact packing tests. The candidate
expands several of those paths. VC6's mutable subscript returns a proxy,
whereas its const subscript delegates directly to test. Compare those real
interfaces and borrowed const test receivers across hero, artifact and magic
groups. Keep all mask construction, modification, zeroing and write order.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

GROUPS = (
    ((156, 'availableHeroes', 'heroBit'), (128, 'availableHeroes', 'roeHeroBit')),
    ((144, 'disabledArtifacts', 'artifactBit'), (129, 'legacyDisabledArtifacts', 'legacyArtifactBit')),
    ((70, 'disabledSpells', 'spell'), (28, 'disabledSkills', 'skill')),
)


def variants(original):
    # Rebase an adopted read interface to the original test control while
    # retaining the current bound expressions and other source decisions.
    for group in GROUPS:
        for bits, mask, index in group:
            view = mask + 'Read'
            declaration = 'const std::bitset<' + str(bits) + '>& ' + view + ' = ' + mask + ';'
            if declaration in original:
                start = original.rfind('\n', 0, original.index(declaration)) + 1
                end = original.index('\n', original.index(declaration)) + 1
                original = original[:start] + original[end:]
            target = mask + '.test(' + index + ')'
            for old in (view + '.test(' + index + ')', view + '[' + index + ']', mask + '[' + index + ']'):
                original = original.replace(old, target)
            assert original.count(target) == 1
    for forms in itertools.product(('test', 'subscript', 'const_test', 'const_subscript'), repeat=3):
        body = original
        for form, group in zip(forms, GROUPS):
            if form == 'test':
                continue
            for bits, mask, index in group:
                old = mask + '.test(' + index + ')'
                assert body.count(old) == 1
                receiver = mask
                if form.startswith('const_'):
                    start = body.index('for (unsigned int ' + index + ' = 0;')
                    line = body.rfind('\n', 0, start) + 1
                    indent = body[line:start]
                    receiver = mask + 'Read'
                    declaration = indent + 'const std::bitset<' + str(bits) + '>& ' + receiver + ' = ' + mask + ';\n'
                    body = body[:line] + declaration + body[line:]
                new = receiver + ('.test(' + index + ')' if form == 'const_test' else '[' + index + ']')
                body = body.replace(old, new, 1)
        yield '+'.join(forms), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'type_random_map_generator::writeMapHeader')
    axis = helper.axis('mask_read_surfaces', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'mask read-interface controls')


if __name__ == '__main__':
    main()
