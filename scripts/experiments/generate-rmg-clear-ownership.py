#!/usr/bin/env python3
"""Compare copied, borrowed and direct packed fields in map-cell reset.

Retail 0x530f10 loads the connection word after pointer-vector erase and
reuses EDI from its copy loop. The prior pre-erase snapshot needs EBX in
that loop. Earlier declaration/read-order families did not recover the
retail allocation. Test independent field ownership and whether each group
of updates occurs together or immediately before its final member store.
Borrowed/direct groups have no copy-back or self-assignment. Preserve the
real pointer-vector erase, every named bitfield update, all preserved bits,
and the movement/zone/previous-position stores. No Dreamcast counterpart.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def make_axes(source):
    helper = generator('generate-rmg-position-family.py')
    original = helper.definition(source, 'TRmgMapItem::clear')
    previous = generator('generate-rmg-clear-lifetime-family.py')
    fields = previous.SNAPSHOTS
    control = previous.copied_control(original)
    start = control.index('    connection.m_present = 0;')
    end = control.index('    m_connection = connection;')
    updates = control[start:end]
    groups = [''.join(line + '\n' for line in updates.splitlines()
                      if line.startswith('    ' + local + '.'))
              for _, local, _ in fields]
    assert ''.join(groups) == updates
    prefix = control[:control.index('\n{\n') + 3]
    tail = control[end:]
    connection = '    TRmgConnectionDecoration connection = m_connection;\n'
    erase = '    m_objects.erase(m_objects.begin(), m_objects.end());\n'
    early = control.replace(connection, '').replace(erase, connection + erase)
    variants = [('three_copies_early_connection', early)]
    for ownership, timing in itertools.product(
            itertools.product(('copy', 'reference', 'member'), repeat=3),
            ('together', 'at_store')):
        body = prefix + '    m_objects.erase(m_objects.begin(), m_objects.end());\n'
        selected = []
        for (kind, local, member), mode, group in zip(fields, ownership, groups):
            if mode != 'member':
                body += '    ' + kind + ('& ' if mode == 'reference' else ' ') + local + ' = ' + member + ';\n'
            selected.append(group.replace(local + '.', member + '.') if mode == 'member' else group)
        body += '\n'
        if timing == 'together':
            body += ''.join(selected)
        stores = tail
        for (_, local, member), mode, group in zip(fields, ownership, selected):
            store = '    ' + member + ' = ' + local + ';\n'
            assert stores.count(store) == 1
            replacement = group if timing == 'at_store' else ''
            if mode == 'copy':
                replacement += store
            stores = stores.replace(store, replacement)
        body += stores
        variants.append(('+'.join(ownership) + '+' + timing, body))
    return [helper.axis('clear_field_ownership', 'src/rmg.cpp', original, variants)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / 'src/rmg.cpp').read_text())
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=axes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    load_manifest(args.output, HOMM3_DIR)
    print(len(axes[0]['options']), 'map-cell packed-field ownership controls')


if __name__ == '__main__':
    main()
