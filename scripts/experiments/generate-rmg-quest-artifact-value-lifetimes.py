#!/usr/bin/env python3
"""Recover artifact selection and seer-hut pointer lifetimes in 0x54b490.

Retail copies the completed scan index into EBX at +0xbd and saves that
selected value for the success path; the candidate spills its reused index
before the scan. Retail also passes the seer-hut pointer's stack home to
vector insertion, whereas the explicit base-pointer copy has a second slot.
Test a separately captured artifact value and actual implicit/base-pointer
conversion lifetimes. Keep canonical helper calls, reference counts, group
ownership, initial-position snapshots and failure replacement order.
Complete-only function: no Dreamcast counterpart is known.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def variants(original):
    selected_store = '    seerHut->m_artifact = artifact;\n'
    pointer = '    type_object* questObject = seerHut;\n'
    for selected, owner in itertools.product(
            ('scan_index', 'copy_before_store', 'copy_after_store', 'reuse_selection'),
            ('base_value', 'direct_conversion', 'const_base_value', 'borrowed_conversion')):
        body = original
        if body.count(selected_store) != 1 or body.count(pointer) != 1:
            raise ValueError('review changed selected artifact or seer-hut pointer')
        if selected != 'scan_index':
            name = 'selected' if selected == 'reuse_selection' else 'selectedArtifact'
            declaration = ('    selected = artifact;\n' if selected == 'reuse_selection'
                           else '    int selectedArtifact = artifact;\n')
            at = body.index(selected_store)
            if selected == 'copy_after_store':
                at += len(selected_store)
            body = body[:at] + declaration + re.sub(r'\bartifact\b', name, body[at:])
        if owner == 'direct_conversion':
            body = body.replace(pointer, '').replace('push_back(questObject)', 'push_back(seerHut)')
            body = body.replace('addObject(questObject, position)', 'addObject(seerHut, position)')
        elif owner == 'const_base_value':
            body = body.replace(pointer, '    type_object* const questObject = seerHut;\n')
        elif owner == 'borrowed_conversion':
            body = body.replace(pointer, '    type_object* const& questObject = seerHut;\n')
        yield selected + '+' + owner, body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                 'type_random_map_generator::placeQuestArtifact')
    axis = helper.axis('quest_artifact_value_lifetimes', 'src/rmg.cpp', original, variants(original))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__, axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'selected-artifact and converted-pointer forms')


if __name__ == '__main__':
    main()
