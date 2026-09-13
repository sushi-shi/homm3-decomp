#!/usr/bin/env python3
"""Cross reproduced artifact scopes with canonical source iterator ownership.

Retail's legacy copy calls bitset<144>::test, then bitset<129>::set, and
retains separate two-word source/end homes. Compare the existing mutable
adapter with the existing const traversal adapter, and temporary versus
named range endpoints. Preserve std::copy and both canonical adapters.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def variants(parent):
    start = parent.index('std::copy(\n', parent.index('std::bitset<129> legacyDisabledArtifacts;'))
    line = parent.rfind('\n', 0, start) + 1
    indent = parent[line:start]
    end = parent.index(');', start) + 2
    original = parent[line:end]
    for const, named in itertools.product((False, True), ('temporary', 'end', 'all')):
        adapter = 'TConstBitsetIterator' if const else 'bitset_iterator'
        first = adapter + '<144>(disabledArtifacts, 0)'
        last = adapter + '<144>(disabledArtifacts, 129)'
        output = 'bitset_iterator<129>(legacyDisabledArtifacts, 0)'
        lines = []
        if named in ('end', 'all'):
            lines.append(adapter + '<144> artifactEnd(disabledArtifacts, 129);')
            last = 'artifactEnd'
        if named == 'all':
            lines += [adapter + '<144> artifactBegin(disabledArtifacts, 0);',
                      'bitset_iterator<129> artifactOutput(legacyDisabledArtifacts, 0);']
            first, output = 'artifactBegin', 'artifactOutput'
        lines += ['std::copy(', '    ' + first + ',', '    ' + last + ',', '    ' + output + ');']
        new = '\n'.join(indent + line for line in lines)
        yield str(const) + '+' + named, parent.replace(original, new, 1), const


def end_variants(parent):
    for construction, placement in itertools.product(
            ('direct', 'copy', 'const', 'assigned'), ('copy', 'before_bitset', 'copy_scope')):
        body = next(body for label, body, const in variants(parent) if label == 'False+end')
        start = body.index('bitset_iterator<144> artifactEnd(')
        line = body.rfind('\n', 0, start) + 1
        indent = body[line:start]
        end = body.index('\n', start) + 1
        old = body[line:end]
        declaration = {
            'direct': 'bitset_iterator<144> artifactEnd(disabledArtifacts, 129);',
            'copy': 'bitset_iterator<144> artifactEnd = bitset_iterator<144>(disabledArtifacts, 129);',
            'const': 'const bitset_iterator<144> artifactEnd(disabledArtifacts, 129);',
            'assigned': 'bitset_iterator<144> artifactEnd;\n' + indent + 'artifactEnd = bitset_iterator<144>(disabledArtifacts, 129);',
        }[construction]
        body = body[:line] + indent + declaration + '\n' + body[end:]
        if placement == 'before_bitset':
            body = body.replace(indent + declaration + '\n', '', 1)
            anchor = indent + 'std::bitset<129> legacyDisabledArtifacts;\n'
            assert body.count(anchor) == 1
            body = body.replace(anchor, indent + declaration + '\n' + anchor, 1)
        elif placement == 'copy_scope':
            end = body.index(');', body.index('std::copy(', line)) + 3
            scopes = generator('generate-rmg-header-artifact-lifetimes.py')
            body = scopes.scope(body, line, end, indent)
        yield construction + '+' + placement, body, False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    parser.add_argument('--end-lifetimes', action='store_true',
                        help='refine mutable end construction and lifetime, including the original scope')
    args = parser.parse_args()
    context = args.parent
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    assert checkpoint.get('generation', 0) >= 1 and len(checkpoint['elites']) == 4
    for file in (context / 'snapshot').rglob('*'):
        if file.is_file() and file.read_bytes() != (HOMM3_DIR / file.relative_to(context / 'snapshot')).read_bytes():
            raise ValueError('stale parent snapshot: ' + str(file))
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::writeMapHeader'
    original = helper.definition(originals['src/rmg.cpp'], name)
    options = [dict(name='baseline', replace=original)]
    seen = {(original, False)}
    parents = [('baseline', original)] if args.end_lifetimes else []
    for entry in checkpoint['elites']:
        folder = context / 'candidates' / entry['id']
        first, repeat = [json.loads((folder / kind / 'result.json').read_text()) for kind in ('first', 'repeat')]
        if first['object_hash'] != repeat['object_hash'] or first['object_hash'] != entry['object_hash']:
            raise ValueError('parent did not reproduce')
        files = render(originals, axes, tuple(entry['choices']))
        if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
            raise ValueError('parent render differs')
        parent = helper.definition(files['src/rmg.cpp'], name)
        parents.append((entry['labels']['header_artifact_lifetimes'], parent))
    for parent_label, parent in parents:
        for label, body, const in (end_variants(parent) if args.end_lifetimes else variants(parent)):
            if (body, const) in seen:
                continue
            seen.add((body, const))
            option = dict(name=parent_label + '+iterator:' + label, replace=body)
            if const:
                option['extra_edits'] = [dict(source='src/rmg.cpp', find='#include "bitset_iterator.h"',
                                              replace='#include "bitset_iterator.h"\n#include "const_bitset_iterator.h"')]
            options.append(option)
    axis = dict(name='header_artifact_iterators', source='src/rmg.cpp', find=original, options=options)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'iterator controls from four reproduced artifact parents' +
          (' and the original scope' if args.end_lifetimes else ''))


if __name__ == '__main__':
    main()
