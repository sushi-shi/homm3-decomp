#!/usr/bin/env python3
"""Cross reproduced object-queue owners with public pop and byte-parity forms.

VC6 VECTOR::pop_back delegates to erase(end()-1). Earlier separate-vector
controls changed its expansion boundary; test that public spelling in the
new paired-vector context, retaining every reproduced parent and its required
utility include. Preserve the actual seed calls and canonical sorted helper.
"""
import argparse
import copy
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def pop_body(body, mask, byte=False):
    pattern = re.compile(r'(positions|costs|pending\.first|pending\.second)\.erase\(\1\.end\(\) - 1\);')
    matches = list(pattern.finditer(body))
    if len(matches) != 2:
        raise ValueError('review the two ordered queue erases')
    for index in (1, 0):
        if mask & (1 << index):
            match = matches[index]
            body = body[:match.start()] + match[1] + '.pop_back();' + body[match.end():]
    if byte:
        if body.count('if (direction & 1)') != 1:
            raise ValueError('review direction parity')
        body = body.replace('if (direction & 1)', 'if (static_cast<unsigned char>(direction) & 1)')
    return body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    context = args.checkpoint.parent
    checkpoint = json.loads(args.checkpoint.read_text())
    if checkpoint.get('generation', 0) < 1 or len(checkpoint['records']) != 17 or len(checkpoint['elites']) != 10:
        raise ValueError('expected the completed queue ownership frontier')
    for relative in ('src/rmg.cpp', 'include/rmg.h'):
        if (context / 'snapshot' / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError('stale queue ownership snapshot: ' + relative)
    payload, originals, axes = source_families.load_manifest(context / 'input.json', HOMM3_DIR)
    parents = []
    for elite in checkpoint['elites']:
        rendered = source_families.render(originals, axes, elite['choices'])
        for attempt in ('first', 'repeat'):
            folder = context / 'candidates' / elite['id'] / attempt
            result = json.loads((folder / 'result.json').read_text())
            for key in ('id', 'choices', 'scores', 'object_hash', 'source_hashes'):
                if result[key] != elite[key]:
                    raise ValueError('parent reproduction mismatch: ' + key)
            for relative, text in rendered.items():
                if (folder / 'tree' / relative).read_text() != text:
                    raise ValueError('parent rendered source mismatch')
        parents.append(copy.deepcopy(payload['axes'][0]['options'][elite['choices'][0]]))
    original = generator('generate-rmg-add-object-family.py').definition((HOMM3_DIR / 'src/rmg.cpp').read_text())
    parents.sort(key=lambda option: option['replace'] != original)
    if parents[0]['replace'] != original:
        raise ValueError('frontier lacks unchanged caller control')
    options = []
    for mask, byte in ((0, False), (1, False), (2, False), (3, False), (1, True)):
        for parent in parents:
            option = copy.deepcopy(parent)
            option['replace'] = pop_body(parent['replace'], mask, byte)
            option['name'] += '+pop_%d+byte_%d' % (mask, byte)
            options.append(option)
    payload.update(evidence=__doc__, axes=[dict(name='object_queue_pop_frontier', source='src/rmg.cpp', find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'reproduced-owner/pop/parity states')


if __name__ == '__main__':
    main()
