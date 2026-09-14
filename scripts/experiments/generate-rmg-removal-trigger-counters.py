#!/usr/bin/env python3
"""Recombine removal counter ownership in the recovered trigger-copy context.

The copied TObjectType::TPoint restores retail's entrance calculation and
type/color registers (98.4%), but the zone counter still splits its memory
decrement. Earlier counter choices sometimes emitted the same object before
that copy existed. Revisit all their source forms with the copied trigger,
carrying every reproduced trigger-family finalist as a control.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkpoint', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    trigger = generator('generate-rmg-removal-trigger-frontier.py')
    retained = trigger.parents(args.checkpoint, expected_states=49)
    original = generator('generate-rmg-object-removal-family.py').definition(
        (HOMM3_DIR / 'src/rmg.cpp').read_text())
    forms = [('original', original)] + [(identity + '+parent', body) for identity, body in retained]
    for label, body in generator('generate-rmg-removal-counter-bindings.py').variants(original):
        name, transformed = next(trigger.variants(body))
        if name != 'TObjectType::TPoint':
            raise ValueError('review changed trigger-copy construction')
        forms.append((label + '+trigger_copy', transformed))
    axis = generator('generate-rmg-position-family.py').axis(
        'removal_trigger_counters', 'src/rmg.cpp', original, forms)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, units=['rmg'], evidence=__doc__,
                                          axes=[axis]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axis['options']), 'counter forms in copied-trigger context;', len(retained), 'reproduced parents')


if __name__ == '__main__':
    main()
