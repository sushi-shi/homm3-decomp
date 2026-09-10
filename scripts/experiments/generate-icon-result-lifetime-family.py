#!/usr/bin/env python3
"""Dialog icon sprite-result and line-count lifetimes."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()
a = source.index('void type_dialog_icon::set(EGameResource resource, long qualifier)\n{')
body = source[a:source.index('\n}\n', a) + 2]
image = '''    CSprite* image = ResourceManager::getSprite(m_spriteName.c_str());
    m_spriteWidth = image->getWidth() + 2;
    m_spriteHeight = image->getHeight() + 2;
    image->dispose();'''
reference = '''    CSprite& image = *ResourceManager::getSprite(m_spriteName.c_str());
    m_spriteWidth = image.getWidth() + 2;
    m_spriteHeight = image.getHeight() + 2;
    image.dispose();'''
line = '    int lines = g_unnamed698a08->lineLength(m_text.c_str(), m_textWidth);'
assert body.count(image) == body.count(line) == 1
options = []
for choices in itertools.product(range(2), repeat=3):
    result_reference, line_long, line_scope = choices
    candidate = body.replace(image, reference) if result_reference else body
    typ = 'long' if line_long else 'int'
    replacement = line.replace('int lines', typ + ' lines')
    if line_scope:
        candidate = candidate.replace('    m_resource = resource;', '    ' + typ + ' lines;\n\n    m_resource = resource;')
        replacement = replacement.replace(typ + ' lines =', 'lines =')
    candidate = candidate.replace(line, replacement)
    row = {'name': '-'.join(map(str, choices))}
    if any(choices): row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'sprite-result-line-count-lifetime', 'find': body, 'options': options}],
    'evidence': [
        'The complete fresh DC/retail pass confirms the stored resource/qualifier accesses and current morale/luck assignment order. They are fixed throughout this family.',
        'DC5082 binds the actual GetSprite result in R10;5084/85 calls dimensions and5086 disposes that same object. Test pointer versus reference source binding without changing the canonical calls or introducing an extra object.',
        'DC5118 returns the line count in R4;5119 consumes it in textHeight and5121 tests it in the growth loop, with another result at5125. Both signed int and long have the retail32-bit representation; no DC local name/type survives, so this is an explicit hypothesis, not recovered type evidence.',
        'Compare the line-count declaration at its5118 initialization with an earlier declaration while preserving its initialization at the same stage. Missing source rows permit but do not prove declaration placement.',
        'Eight finite states; no synthetic operations, alternate helper declarations, forced inlining or shared-header edits. All45 exact kb siblings must hold.',
    ],
}, indent=2) + '\n')
