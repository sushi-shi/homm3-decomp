#!/usr/bin/env python3
"""Check the authored quick-town array-counter scan and four negative controls.

Uses the actual EGameResource declaration and constructor scan. Nine hash-gated
retail silo rows establish the two-output bound; 99 synthetic rows also exercise
zero, negative, gold, and every pair of nonzero resource columns.
"""
from pathlib import Path
import itertools
import re
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from homm3.core.common import load_image

source = (ROOT / 'src/quicktownwindow.cpp').read_text()
start = source.index('            EGameResource resource[3];')
end = source.index('\n\n            // DC94/95', start)
scan = source[start:end]
bonus_end = source.index('\n        sprintf(g_text', end)
indices = [int(x) for x in re.findall(r'resource\[(\d+)\], 0, 0, 0, 0x10', source[end:bonus_end])]
assert len(indices) == 3, indices
header = (ROOT / 'include/town.h').read_text()
start = header.index('enum EGameResource {')
enum = header[start:header.index('\n};', start) + 3]

image, _ = load_image()
rva = 0x688eb4 - image.image_base
section = image.section_of(rva)
assert section is not None
values = struct.unpack_from('<63i', image.blob(section), rva - section.rva)
retail_rows = [values[i:i + 7] for i in range(0, 63, 7)]
assert all(sum(x != 0 for x in row) <= 2 for row in retail_rows)
rows = [row for row in itertools.product((-1, 0, 1), repeat=7)
        if sum(x != 0 for x in row) <= 2]
assert len(rows) == 99
rows += retail_rows

old_order = '++resourceCount;\n                    resource[resourceCount] = resource[0];'
assert scan.count(old_order) == 1
negative_post = scan.replace(old_order, 'resource[resourceCount] = resource[0];\n                    ++resourceCount;')
negative_positive = scan.replace('if (siloIncome[resource[0]])', 'if (siloIncome[resource[0]] > 0)')
negative_gold = scan.replace('resource[0] <= GOLD', 'resource[0] < GOLD')
variants = [
    ('authored', scan, indices),
    ('wrong_first_output', scan, [0, indices[1], 0]),
    ('wrong_postincrement', negative_post, indices),
    ('wrong_positive_test', negative_positive, indices),
    ('wrong_gold_bound', negative_gold, indices),
]
code = '#include <vector>\n#include <cstdio>\n' + enum + '\n'
for name, body, slots in variants:
    code += f'''std::vector<int> {name}(const int* siloIncome) {{
{body}
    std::vector<int> out;
    if (resourceCount == 2) {{ out.push_back(resource[{slots[0]}]); out.push_back(resource[{slots[1]}]); }}
    else if (resourceCount == 1) out.push_back(resource[{slots[2]}]);
    return out;
}}
'''
code += 'int rows[][7] = {\n' + ',\n'.join('{' + ','.join(map(str, row)) + '}' for row in rows) + '\n};\n'
code += '''int main() {
    int rejected[4] = {};
    for (const auto& row : rows) {
        std::vector<int> expected;
        for (int column = 0; column != 7; ++column)
            if (row[column] != 0) expected.push_back(column);
        if (authored(row) != expected) return 1;
'''
for i, (name, _, _) in enumerate(variants[1:]):
    code += f'        rejected[{i}] += {name}(row) != expected;\n'
code += '''    }
    for (int count : rejected) if (!count) return 2;
    std::printf("108 input rows passed; four negative controls rejected (%d/%d/%d/%d cases)\\n",
                rejected[0], rejected[1], rejected[2], rejected[3]);
}
'''
with tempfile.TemporaryDirectory(prefix='homm3-quicktown-counter-') as tmp:
    cpp = Path(tmp) / 'counter.cpp'
    binary = Path(tmp) / 'counter'
    cpp.write_text(code)
    subprocess.run(['g++', '-std=c++17', '-O2', '-fsanitize=undefined,bounds',
                    '-fno-sanitize-recover=all', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
