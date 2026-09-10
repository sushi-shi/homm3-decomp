#!/usr/bin/env python3
"""Negative controls for dialog icon morale statement ordering."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()
start = source.index('void type_dialog_icon::set(EGameResource resource, long qualifier)\n{')
body = source[start:source.index('\n}\n', start) + 2]
edits = []
for mood, frame in [('GOOD', 4), ('NEUTRAL', 3), ('BAD', 2)]:
    old = f'''    case RES_{mood}_MORALE:
        m_spriteName = DATA_COMPGEN(
            0x006600ec, dialogMoraleSprite, "imrl82.def");
        m_spriteFrameIndex = {frame};
        break;'''
    new = f'''    case RES_{mood}_MORALE:
        m_spriteFrameIndex = {frame};
        m_spriteName = DATA_COMPGEN(
            0x006600ec, dialogMoraleSprite, "imrl82.def");
        break;'''
    assert body.count(old) == 1
    edits.append((old, new))
options = []
for choices in itertools.product(range(2), repeat=3):
    candidate = body
    for choice, (old, new) in zip(choices, edits):
        if choice: candidate = candidate.replace(old, new)
    row = {'name': '-'.join(map(str, choices))}
    if any(choices): row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'morale-frame-before-sprite-name', 'find': body, 'options': options}],
    'evidence': [
        'Fresh full DC/retail pass and verified C2 inline trace for0x4f4eb0; current99.1667, two nested experience _Eos sites are the residual. The resource/qualifier field accesses already agree and are fixed.',
        'DC5046/5051/5056 loads the morale FILENAME into R5, not a frame constant. The following groups jump into the luck arms shared name-assignment calls; frame values follow those calls. The earlier frame-first reading was mistaken.',
        'The existing name-before-frame order agrees with the DC shared tails at5061/5062,5066/5067,5071/5072. The reversed states are negative controls, not source corrections, and no state is adopted merely for a better score.',
        'Eight finite states measure each reversed morale arm independently and together. No new calls, synthetic statements, inline pins or shared-header edits. All45 exact kb siblings are scored.',
    ],
}, indent=2) + '\n')
