#!/usr/bin/env python3
"""Four canonical position-constructor/lookup definition ownership states.

Retail places the position constructor at align16(end(addGuard)), its first
retained caller, and the position lookup at align16(end(decorateMapCell)),
likewise its first retained caller. This is consistent with deferred header
emission, not proof of original TU or inline spelling. No RMG Dreamcast source
declaration survives. Test genuine in-class ownership of each canonical body
without explicit inline keywords, caller changes, duplicate definitions or
forced emission. Keep ABI and body expressions, moving each VA with its body.
"""
import argparse
import json
import textwrap
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    header = (HOMM3_DIR / 'include/rmg.h').read_text()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    module = generator('generate-rmg-position-family.py')
    pairs = []
    for name, declaration, claim in (
        ('TRmgMapPosition::TRmgMapPosition',
         '    TRmgMapPosition(int newX, int newY, int newZ);',
         'VA(0x005355C0, 0x1A)'),
        ('type_random_map::getMapItem',
         '    TRmgMapItem* getMapItem(TRmgMapPosition point);',
         'VA(0x005378E0, 0x27)'),
    ):
        parameters = 'TRmgMapPosition point' if 'getMapItem' in name else None
        body = module.definition(source, name, parameters=parameters)
        claimed = claim + '\n' + body
        assert source.count(claimed) == 1 and header.count(declaration) == 1
        member_body = body.replace(name, name.split('::')[-1], 1)
        replacement = textwrap.indent(claim + '\n' + member_body, '    ')
        pairs.append((name, declaration, claimed, replacement))
    options = []
    for ctor_in, lookup_in in ((False, False), (True, False), (False, True), (True, True)):
        new_source, new_header = source, header
        for enabled, (_, declaration, claimed, replacement) in zip((ctor_in, lookup_in), pairs):
            if enabled:
                new_header = new_header.replace(declaration, replacement)
                new_source = new_source.replace(claimed, '')
        options.append(dict(name=('ctor-in' if ctor_in else 'ctor-out') + '+' +
                            ('lookup-in' if lookup_in else 'lookup-out'), replace=new_header,
                            extra_edits=[dict(source='src/rmg.cpp', find=source, replace=new_source)]))
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain', 'scenarioinfo',
                                   'singleselectionpopups', 'singleselectionwindow', 'tiles'],
                   evidence=__doc__, axes=[dict(name='position_member_ownership',
                                               source='include/rmg.h', find=header, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, originals, axes = load_manifest(args.output, HOMM3_DIR)
    for choice in range(4):
        edited = render(originals, axes, (choice,))
        for name, _, claimed, _ in pairs:
            assert sum(text.count(claimed.split('\n', 1)[0]) for text in edited.values()) == 1
        assert edited['include/rmg.h'].count('TRmgMapPosition(int newX, int newY, int newZ)') == 1
        assert edited['include/rmg.h'].count('getMapItem(TRmgMapPosition point)') == 1
    print('4 canonical definition ownership states; one source claim/declaration per helper')


if __name__ == '__main__':
    main()
