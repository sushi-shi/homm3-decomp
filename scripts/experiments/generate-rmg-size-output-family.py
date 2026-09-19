#!/usr/bin/env python3
"""Map dimension query: hidden value result versus explicit output reference.

The calibrated tiny output-reference control reproduces base getSize's 21 bytes
and both adapters' 39 bytes. Retail painter 0x5b4669 passes a stack temporary,
then copies from the returned pointer at 0x5b4676..0x5b4684: passing m_size
directly contradicts that positive evidence. Keep one local output object and
consume the returned reference. No signed-domain conversion or new helper.
"""
import argparse
import hashlib
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--scoped', action='store_true', help='include the shorter painter query lifetime')
    args = parser.parse_args()
    files = ('include/rmg.h', 'src/rmg.cpp', 'src/rmg_terrain.cpp')
    originals = {name: (HOMM3_DIR / name).read_text() for name in files}
    live = dict(originals)
    helper = generator('generate-rmg-size-helper-family.py').HELPER
    canonical = helper in originals[files[0]]
    if canonical:
        originals[files[0]] = originals[files[0]].replace(helper, '').replace(
            '    using TRmgMapInterface::getSize;\n', '')
        originals[files[1]] = originals[files[1]].replace(
            '    return m_map->getSize();',
            '    TRmgGridPoint size;\n    return m_map->getSize(size);')
        originals[files[2]] = originals[files[2]].replace(
            '    m_size = m_adapter->getSize();',
            '    {\n        TRmgGridPoint size;\n        m_size = m_adapter->getSize(size);\n    }')
    adopted = 'virtual TRmgGridPoint& getSize(TRmgGridPoint& output)' in originals[files[0]]
    if adopted:
        assert originals[files[0]].count('virtual TRmgGridPoint& getSize(TRmgGridPoint& output)') == 2
        originals[files[0]] = originals[files[0]].replace(
            'virtual TRmgGridPoint& getSize(TRmgGridPoint& output)', 'virtual TRmgGridPoint getSize()')
        originals[files[1]] = originals[files[1]].replace(
            'TRmgGridPoint& type_random_map::getSize(TRmgGridPoint& output)',
            'TRmgGridPoint type_random_map::getSize()').replace(
            'output = TRmgGridPoint(m_mapWidth, m_mapHeight);\n    return output;',
            'return TRmgGridPoint(m_mapWidth, m_mapHeight);').replace(
            'TRmgGridPoint size;\n    return m_map->getSize(size);',
            'TRmgGridPoint size = m_map->getSize();\n    return size;')
        scoped_query = '    {\n        TRmgGridPoint size;\n        m_size = m_adapter->getSize(size);\n    }'
        assert originals[files[2]].count(scoped_query) == 1
        originals[files[2]] = originals[files[2]].replace(scoped_query,
            '    m_size = m_adapter->getSize();')
    edited = dict(originals)
    extract = generator('generate-rmg-position-family.py').definition
    header = edited['include/rmg.h']
    for owner in ('TRmgMapInterface', 'type_random_map'):
        start = header.index('class ' + owner + ' ')
        end = header.index('\n};', start)
        group = header[start:end]
        old = 'virtual TRmgGridPoint getSize()'
        assert group.count(old) == 1
        header = header[:start] + group.replace(old,
            'virtual TRmgGridPoint& getSize(TRmgGridPoint& output)') + header[end:]
    edited['include/rmg.h'] = header
    source = edited['src/rmg.cpp']
    base = extract(source, 'type_random_map::getSize')
    replacement = base.replace('TRmgGridPoint type_random_map::getSize()',
        'TRmgGridPoint& type_random_map::getSize(TRmgGridPoint& output)')
    replacement = replacement.replace('return TRmgGridPoint(m_mapWidth, m_mapHeight);',
        'output = TRmgGridPoint(m_mapWidth, m_mapHeight);\n    return output;')
    source = source.replace(base, replacement)
    for owner in ('TRmgMapAdapter', 'TRmgRoadMapAdapter'):
        body = extract(source, owner + '::getSize')
        replacement = body.replace('TRmgGridPoint size = m_map->getSize();\n    return size;',
            'TRmgGridPoint size;\n    return m_map->getSize(size);')
        assert replacement != body
        source = source.replace(body, replacement)
    edited['src/rmg.cpp'] = source
    terrain = edited['src/rmg_terrain.cpp']
    old = '    m_size = m_adapter->getSize();'
    assert terrain.count(old) == 1
    edited['src/rmg_terrain.cpp'] = terrain.replace(old,
        '    TRmgGridPoint size;\n    m_size = m_adapter->getSize(size);')
    runner = Path(__file__).with_name('run-rmg-size-output-family.py')
    payload = dict(schema=1, units=['rmg', 'rmg_support', 'rmg_terrain', 'scenarioinfo',
        'singleselectionpopups', 'singleselectionwindow', 'tiles'], evidence=__doc__,
        diagnostic_runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
        axes=[dict(name='size_output_ownership', source=files[0], find=originals[files[0]],
            options=[dict(name='value_control', replace=originals[files[0]]),
                dict(name='explicit_output_reference', replace=edited[files[0]],
                    extra_edits=[dict(source=name, find=originals[name], replace=edited[name])
                                 for name in files[1:]])])])
    if args.scoped:
        scoped = dict(edited)
        scoped['src/rmg_terrain.cpp'] = scoped['src/rmg_terrain.cpp'].replace(
            '    TRmgGridPoint size;\n    m_size = m_adapter->getSize(size);',
            '    {\n        TRmgGridPoint size;\n        m_size = m_adapter->getSize(size);\n    }')
        payload['axes'][0]['options'].append(dict(name='scoped_output_diagnostic',
            replace=scoped[files[0]], extra_edits=[dict(source=name,
                find=originals[name], replace=scoped[name]) for name in files[1:]]))
    if adopted:
        # Current authored source is always the unchanged first control.
        # The value-return interpretation and long output lifetime remain
        # meaningful opposite controls after the supported model is adopted.
        states = [('authored_canonical_value_helper' if canonical else 'authored_scoped_output_diagnostic', live),
                  ('value_control', originals)]
        if canonical or args.scoped:
            states.append(('unscoped_output_reference', edited))
        if canonical and args.scoped:
            states.append(('scoped_output_diagnostic', scoped))
        payload['axes'][0] = dict(name='size_output_ownership', source=files[0], find=live[files[0]],
            options=[dict(name=name, replace=state[files[0]],
                extra_edits=[dict(source=path, find=live[path], replace=state[path])
                             for path in files[1:]]) for name, state in states])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, loaded, axes = load_manifest(args.output, HOMM3_DIR)
    for choice in range(len(axes[0].options)):
        produced = render(loaded, axes, (choice,))
        expected = states[choice][1] if adopted else (originals if choice == 0 else (edited if choice == 1 else scoped))
        assert all(produced[name] == expected[name] for name in files)
    print('%d source models; one canonical map interface, one stack output at each of 3 callers' % len(axes[0].options))


if __name__ == '__main__':
    main()
