#!/usr/bin/env python3
"""Four isolated profile/tail-overload states, with no source adoption.

Retail openConnectionPath retains its initial by-value position lookup but
performs scalar arithmetic at the loop tail. That arithmetic does not prove
the same source overload. Cross the two existing tail interfaces with /Ob1
and /Ob2, preserving all other source and all non-inline compiler switches.
"""
import argparse
import hashlib
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / 'src/rmg.cpp').read_text()
    extract = generator('generate-rmg-position-family.py').definition
    original = extract(source, 'type_random_map_generator::openConnectionPath')
    anchor = '        position = previous;\n        item = m_map.getMapItem(position);'
    replacement = ('        position = previous;\n'
                   '        item = m_map.getMapItem(position.m_x, position.m_y, position.m_z);')
    assert original.count(anchor) == 1
    options = []
    marker = '// HOMM3_ISOLATED_PROFILE_DIAGNOSTIC: /Ob1\n'
    assert marker not in source
    for profile in (2, 1):
        for scalar in (False, True):
            body = original.replace(anchor, replacement) if scalar else original
            if profile == 1:
                body = marker + body
            options.append(dict(name='Ob%d+%s' % (profile, 'scalar_tail' if scalar else 'position_tail'),
                                replace=body))
    runner = Path(__file__).with_name('run-rmg-inline-profile-diagnostic.py')
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain'], evidence=__doc__,
                   diagnostic_runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                   axes=[dict(name='profile_and_tail_interface', source='src/rmg.cpp',
                              find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    source_families.load_manifest(args.output, HOMM3_DIR)
    print('4 isolated profile/interface states')


if __name__ == '__main__':
    main()
