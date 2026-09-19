#!/usr/bin/env python3
"""Cross reproduced roster lifetimes with coherent scalar construction policy.

The canonical base constructor owns four scalar fields. Six inline derived
constructors own eight further scalar fields. Keep interfaces, helper calls,
inline declarations, field order and definition positions; compare body stores
with member initializers, never inherited-field overrides. Retail requires the
base's 39 bytes and caller registration order. Both nested 63/64 budgets and the
35000-unit running cap must be reviewed across all seven header consumers.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path

from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator

KINDS = {
    'type_black_box_experience_def': [('m_experience', 'experience')],
    'type_black_box_gold_def': [('m_gold', 'gold')],
    'type_black_box_spells_def': [('m_minimumLevel', 'minimumLevel'),
                                ('m_maximumLevel', 'maximumLevel'),
                                ('m_schoolMask', 'schoolMask')],
    'type_prison_def': [('m_experience', 'experience')],
    'type_quest_experience_def': [('m_experience', 'experience')],
    'type_quest_gold_def': [('m_gold', 'gold')],
}


def definition(source, name):
    return generator('generate-rmg-position-family.py').definition(source, name)


def class_prefix(header, kind):
    start = header.index('class ' + kind + ' : public type_treasure_def {')
    end = header.index('\n    virtual ', start)
    return header[start:end]


def header_edits(header):
    edits = []
    for kind, fields in KINDS.items():
        old = class_prefix(header, kind)
        head, body = old.rsplit('\n    {', 1)
        expected = '\n' + ''.join('        this->' + field + ' = ' + arg + ';\n'
                                  for field, arg in fields) + '    }\n'
        assert body == expected, kind
        new = head + ',\n        ' + ', '.join(field + '(' + arg + ')'
                                               for field, arg in fields)
        new += '\n    {\n    }\n'
        edits.append(dict(source='include/rmg.h', find=old, replace=new))
    return edits


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('parent', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    root = Path(os.environ['HOMM3_DIR'])
    context = args.parent
    source = (root / 'src/rmg.cpp').read_text()
    header = (root / 'include/rmg.h').read_text()
    # The whole source/header snapshot must still be the current baseline,
    # rather than merely finding a surviving local edit anchor.
    for folder in ('src', 'include'):
        for path in (context / 'snapshot' / folder).rglob('*'):
            if path.is_file():
                assert path.read_bytes() == (root / path.relative_to(context / 'snapshot')).read_bytes(), path
    prior, originals, axes = load_manifest(context / 'input.json', root)
    checkpoint = json.loads((context / 'checkpoint.json').read_text())
    parents = []
    for choices in ([0], [1]):
        row = next(r for r in checkpoint['elites'] if r['choices'] == choices)
        candidate = context / 'candidates' / row['id']
        first = json.loads((candidate / 'first/result.json').read_text())
        repeat = json.loads((candidate / 'repeat/result.json').read_text())
        assert all(first[k] == repeat[k] == row[k]
                   for k in ('scores', 'object_hash', 'source_hashes'))
        sources = render(originals, axes, choices)
        for name, value in sources.items():
            assert value == (candidate / 'first/tree' / name).read_text()
            assert hashlib.sha256(value.encode()).hexdigest() == row['source_hashes'][name]
        parents.append((row, definition(sources['src/rmg.cpp'],
                                       'type_random_map_generator::initializeObjectGenerators')))
    roster = definition(source, 'type_random_map_generator::initializeObjectGenerators')
    base = definition(source, 'type_treasure_def::type_treasure_def')
    base_new = base[:base.index('\n{')] + (
        '\n    : m_objectType(newObjectType), m_subtype(newSubtype),\n'
        '      m_value(newValue), m_density(newDensity)\n{\n}')
    assert base[base.index('\n{'):] == (
        '\n{\n    m_objectType = newObjectType;\n    m_subtype = newSubtype;\n'
        '    m_value = newValue;\n    m_density = newDensity;\n}')
    derived = header_edits(header)
    options = []
    for parent, base_init, derived_init in itertools.product(range(2), range(2), range(2)):
        row, body = parents[parent]
        edits = []
        if base_init:
            edits.append(dict(source='src/rmg.cpp', find=base, replace=base_new))
        if derived_init:
            edits.extend(derived)
        options.append(dict(name=f"parent_{row['id']}+base_{base_init}+derived_{derived_init}",
                            replace=body, extra_edits=edits))
    payload = dict(schema=1, evidence=__doc__,
        units=['rmg', 'rmg_support', 'rmg_terrain', 'scenarioinfo',
               'singleselectionpopups', 'singleselectionwindow', 'tiles'],
        parent_context=context.name,
        reproduced_parents=[{key: row[key] for key in ('id', 'choices', 'source_hashes', 'object_hash')}
                            for row, _ in parents],
        axes=[dict(name='roster_constructor_policy', source='src/rmg.cpp',
                   find=roster, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, root)
    print('8 states from two fully reproduced current parents ->', args.output)


if __name__ == '__main__':
    main()
