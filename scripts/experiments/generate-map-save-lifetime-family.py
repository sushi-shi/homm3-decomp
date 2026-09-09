"""Restore the DC map-save result local and test Complete list lifetimes.

NewfullMap::Save dc:0xecdf8 (mapcell.cpp:759) names function-scope int count
and assigns each ordinary save helper's result before its negative check.
Retail 0x4fdf40 keeps those calls/expansions in the same order. The final
seer/quest loops are Complete source: they use this map's vectors, unlike
DC TSeerHut::SaveSeerList's global list and per-record failure checks.
Keep those instance-relative loops, their re-read size bounds and asymmetric
write/save error handling. Vary result assignment spelling, count-buffer
sharing, and real loop-index scopes without new helpers or inline pragmas.
"""
import argparse
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/mapcell.cpp'
SIGNATURE = 'int NewfullMap::save(TAbstractFile* outfile, int size, unsigned char twoLayers)'
CALLS = ['saveMapLayer(outfile, size, 0)', 'saveMapLayer(outfile, size, 1)',
         'saveMapObjects(outfile)', 'saveBlackBoxList(outfile)',
         'saveTreasureList(outfile)', 'saveMonsterList(outfile)', 'saveTimedEventList(outfile)']


def original_body():
    source = (ROOT / SOURCE).read_text()
    start = source.index(SIGNATURE + '\n{')
    return source[start:source.index('\n}', start) + 2]


def canonical_body(body):
    # The adopted separate-result/explicit-return state is a finite member,
    # not a new source model accepted by loose pattern matching.
    canonical = body
    if '    int count;\n' in canonical:
        canonical = canonical.replace('    int count;\n', '', 1)
        for call in CALLS:
            indent = '        ' if call.endswith('size, 1)') else '    '
            canonical = canonical.replace(indent + 'count = ' + call
                + ';\n' + indent + 'if (count < 0)', indent + 'if (' + call + ' < 0)')
        canonical = canonical.replace('    count = saveTownEventList(outfile);\n'
            + '    if (count < 0)\n        return -1;\n    return 0;',
            '    return saveTownEventList(outfile) >= 0 ? 0 : -1;')
    if body not in [candidate for _, candidate in _variants(canonical)]:
        raise ValueError('Current body is outside the reviewed map-save family')
    return canonical


def variants(body):
    return _variants(canonical_body(body))


def _variants(body):
    if body.count('        int count = ') != 2:
        raise ValueError('Review the Complete count buffers')
    for results, buffers, indices, tail in itertools.product(range(3), range(2), range(2), range(2)):
        candidate = body
        if results or buffers:
            candidate = candidate.replace('\n{\n', '\n{\n    int count;\n', 1)
        if results:
            for call in CALLS:
                indent = '        ' if call.endswith('size, 1)') else '    '
                old = indent + 'if (' + call + ' < 0)'
                if candidate.count(old) != 1:
                    raise ValueError('Review result boundary ' + call)
                if results == 1:
                    new = indent + 'count = ' + call + ';\n' + indent + 'if (count < 0)'
                else:
                    new = indent + 'if ((count = ' + call + ') < 0)'
                candidate = candidate.replace(old, new)
        if buffers:
            candidate = candidate.replace('        int count = ', '        count = ')
        if indices:
            marker = '    {\n        ' + ('count' if buffers else 'int count') + ' = m_seerHutList.size();'
            candidate = candidate.replace(marker, '    unsigned int i;\n' + marker, 1)
            candidate = candidate.replace('for (unsigned int i = 0;', 'for (i = 0;')
        old = '    return saveTownEventList(outfile) >= 0 ? 0 : -1;'
        if tail:
            new = ('    count = saveTownEventList(outfile);\n    if (count < 0)\n'
                   if results or buffers else '    if (saveTownEventList(outfile) < 0)\n')
            new += '        return -1;\n    return 0;'
        elif results:
            new = '    count = saveTownEventList(outfile);\n    return count >= 0 ? 0 : -1;'
        else:
            new = old
        candidate = candidate.replace(old, new)
        yield f'results-{results}-buffers-{buffers}-indices-{indices}-tail-{tail}', candidate


def make_manifest():
    body = original_body()
    options = list(variants(body))
    assert len(options) == 24 and any(candidate == body for _, candidate in options)
    return {'schema': 1, 'source': SOURCE, 'units': ['mapcell'], 'evidence': __doc__,
            'axes': [{'name': 'map-save-result-lifetimes', 'find': body, 'options': [
                {'name': 'unchanged'}] + [{'name': name, 'replace': candidate}
                                        for name, candidate in options if candidate != body]}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
