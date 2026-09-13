"""Test real scalar read-buffer scopes in the Complete campaign loader.

Retail 0x48a310 retains distinct homes for scenario days, score, completion
order and index (+0x757..+0x7ac), while its leading scalar reads reuse dead
parameter homes. The former reconstruction-only scalar helpers left separate
three-statement brace scopes in the caller. Compare those scopes with named
locals in their actual enclosing prefix/loop and compare separate scopes for
the two full-width score reads. Every local is read and used once, every
virtual read/assignment remains in order, and all canonical calls survive.
The earlier six-site width family produced no score change; this family
changes lifetimes, not read sizes, signedness or masked-value semantics.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/customcampaign.cpp'
PATTERN = re.compile(r'(?m)^( +)\{\n\1    (unsigned char|short) value;\n\1    infile->read\(&value, sizeof\(value\)\);\n\1    ([^\n]+);\n\1\}')
NAMES = ['cheaterByte', 'currentMapByte', 'campaignByte', 'regionByte',
         'crossoverByte', 'briefingByte', 'scenarioCountByte', 'completedByte',
         'completeOrderByte', 'scenarioIndexByte', 'poolCountByte',
         'heroCountByte', 'artifactCountWord', 'artifactIdWord', 'artifactExtraWord',
         'assignedCountByte']
GROUPS = [(0, 1, 2, 3, 4, 5), (7, 8, 9), (6, 10, 11, 12, 15), (13, 14)]


def original_body():
    spec = importlib.util.spec_from_file_location(
        'campaign_buffers', Path(__file__).with_name('generate-campaign-load-buffer-family.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.original_body()


def canonical_body(body):
    # Admit the retained named-local model only by exact family round-trip.
    canonical = body
    for name in NAMES:
        pattern = re.compile(r'(?m)^( +)(unsigned char|short) ' + name
            + r';\n\1infile->read\(&' + name + r', sizeof\(' + name
            + r'\)\);\n\1([^\n]+);')
        def restore(match):
            indent, typename, assignment = match.groups()
            return (indent + '{\n' + indent + '    ' + typename + ' value;\n'
                    + indent + '    infile->read(&value, sizeof(value));\n'
                    + indent + '    ' + assignment.replace(name, 'value')
                    + ';\n' + indent + '}')
        canonical = pattern.sub(restore, canonical)
    if body not in [candidate for _, candidate in _variants(canonical)]:
        raise ValueError('Current body is outside the reviewed lifetime family')
    return canonical


def variants(body):
    return _variants(canonical_body(body))


def _variants(body):
    matches = list(PATTERN.finditer(body))
    if len(matches) != len(NAMES):
        raise ValueError('Review campaign scalar read scopes')
    days = '''        int days;
        infile->read(&days, sizeof(days));
        scenario.m_days = days;
        int score;
        infile->read(&score, sizeof(score));
        scenario.m_score = score;'''
    if body.count(days) != 1:
        raise ValueError('Review full-width score read lifetimes')
    for choices in itertools.product(range(2), repeat=5):
        candidate = body
        selected = {index for group, choice in zip(GROUPS, choices) if choice for index in group}
        for index in reversed(range(len(matches))):
            if index not in selected:
                continue
            match = matches[index]
            indent, typename, assignment = match.groups()
            name = NAMES[index]
            replacement = (indent + typename + ' ' + name + ';\n'
                           + indent + 'infile->read(&' + name + ', sizeof(' + name + '));\n'
                           + indent + assignment.replace('value', name) + ';')
            candidate = candidate[:match.start()] + replacement + candidate[match.end():]
        if choices[4]:
            replacement = []
            for name in ('days', 'score'):
                replacement.append('        {\n            int ' + name + ';\n'
                    + '            infile->read(&' + name + ', sizeof(' + name + '));\n'
                    + '            scenario.m_' + name + ' = ' + name + ';\n        }')
            candidate = candidate.replace(days, '\n'.join(replacement))
        yield 'scopes-' + ''.join(map(str, choices)), candidate


def make_manifest():
    body = original_body()
    options = list(variants(body))
    assert len(options) == 32 and any(candidate == body for _, candidate in options)
    return {'schema': 1, 'source': SOURCE, 'units': ['customcampaign'], 'evidence': __doc__,
            'axes': [{'name': 'scalar-read-lifetimes', 'find': body, 'options': [
                {'name': 'unchanged'}] + [{'name': name, 'replace': candidate}
                                        for name, candidate in options if candidate != body]}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
