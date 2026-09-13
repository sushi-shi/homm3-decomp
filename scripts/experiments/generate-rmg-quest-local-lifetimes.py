#!/usr/bin/env python3
"""Test quest-factory local lifetimes around the wrapper allocation merge.

Retail 0x534b90 reloads the hut pointer before the definition pointer on
wrapper allocation failure; the current candidate reverses those two loads.
All success-path instructions and canonical calls already match. Cross
pointer declaration placement/order, the selected artifact's last-use scope,
and the creature count's declaration/reward scope. Move only uninitialized
scalar declarations; preserve allocation, query, capture and store order.
There is no Dreamcast counterpart for this Complete-only override.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    head = original[:original.index('\n{\n')]
    declarations = {
        'seerHut': 'rmgSeerHutObject* seerHut;',
        'artifact': 'TRmgObjectPropertiesRef* artifact;',
        'object': 'rmgQuestArtifactObject* object;',
    }
    initializers = {
        'seerHut': 'new rmgSeerHutObject(properties)',
        'artifact': 'generator->selectObjectPrototype(eTerrainDirt, 0x41, 0)',
        'object': 'new rmgQuestArtifactObject(artifact, generator, seerHut, this)',
    }
    for pointers, count_place, artifact_scope, reward_scope in itertools.product(
            ('original', 'result_after_hut', 'result_entry', 'all_entry', 'reverse_entry'),
            ('capture', 'entry', 'after_hut'), (False, True), (False, True)):
        lines = []
        declared = set()

        def declare(name):
            lines.append(declarations[name])
            declared.add(name)

        def initialize(name):
            if name in declared:
                return name + ' = ' + initializers[name] + ';'
            return declarations[name][:-1] + ' = ' + initializers[name] + ';'

        if pointers in ('all_entry', 'reverse_entry'):
            for name in (('seerHut', 'artifact', 'object') if pointers == 'all_entry'
                         else ('object', 'artifact', 'seerHut')):
                declare(name)
        elif pointers == 'result_entry':
            declare('object')
        if count_place == 'entry':
            lines.append('int count;')
        lines.append(initialize('seerHut'))
        if count_place == 'after_hut':
            lines.append('int count;')
        if pointers == 'result_after_hut' or (artifact_scope and 'object' not in declared):
            declare('object')
        construction = [initialize('artifact'), initialize('object')]
        if artifact_scope:
            lines += ['{'] + ['    ' + line for line in construction] + ['}']
        else:
            lines += construction
        rewards = [('int count' if count_place == 'capture' else 'count') + ' = m_adjustedValue;',
                   'seerHut->m_creatureType = m_creatureType;',
                   'seerHut->m_creatureCount = count;']
        if reward_scope:
            lines += ['{'] + ['    ' + line for line in rewards] + ['}']
        else:
            lines += rewards
        lines.append('return object;')
        body = head + '\n{\n' + ''.join('    ' + line + '\n' for line in lines) + '}'
        yield '+'.join(map(str, (pointers, count_place, artifact_scope, reward_scope))), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(), 'type_quest_creature_def::generate')
    alternatives = list(variants(original))
    assert alternatives[0][1] == original, 'review the changed factory before generating'
    axis = helper.axis('quest_local_lifetimes', 'src/rmg.cpp', original, alternatives)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'quest local-lifetime controls')


if __name__ == '__main__':
    main()
