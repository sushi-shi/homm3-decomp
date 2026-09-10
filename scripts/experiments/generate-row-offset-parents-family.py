"""Retain reproduced row-offset parents and combine independent gains."""
import itertools
import json
from pathlib import Path
import sys

checkpoint = Path(sys.argv[1])
root = checkpoint.parent
data = json.loads(checkpoint.read_text())
manifest = json.loads((root / 'input.json').read_text())
source_path = manifest['source']
source = Path(source_path).read_text()
assert source == (root / 'snapshot' / source_path).read_text()
if manifest['units'] == ['bitmap16']:
    assert [axis['name'] for axis in manifest['axes']] == [
        'draw', 'grab', 'fill', 'frame', 'darken', 'mask', 'colorize']
    combinations = [(0, grab, 0, 0, darken, 0, colorize)
        for grab, darken, colorize in itertools.product([0, 2], [0, 1], [0, 1])]
elif manifest['units'] == ['cspriteframe']:
    assert [axis['name'] for axis in manifest['axes']] == [
        'draw', 'drawCreatureImpl', 'drawAdvObjImpl', 'drawAdvObjWithFlagAlpha',
        'drawAdvObjShadowImpl', 'drawTile', 'drawTileShadow', 'drawSpellEffect']
    combinations = [(1,) * 8, (2,) * 8]
else:
    raise ValueError('No reviewed combination for these units')
for header in (root / 'snapshot' / 'include').rglob('*.h'):
    assert header.read_bytes() == Path(header.relative_to(root / 'snapshot')).read_bytes()

def materialize(choices):
    result = source
    for axis, choice in zip(manifest['axes'], choices):
        assert result.count(axis['find']) == 1
        result = result.replace(axis['find'], axis['options'][choice].get('replace', axis['find']))
    return result

options = [{'name': 'adopted-safe-control'}]
seen = {(0,) * len(manifest['axes'])}
for parent in data['elites']:
    choices = tuple(parent['choices'])
    result = materialize(choices)
    for repeat in ['first', 'repeat']:
        assert result == (root / 'candidates' / parent['id'] / repeat / 'tree' / source_path).read_text()
    if choices not in seen:
        options.append({'name': 'parent-' + parent['id'], 'replace': result})
        seen.add(choices)
for choices in combinations:
    if choices not in seen:
        options.append({'name': 'combine-' + '-'.join(map(str, choices)),
                        'replace': materialize(choices)})
        seen.add(choices)
Path(sys.argv[2]).write_text(json.dumps({'schema': 1, 'source': source_path,
    'units': manifest['units'], 'axes': [{'name': 'reproduced-offset-parents',
    'find': source, 'options': options}]}, indent=2) + '\n')
print('Wrote', len(options), 'verified parent/combined forms')
