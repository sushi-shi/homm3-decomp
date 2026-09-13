#!/usr/bin/env python3
"""Combine reproduced removal counter bindings with trigger-query ownership.

Retail 0x54bc50 frees ESI after the global decrement, uses EDX for the
entrance Y load, then loads the object type into ESI before testing the
zone. All counter-binding parents retain a split zone decrement; their
entrance arithmetic still occupies different registers. Test the existing
trigger point's copied/borrowed lifetime and named X/Y subtraction order,
without changing the canonical scalar map accessor or footprint scan.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def parents(checkpoint_path, expected_states=51):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get('generation', 0) != 1 or len(checkpoint['records']) != expected_states:
        raise ValueError('expected the completed ' + str(expected_states) + '-state parent population')
    if {tuple(row['choices']) for row in checkpoint['records']} != {(i,) for i in range(expected_states)}:
        raise ValueError('counter population is not exhaustive')
    for folder in ('src', 'include'):
        frozen, live = context / 'snapshot' / folder, HOMM3_DIR / folder
        files = {p.relative_to(frozen): p.read_bytes() for p in frozen.rglob('*') if p.is_file()}
        current = {p.relative_to(live): p.read_bytes() for p in live.rglob('*') if p.is_file()}
        if files != current:
            raise ValueError('changed counter snapshot: ' + folder)
    _, originals, axes = source_families.load_manifest(context / 'input.json', HOMM3_DIR)
    extract = generator('generate-rmg-object-removal-family.py').definition
    result = []
    for elite in checkpoint['elites']:
        candidate = context / 'candidates' / elite['id']
        first = json.loads((candidate / 'first/result.json').read_text())
        repeat = json.loads((candidate / 'repeat/result.json').read_text())
        for key in ('scores', 'object_hash', 'source_hashes', 'choices'):
            if first[key] != repeat[key] or repeat[key] != elite[key]:
                raise ValueError('counter parent did not reproduce: ' + key)
        rendered = source_families.render(originals, axes, tuple(elite['choices']))
        for relative, text in rendered.items():
            if (candidate / 'repeat/tree' / relative).read_text() != text:
                raise ValueError('counter input does not render the reproduced parent')
            if source_families.digest(text.encode()) != repeat['source_hashes'][relative]:
                raise ValueError('counter parent source hash differs')
        result.append((elite['id'], extract(rendered['src/rmg.cpp'])))
    return result


def variants(parent):
    query = ('        int zone = m_map.getMapItem(position.m_x - prototype->m_triggerCell.m_x,\n'
             '            position.m_y - prototype->m_triggerCell.m_y, position.m_z)->m_zoneState.m_zone;')
    if parent.count(query) != 1:
        raise ValueError('review changed entrance query')
    for kind in ('TObjectType::TPoint', 'const TObjectType::TPoint', 'const TObjectType::TPoint&'):
        statement = '        ' + kind + ' trigger = prototype->m_triggerCell;\n'
        yield kind, parent.replace(query, statement + query.replace('prototype->m_triggerCell.', 'trigger.'))
    for order in ('xy', 'yx'):
        statements = ''.join('        int entrance' + component.upper() + ' = position.m_' + component
                             + ' - prototype->m_triggerCell.m_' + component + ';\n' for component in order)
        replacement = '        int zone = m_map.getMapItem(entranceX, entranceY, position.m_z)->m_zoneState.m_zone;'
        yield 'subtractions_' + order, parent.replace(query, statements + replacement)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    retained = parents(args.checkpoint)
    original = generator('generate-rmg-object-removal-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    forms = [('original', original)] + [(identity + '+parent', body) for identity, body in retained]
    for identity, body in retained:
        forms += [(identity + '+' + name, variant) for name, variant in variants(body)]
    axis = generator('generate-rmg-position-family.py').axis(
        'removal_trigger_ownership', 'src/rmg.cpp', original, forms)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'trigger/counter ownership states from', len(retained), 'reproduced parents')


if __name__ == '__main__':
    main()
