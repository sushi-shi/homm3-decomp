#!/usr/bin/env python3
"""Check rendered hero and magic mask regions with their actual scopes.

Imports setAvailableRmgHeroes and the actual edited header regions. Native
std::bitset uses size_t for its template parameter, so only the helper's
non-type parameter spelling is adapted from VC6's unsigned int to size_t.
The remaining player, artifact and header serialization is outside this test.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def regions(source):
    head = source.index('void type_random_map_generator::writeMapHeader')
    call = source.index('        setAvailableRmgHeroes(', head) if '        setAvailableRmgHeroes(' in source[head:] else source.index('setAvailableRmgHeroes(', head)
    start = source.rfind('    if (m_mapVersion >= 1) {', head, call)
    end = source.index('    if (m_mapVersion >= 1) {\n        int intBuffer = 0;', call)
    heroes = source[start:end]
    start = source.index('std::bitset<70> disabledSpells;', end)
    start = source.rfind('    if (m_mapVersion >= 2) {', end, start)
    end = source.index('\n}\n', start)
    return heroes, source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--snapshot', type=Path, help='use a recorded search snapshot after source adoption')
    args = parser.parse_args()
    source_root = args.snapshot or HOMM3_DIR
    _, originals, axes = load_manifest(args.manifest, source_root)
    if len(axes) != 1:
        raise ValueError('expected one mask lifetime axis')
    pairs = [regions(render(originals, axes, (i,))['src/rmg.cpp']) for i in range(len(axes[0].options))]
    count = len(pairs)
    heroes, magic = pairs[0]
    for old, new in (
            ('heroBit < 156', 'heroBit < 155'),
            ('1 << (heroBit & 7)', '1 << ((heroBit + 1) & 7)'),
            ('m_disabledHeroes + 128', 'm_disabledHeroes + 127')):
        assert heroes.count(old) == 1
        pairs.append((heroes.replace(old, new), magic))
    assert magic.count('hero < 156') == 1
    pairs.append((heroes, magic.replace('hero < 156', 'hero < 155')))
    helper = generator('generate-rmg-position-family.py')
    setup = helper.definition((source_root / 'src/rmg.cpp').read_text(), 'setAvailableRmgHeroes')
    program = ['#include <bitset>\n#include <vector>\n#include <cstring>\n#include <cstdio>\n',
               'template<size_t N>\n', setup, r'''
struct Sink {
    std::vector<unsigned char> data;
    std::vector<unsigned> sizes;
    void write(const void* input, unsigned size) {
        const unsigned char* bytes = static_cast<const unsigned char*>(input);
        sizes.push_back(size); data.insert(data.end(), bytes, bytes + size);
    }
};
struct Owner { int m_mapVersion; unsigned char m_disabledHeroes[156]; };
''']
    for i, (heroes, magic) in enumerate(pairs):
        program += ['struct Generator' + str(i) + ' : Owner {\n    void heroes(Sink* outfile) {\n',
                    heroes, '    }\n    void magic(Sink* outfile) {\n', magic, '    }\n};\n']
    program += [r'''
template<class Generator> bool check() {
    for (int version = 0; version != 3; ++version) for (int pattern = 0; pattern != 348; ++pattern) {
        Generator owner; owner.m_mapVersion = version;
        for (int i = 0; i != 156; ++i) {
            bool disabled = pattern < 156 ? i == pattern :
                pattern < 312 ? i != pattern - 156 : ((i * 13 + pattern) % 7 < 3);
            owner.m_disabledHeroes[i] = disabled ? ((i + pattern) % 2 ? 255 : 2) : 0;
        }
        unsigned char before[156]; std::memcpy(before, owner.m_disabledHeroes, 156);
        Sink heroSink; owner.heroes(&heroSink);
        unsigned count = version ? 156 : 128, bytes = (count + 7) / 8;
        std::vector<unsigned char> expected(bytes, 0);
        for (unsigned bit = 0; bit != count; ++bit)
            if (!before[bit]) expected[bit / 8] += static_cast<unsigned char>(1u << (bit % 8));
        if (heroSink.data != expected || heroSink.sizes != std::vector<unsigned>(1, bytes) ||
            std::memcmp(before, owner.m_disabledHeroes, 156)) return false;
        Sink magicSink; owner.magic(&magicSink);
        expected.assign(version >= 2 ? 9 + 4 + 156 : 0, 0);
        std::vector<unsigned> sizes;
        if (version >= 2) { sizes.push_back(9); sizes.push_back(4); sizes.insert(sizes.end(), 156, 1); }
        if (magicSink.data != expected || magicSink.sizes != sizes) return false;
    }
    return true;
}
int main() {
''']
    for i in range(len(pairs)):
        program.append('    if (' + ('!' if i < count else '') + 'check<Generator' + str(i) + '>()) { std::printf("failed form ' + str(i) + '\\n"); return 1; }\n')
    program.append('    std::puts("' + str(count) + ' mask regions: 1044 cases each; four bad controls rejected");\n}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-header-mask-oracle-') as folder:
        cpp, exe = Path(folder) / 'oracle.cpp', Path(folder) / 'oracle'
        cpp.write_text(''.join(program))
        subprocess.run(['g++', '-std=c++98', '-O2', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
