#!/usr/bin/env python3
"""Test the rule object's default-construction boundary in the reader.

Retail 0x536560 retains two scalar single-insert calls where the candidate
expands to count-insert. Earlier matrices covered scalar-vector constructors,
not the owning rule's implicit default constructor. Preserve its two vector
members and leave scalars uninitialized until the reader assigns them. Test
an explicit default constructor in-class or ordinarily before/after the reader,
with implicit/explicit member-vector construction. No index-taking constructor,
library edits, fake caller expressions or diagnostic inlining directives.
All current uses assign the rule's scalar fields before copying it.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    start = header.index('struct TRmgObjectPlacementRule {')
    original = header[start:header.index('\n};', start) + 3]
    options = [dict(name='implicit', replace=original)]
    for location, members in itertools.product(('in_class', 'before', 'after'), range(4)):
        names = [name + '()' for bit, name in enumerate(('m_adjacentScores', 'm_blockedScores')) if members & (1 << bit)]
        init = ('\n    : ' + ', '.join(names)) if names else ''
        if location == 'in_class':
            declaration = '    TRmgObjectPlacementRule()' + init.replace('\n', '\n    ') + '\n    {\n    }\n'
            edits = []
        else:
            declaration = '    TRmgObjectPlacementRule();\n'
            body = ('// Rule construction boundary probe for readObjectPlacementRules;\n'
                    '// default vector construction precedes the existing scalar assignments.\n'
                    'TRmgObjectPlacementRule::TRmgObjectPlacementRule()' + init + '\n{\n}\n\n')
            anchor = ('void TRmgGeneratorBase::readObjectPlacementRules()' if location == 'before' else
                      '// Rank a footprint against terrain and already placed objects. The caller')
            # Keep the reader VA annotation attached to its own declaration.
            if location == 'before':
                anchor = 'VA(0x00536560, 0x5F2)'
            edits = [dict(source='src/rmg.cpp', insert_before=anchor, text=body)]
        options.append(dict(name=location + '+members_' + str(members),
                            replace=original[:-3] + '\n' + declaration + '};', extra_edits=edits))
    args.output.write_text(json.dumps(dict(schema=1,
        units=generator('generate-rmg-map-accessor-family.py').UNITS, evidence=__doc__,
        axes=[dict(name='rule_default_constructor', source='include/rmg.h', find=original, options=options)]), indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(options), 'rule construction controls across seven consumers')


if __name__ == '__main__':
    main()
