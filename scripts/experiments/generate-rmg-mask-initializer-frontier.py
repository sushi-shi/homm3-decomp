#!/usr/bin/env python3
"""Couple reproduced mask packing forms with canonical hero output construction.

The size-bound frontier restores the legacy artifact test call but its best
member adds hero output-subscript calls absent in retail. Revisit the three
real transform-output constructions and const input ownership in this changed
caller context. Preserve every mask parent, std::transform, the byte predicate,
the ordinary helper and its source calls; never restore an inline-depth pin.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--parent', type=Path, required=True)
    args = parser.parse_args()
    context = args.parent
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    assert checkpoint.get('generation', 0) >= 1 and len(checkpoint['elites']) == 10
    for file in (context / 'snapshot').rglob('*'):
        if file.is_file() and file.read_bytes() != (HOMM3_DIR / file.relative_to(context / 'snapshot')).read_bytes():
            raise ValueError('stale parent snapshot: ' + str(file))
    _, originals, axes = load_manifest(context / 'input.json', HOMM3_DIR)
    helper = generator('generate-rmg-position-family.py')
    name = 'type_random_map_generator::writeMapHeader'
    original = helper.definition(originals['src/rmg.cpp'], name)
    initializer = helper.definition(originals['src/rmg.cpp'], 'setAvailableRmgHeroes')
    head = initializer[:initializer.index('\n{\n')]
    options = [dict(name='baseline', replace=original)]
    seen = {(original, initializer)}
    for entry in checkpoint['elites']:
        folder = context / 'candidates' / entry['id']
        first, repeat = [json.loads((folder / kind / 'result.json').read_text()) for kind in ('first', 'repeat')]
        if first['object_hash'] != repeat['object_hash'] or first['object_hash'] != entry['object_hash']:
            raise ValueError('parent did not reproduce')
        files = render(originals, axes, tuple(entry['choices']))
        if files['src/rmg.cpp'] != (folder / 'repeat/tree/src/rmg.cpp').read_text():
            raise ValueError('parent render differs')
        parent = helper.definition(files['src/rmg.cpp'], name)
        assert helper.definition(files['src/rmg.cpp'], 'setAvailableRmgHeroes') == initializer
        for construction, borrowed_const in itertools.product(('copy', 'direct', 'temporary'), (False, True)):
            signature = head.replace('unsigned char*', 'const unsigned char*') if borrowed_const else head
            output = 'bitset_iterator<N>(*availableHeroes, 0)'
            inner = ''
            if construction == 'copy':
                inner = '    bitset_iterator<N> output = bitset_iterator<N>(*availableHeroes, 0);\n'
                output = 'output'
            elif construction == 'direct':
                inner = '    bitset_iterator<N> output(*availableHeroes, 0);\n'
                output = 'output'
            inner += '    std::transform(heroFlag, end, ' + output + ', std::logical_not<unsigned char>());\n'
            setup = signature + '\n{\n' + inner + '}'
            if (parent, setup) in seen:
                continue
            seen.add((parent, setup))
            options.append(dict(name=entry['labels']['mask_size_bounds'] + '+initializer:' + construction + '+' + str(borrowed_const),
                                replace=parent, extra_edits=[dict(source='src/rmg.cpp', find=initializer, replace=setup)]))
    axis = dict(name='mask_initializer_frontier', source='src/rmg.cpp', find=original, options=options)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'initializer controls from ten reproduced size-bound parents')


if __name__ == '__main__':
    main()
