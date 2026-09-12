"""Recover SCampaign::load's independently evidenced widened read buffers.

Retail 0x48a310 has no Dreamcast counterpart. Its modern arm reads one
byte into a dword then masks 0xff for campaign id, scenario/pool/assigned
counts and hero count; artifact count reads two bytes then masks 0xffff.
Narrow locals can also produce that mask, so the instructions alone do
not prove their source types. Explore those six sites independently,
keeping their separate scopes, every virtual
read, promotion, vector operation, and implicit legacy constructor intact.
No wrapper or inline pragma is introduced. Partial high bytes are discarded
by the exact retail mask; stream read sizes stay one or two bytes.
"""
import argparse
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/customcampaign.cpp'
SIGNATURE = 'void SCampaign::load(TAbstractFile* infile, int saveVersion)'


def original_body():
    source = (ROOT / SOURCE).read_text()
    start = source.index(SIGNATURE + '\n{')
    return source[start:source.index('\n}', start) + 2]


def variants(body):
    replacements = []
    for destination in ('m_currentCampaign', 'count', 'heroCount', 'artifactCount'):
        indent = '        ' if destination in ('m_currentCampaign', 'count') else '            '
        short = destination == 'artifactCount'
        old = (indent + ('short' if short else 'unsigned char') + ' value;\n'
               + indent + 'infile->read(&value, sizeof(value));\n'
               + indent + destination + ' = '
               + ('static_cast<unsigned short>(value)' if short else 'value') + ';')
        new = (indent + 'int value;\n' + indent + 'infile->read(&value, '
               + ('2' if short else '1') + ');\n' + indent + destination
               + ' = value & ' + ('0xffff' if short else '0xff') + ';')
        expected = 3 if destination == 'count' else 1
        if body.count(old) != expected:
            raise ValueError('Review read-buffer anchors for ' + destination)
        offset = 0
        for number in range(expected):
            start = body.index(old, offset)
            replacements.append((start, old, new, destination + '-' + str(number)))
            offset = start + len(old)
    replacements.sort()
    for choices in itertools.product(range(2), repeat=len(replacements)):
        candidate = body
        for choice, (start, old, new, _) in reversed(list(zip(choices, replacements))):
            if choice:
                candidate = candidate[:start] + new + candidate[start + len(old):]
        yield ('buffers-' + ''.join(map(str, choices)), candidate)


def make_manifest():
    body = original_body()
    options = list(variants(body))
    assert len(options) == 64 and options[0][1] == body
    return {'schema': 1, 'source': SOURCE, 'units': ['customcampaign'], 'evidence': __doc__,
            'axes': [{'name': 'widened-read-buffers', 'find': body, 'options': [
                {'name': 'unchanged'}] + [{'name': name, 'replace': candidate}
                                        for name, candidate in options[1:]]}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
