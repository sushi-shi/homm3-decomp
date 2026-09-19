#!/usr/bin/env python3
"""Zone-filter phase scopes and the bounding phase's coordinate lifetime.

Retail0x53b2f0 agrees in all110 block sizes and ordered semantic calls, but
both expanded counters allocate the destination/receiver differently. The
later bounding pass also keeps its index in a different spill slot and
reuses one coordinate-expression temporary. Test the combined function's
natural phase ownership, not additional spelling aliases inside the counter.
No Dreamcast counterpart exists. Canonical setters/getters/count helper and
all operations remain; these are explicit source hypotheses.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator


def models(original):
    marker = '    for (int candidate = 0; candidate < candidates.size(); ++candidate) {'
    boundary = '    int bestSize = 32000;'
    snapshots = [
        ('copy_initialize', 'TRmgMapPosition position = m_zones[other]->getLevelPosition();'),
        ('default_assign', 'TRmgMapPosition position;\n            position = m_zones[other]->getLevelPosition();'),
        ('const_reference', 'const TRmgMapPosition& position = m_zones[other]->getLevelPosition();'),
    ]
    current = [form for form in snapshots if form[1] in original]
    if len(current) != 1:
        raise ValueError('review the current bounds snapshot before generating')
    snapshots = current + [form for form in snapshots if form != current[0]]
    for scope, (snapshot, spelling) in itertools.product(range(4), snapshots):
        body = original.replace(current[0][1], spelling)
        if scope:
            body = body.replace('    int bestConnections = 0;\n', '', 1)
            start = body.index(marker)
            end = body.index(boundary)
            connection = '    int bestConnections = 0;\n' + body[start:end].rstrip()
            if scope >= 2:
                connection = '    {\n' + '\n'.join('    ' + line if line else '' for line in connection.splitlines()) + '\n    }'
            body = body[:start] + connection + '\n\n' + body[end:]
            if scope >= 2:
                # The bounds phase owns its index after the connection scope ends.
                start = body.index(boundary)
                body = body[:start] + body[start:].replace('for (candidate = 0;', 'for (int candidate = 0;', 1)
            if scope == 3:
                start = body.index(boundary)
                bounds = body[start:body.rfind('\n}')]
                body = body[:start] + '    {\n' + '\n'.join('    ' + line if line else '' for line in bounds.splitlines()) + '\n    }\n}'
        yield dict(name=f'phase_{scope}+snapshot_{snapshot}', replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR/'src/rmg.cpp').read_text()
    original = generator('generate-rmg-position-family.py').definition(source, 'type_random_map_generator::filterZonePositions')
    options = list(models(original))
    assert len(options) == 12 and options[0]['replace'] == original
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[dict(name='filter_phase', source='src/rmg.cpp', find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2)+'\n')
    load_manifest(args.output, HOMM3_DIR)
    print('12 whole-filter phase/snapshot models')


if __name__ == '__main__':
    main()
