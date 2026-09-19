#!/usr/bin/env python3
"""Canonical vector length, source ownership and connection distance lifetime.

canConnect's signed square/sqrt/ftol sequence has the same semantics as the
retained57-byte TRmgVector::length, currently hidden in rmg_support.cpp.
Test its existing ordinary body in either actual TU with natural caller
vector construction/lifetimes and meaningful arithmetic locals. Preserve the
six existing length calls in rmg.cpp; their call/expansion decisions and the
retained body must be audited together, not inferred from this one score.
No DC counterpart, invented helper, inline keyword or dummy operation.
Cross-TU claim migration is not scored by frozen targets: audit raw bodies
and run a normal full delink/build before adopting an ownership change.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def length_bodies(original):
    signature = 'int TRmgVector::length() const\n{\n'
    bodies = [
        original,
        signature + '    int squared = m_x * m_x + m_y * m_y;\n    return static_cast<int>(sqrt(static_cast<double>(squared)));\n}',
        signature + '    double squared = static_cast<double>(m_x * m_x + m_y * m_y);\n    return static_cast<int>(sqrt(squared));\n}',
        signature + '    double distance = sqrt(static_cast<double>(m_x * m_x + m_y * m_y));\n    return static_cast<int>(distance);\n}',
        signature + '    int xSquared = m_x * m_x;\n    int ySquared = m_y * m_y;\n    return static_cast<int>(sqrt(static_cast<double>(xSquared + ySquared)));\n}',
        signature + '    int squared = m_x * m_x;\n    squared += m_y * m_y;\n    return static_cast<int>(sqrt(static_cast<double>(squared)));\n}',
    ]
    assert len(set(bodies)) == 6
    return bodies


def callers(original):
    start = original.index('    int dy =')
    end = original.index('    int otherSize =')
    x = 'm_levelPosition.m_x - other->m_levelPosition.m_x'
    y = 'm_levelPosition.m_y - other->m_levelPosition.m_y'
    prefixes = [original[start:end],
        f'    int dy = {y};\n    int dx = {x};\n    TRmgVector delta(dx, dy);\n    int distance = delta.length();\n',
        f'    TRmgVector delta({x}, {y});\n    int distance = delta.length();\n',
        f'    TRmgVector delta;\n    delta.m_y = {y};\n    delta.m_x = {x};\n    int distance = delta.length();\n',
        f'    int distance;\n    {{\n        TRmgVector delta({x}, {y});\n        distance = delta.length();\n    }}\n',
    ]
    return [original[:start] + prefix + original[end:] for prefix in prefixes]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    extract = generator('generate-rmg-position-family.py').definition
    source = (HOMM3_DIR/'src/rmg.cpp').read_text()
    support = (HOMM3_DIR/'src/rmg_support.cpp').read_text()
    original = extract(source,'TRmgZone::canConnect')
    helper = extract(support,'TRmgVector::length')
    claimed = 'VA(0x005FCEB0, 0x39)\n' + helper + '\n\n'
    assert support.count(claimed)==1
    anchor = '// The three-point orientation helper at 0x5fdae0 belongs with the retained\n'
    assert source.count(anchor)==1
    options=[]
    for owner, caller, body in itertools.product(range(2), range(5), range(6)):
        row=dict(name=f'owner_{owner}+caller_{caller}+length_{body}',replace=callers(original)[caller])
        replacement='VA(0x005FCEB0, 0x39)\n'+length_bodies(helper)[body]+'\n\n'
        if owner:
            row['extra_edits']=[dict(source='src/rmg_support.cpp',find=claimed,replace=''),
                dict(source='src/rmg.cpp',find=anchor,replace=replacement+anchor)]
        elif body:
            row['extra_edits']=[dict(source='src/rmg_support.cpp',find=claimed,replace=replacement)]
        options.append(row)
    payload=dict(schema=1,units=['rmg','rmg_support'],evidence=__doc__,axes=[dict(
        name='connection_length_ownership',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    _,loaded,axes=source_families.load_manifest(args.output,HOMM3_DIR)
    assert source_families.render(loaded,axes,(0,))==loaded
    print('60 ordinary length ownership/body/caller states')


if __name__=='__main__':
    main()
