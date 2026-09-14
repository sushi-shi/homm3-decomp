#!/usr/bin/env python3
"""Test the existing zone-size accessor's ordinary definition boundary.

canConnect has the retail distance prefix and CFG but swaps ECX/EBX after
the size loads. The previous 64 accessor/ownership forms kept getSize's
in-class body. The ordinary count helper resolved removeObject's analogous
memory-operation residual. Test this existing getter with its unchanged
int-returning const signature and body, either in-class or as one ordinary
definition before/after canConnect. Cross copied/borrowed field/getter reads
on both receivers. Preserve query order, arithmetic and all other callers.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    original = generator('generate-rmg-connect-owners-family.py').definition(source)
    variants = [(name, body) for name, body in
                generator('generate-rmg-connect-accessor-bindings.py').variants(original)
                if name.endswith('+0+0')]
    calls = generator('generate-rmg-position-family.py').axis(
        'connection_size_calls', 'src/rmg.cpp', original, variants)
    old = '    int getSize() const\n    {\n        return m_slot->m_size;\n    }'
    body = ('// Ordinary size accessor boundary probe for canConnect 0x532bd0;\n'
            '// retain the one canonical body and existing const value interface.\n'
            'int TRmgZone::getSize() const\n{\n    return m_slot->m_size;\n}\n\n')
    visibility = dict(name='zone_size_definition', source='include/rmg.h', find=old,
                      options=[dict(name='in_class')])
    for name, anchor in (
            ('ordinary_before', '// FilterZonePositions calls this predicate at 0x53b4b7 and 0x53b5ae.'),
            ('ordinary_after', '// Lazy exterior boundary of blocked/trigger cells. Start below the first')):
        visibility['options'].append(dict(name=name, replace='    int getSize() const;',
            extra_edits=[dict(source='src/rmg.cpp', insert_before=anchor, text=body)]))
    args.output.write_text(json.dumps(dict(schema=1,
        units=generator('generate-rmg-map-accessor-family.py').UNITS, evidence=__doc__,
        axes=[calls, visibility]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(variants) * 3, 'caller/size-definition states across seven consumers')


if __name__ == '__main__':
    main()
