#!/usr/bin/env python3
"""Value query snapshots preserve a natural named result in the painter.

The helper owns the output-reference temporary; the caller consumes its returned
value either directly or through a named snapshot. Copy initialization, const
snapshot and default construction followed by assignment are meaningful lifetime
models. No artificial caller block or unsupported inline keyword is introduced.
"""
import argparse
import json
from pathlib import Path
import sys
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from experiments._support import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    parent = generator('generate-rmg-size-helper-family.py')
    with tempfile.TemporaryDirectory(prefix='rmg-size-storage-parent-') as directory:
        path = Path(directory) / 'parent.json'
        saved = sys.argv
        try:
            sys.argv = [str(parent.__file__), str(path)]
            parent.main()
        finally:
            sys.argv = saved
        _, originals, axes = load_manifest(path, HOMM3_DIR)
        named = {option.name: render(originals, axes, (index,))
                 for index, option in enumerate(axes[0].options)}
        control = named['scoped_diagnostic']
        assignment = named['canonical_value_helper_assignment']
        authored = render(originals, axes, (0,))
    states = [('scoped_diagnostic', control), ('value_helper_assignment', assignment)]
    query = '    m_size = m_adapter->getSize();'
    for name, replacement in (
        ('named_value_snapshot', '    TRmgGridPoint size = m_adapter->getSize();\n    m_size = size;'),
        ('const_value_snapshot', '    const TRmgGridPoint size = m_adapter->getSize();\n    m_size = size;'),
        ('assigned_value_snapshot', '    TRmgGridPoint size;\n    size = m_adapter->getSize();\n    m_size = size;')):
        state = dict(assignment)
        assert state['src/rmg_terrain.cpp'].count(query) == 1
        state['src/rmg_terrain.cpp'] = state['src/rmg_terrain.cpp'].replace(query, replacement)
        states.append((name, state))
    states.sort(key=lambda item: item[1] != authored)
    assert states[0][1] == authored
    control = authored
    primary = 'include/rmg.h'
    payload = dict(schema=1, units=generator('generate-rmg-map-accessor-family.py').UNITS,
        evidence=__doc__, axes=[dict(name='size_storage_ownership', source=primary, find=control[primary],
            options=[dict(name=name, replace=state[primary], extra_edits=[
                dict(source=path, find=control[path], replace=state[path])
                for path in control if path != primary]) for name, state in states])])
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    _, originals, axes = load_manifest(args.output, HOMM3_DIR)
    for i, (_name, state) in enumerate(states):
        assert render(originals, axes, (i,)) == state
    print('five states: scoped control, value helper, named/const/assigned returned-value snapshots')


if __name__ == '__main__':
    main()
