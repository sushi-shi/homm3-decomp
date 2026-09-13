#!/usr/bin/env python3
"""Recombine reproduced rule constructors with the three scalar append calls.

The constructor-only frontier leaves single-insert expansions unresolved.
Earlier all-push_back and two-insert caller forms differed in retained rule
fill/copy_backward coverage. Cross every reproduced constructor object with
all eight public scalar append choices in this changed construction context;
score the reader and every affected helper rather than only its caller peak.
"""
import argparse
import copy
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    context = args.checkpoint.parent
    checkpoint = json.loads(args.checkpoint.read_text())
    if checkpoint.get('generation', 0) < 1 or len(checkpoint['records']) != 13:
        raise ValueError('expected the completed constructor frontier')
    for relative in ('src/rmg.cpp', 'include/rmg.h'):
        if (context / 'snapshot' / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError('stale constructor snapshot: ' + relative)
    payload, originals, axes = source_families.load_manifest(context / 'input.json', HOMM3_DIR)
    constructor = copy.deepcopy(payload['axes'][0])
    constructor['options'] = []
    for elite in checkpoint['elites']:
        expected = source_families.render(originals, axes, elite['choices'])
        for attempt in ('first', 'repeat'):
            folder = context / 'candidates' / elite['id'] / attempt
            result = json.loads((folder / 'result.json').read_text())
            for key in ('id', 'choices', 'scores', 'object_hash', 'source_hashes'):
                if result[key] != elite[key]:
                    raise ValueError('constructor reproduction mismatch: ' + key)
            for relative, text in expected.items():
                if (folder / 'tree' / relative).read_text() != text:
                    raise ValueError('constructor rendered-source mismatch')
        constructor['options'].append(copy.deepcopy(payload['axes'][0]['options'][elite['choices'][0]]))
    # Retain the unchanged source as the first corner; the parent constructor
    # experiment must include its independently reproduced implicit control.
    constructor['options'].sort(key=lambda option: option['name'] != 'implicit')
    if constructor['options'][0]['name'] != 'implicit':
        raise ValueError('frontier lacks its unchanged constructor control')
    anchor = ('        objectTypes.insert(objectTypes.end(), objectType);\n'
              '        terrains.insert(terrains.end(), terrain);\n'
              '        subtypes.push_back(subtype);')
    options = []
    for mask in (3, 0, 1, 2, 4, 5, 6, 7):
        lines = []
        for bit, (container, value) in enumerate((('objectTypes', 'objectType'), ('terrains', 'terrain'), ('subtypes', 'subtype'))):
            statement = container + '.insert(' + container + '.end(), ' + value + ')' if mask & (1 << bit) else container + '.push_back(' + value + ')'
            lines.append('        ' + statement + ';')
        options.append(dict(name='insert_mask_' + str(mask), replace='\n'.join(lines)))
    appends = dict(name='scalar_append_calls', source='src/rmg.cpp', find=anchor, options=options)
    payload.update(evidence=__doc__, axes=[constructor, appends])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(constructor['options']) * 8, 'reproduced-constructor/scalar-append states')


if __name__ == '__main__':
    main()
