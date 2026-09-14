#!/usr/bin/env python3
"""Test the header artifact masks' and reserved buffer's last-use boundaries.

Retail keeps the 144-bit mask at EBP-0x64 and the legacy mask at -0x88;
the current writer instead uses -0x9c and -0x6c. The reserved output array
is at -0xd0 versus -0xbc. Cross artifact-region scope, reserved-array scope
and declaration point, and legacy packed-buffer/bitset lifetimes. Preserve
every constructor, zero fill, canonical iterator/copy call and write order.
Only the uninitialized char array declaration can move to function entry.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def scope(body, start, end, indent):
    inner = body[start:end]
    return (body[:start] + indent + '{\n' +
            ''.join('    ' + line if line.strip() else line
                    for line in inner.splitlines(keepends=True)) +
            indent + '}\n' + body[end:])


def variants(original):
    for artifacts, reserved_scope, reserved_decl, legacy in itertools.product(
            (False, True), (False, True), ('local', 'entry'),
            ('original', 'buffer_first', 'bitset_last_use')):
        body = original
        if legacy != 'original':
            start = body.index('        std::bitset<129> legacyDisabledArtifacts;')
            end = body.index('        outfile->write(packedArtifacts, sizeof(packedArtifacts));', start)
            inner = body[start:end]
            declaration = '        unsigned char packedArtifacts[17];\n'
            assert inner.count(declaration) == 1
            inner = inner.replace(declaration, '')
            if legacy == 'bitset_last_use':
                inner = scope(inner, 0, len(inner), '        ')
            body = body[:start] + declaration + inner + body[end:]
        if artifacts:
            start = body.index('    std::bitset<144> disabledArtifacts;')
            end = body.index('    if (m_mapVersion >= 2) {\n        {\n            std::bitset<70>', start)
            body = scope(body, start, end, '    ')
        declaration = '    char reserved[31];\n'
        if reserved_decl == 'entry':
            assert body.count(declaration) == 1
            body = body.replace(declaration, '')
            body = body.replace('\n{\n', '\n{\n' + declaration, 1)
        if reserved_scope:
            start = body.index(declaration) if reserved_decl == 'local' else body.index('    memset(reserved,')
            last = '    outfile->write(reserved, sizeof(reserved));\n'
            end = body.index(last, start) + len(last)
            body = scope(body, start, end, '    ')
        yield '+'.join(map(str, (artifacts, reserved_scope, reserved_decl, legacy))), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::writeMapHeader')
    axis = helper.axis('header_artifact_lifetimes', 'src/rmg.cpp', original, variants(original))
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'artifact lifetime controls')


if __name__ == '__main__':
    main()
