#!/usr/bin/env python3
"""Recover the DC empty-label return and text-wrapping scope."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()

def function(text):
    start = text.index('void type_dialog_icon::set(EGameResource resource, long qualifier)\n{')
    return text[start:text.index('\n}\n', start) + 2]

body = function(source)
checkpoint = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in checkpoint['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('icon body does not equal a reproduced parent')

start = body.index('    if (m_text.length()) {\n')
tail = body[start:].splitlines()
assert tail[-2:] == ['    }', '}']
unwrapped = '\n'.join(line[4:] if line.startswith('    ') else line
                      for line in tail[1:-2])
replacement = body[:start] + '    if (!m_text.length())\n        return;\n\n' + unwrapped + '\n}'
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'empty-label-return-scope', 'find': body, 'options': [
        {'name': 'positive-length-encloses-tail'},
        {'name': 'empty-length-returns', 'replace': replacement},
    ]}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'DC5089 calls string::length; DC5090 branches through e585a to the epilogue. Its nested lexical scopes close at e585e/e5860 before max and the text scan. This supports an early empty-label return, rather than a positive-length scope enclosing all remaining work.',
        'The word loop at e587e and wrapping loop at e58f4 both have DC lexical depth0. Preserve their predicates, recorded helper calls, width-store/clamp boundary and line-count/pixel-height statements.',
        'Current retail difference is exactly two _Eos sites: trace budgets47/48 exceed callee46. This family tests the supported return/scope difference, without synthetic statements or inline controls.',
    ],
}, indent=2) + '\n')
