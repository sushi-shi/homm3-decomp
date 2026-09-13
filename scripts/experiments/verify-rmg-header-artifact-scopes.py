#!/usr/bin/env python3
"""Check the actual rendered reserved/artifact serialization regions.

Uses the authored bitset iterator with native bitsets. An iterator_traits
adapter supplies modern std::copy's compile-time interface; the game adapter
and its operations are unchanged. Trait layout and the rest of the header
are outside this reduced fixture. A stream callback changes artifact traits
and map version at the reserved write, verifying that later reads stay live.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


def region(source):
    helper = generator('generate-rmg-position-family.py')
    body = helper.definition(source, 'type_random_map_generator::writeMapHeader')
    start = body.index('    if (m_mapVersion >= 1) {\n        int intBuffer = 0;', body.index('setAvailableRmgHeroes('))
    end = body.index('    if (m_mapVersion >= 2) {\n        {\n            std::bitset<70>', start)
    result = body[start:end]
    if 'char reserved[31];' not in result:
        assert body.count('    char reserved[31];\n') == 1
        result = '    char reserved[31];\n' + result
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--snapshot', type=Path)
    args = parser.parse_args()
    root = args.snapshot or HOMM3_DIR
    _, originals, axes = load_manifest(args.manifest, root)
    assert len(axes) == 1
    forms = [region(render(originals, axes, (i,))['src/rmg.cpp'])
             for i in range(len(axes[0].options))]
    count = len(forms)
    artifact_bound = ('artifactBit < 144', 'artifactBit < 143') if 'artifactBit < 144' in forms[0] else (
        'artifactBit < disabledArtifacts.size()', 'artifactBit < disabledArtifacts.size() - 1')
    for old, new in (
            ('disabledArtifacts.set(63)', 'disabledArtifacts.set(62)'),
            ('m_comboType != -1', 'm_comboType > 0'),
            artifact_bound,
            ('disabledArtifacts, 129)', 'disabledArtifacts, 128)'),
            ('1 << (artifactBit & 7)', '1 << ((artifactBit + 1) & 7)')):
        assert forms[0].count(old) == 1
        forms.append(forms[0].replace(old, new))
    program = ['#include <algorithm>\n#include <bitset>\n#include <vector>\n#include <cstring>\n#include <cstdio>\n',
               (root / 'include/bitset_iterator.h').read_text(),
               (root / 'include/const_bitset_iterator.h').read_text(), r'''
namespace std {
template<size_t N> struct iterator_traits<bitset_iterator<N> > {
    typedef input_iterator_tag iterator_category;
    typedef bool value_type;
    typedef ptrdiff_t difference_type;
    typedef void pointer;
    typedef typename bitset<N>::reference reference;
};
template<size_t N> struct iterator_traits<TConstBitsetIterator<N> > {
    typedef input_iterator_tag iterator_category;
    typedef bool value_type;
    typedef ptrdiff_t difference_type;
    typedef void pointer;
    typedef bool reference;
};
}
struct Trait { int m_comboType; };
Trait g_artifactTraits[144];
struct Sink {
    int* version; int mutation;
    std::vector<unsigned char> data;
    std::vector<unsigned> sizes;
    Sink(int* v, int m) : version(v), mutation(m) {}
    void write(const void* input, unsigned size) {
        const unsigned char* bytes = static_cast<const unsigned char*>(input);
        sizes.push_back(size); data.insert(data.end(), bytes, bytes + size);
        if (size == 31 && mutation) {
            *version = (*version + 1) % 3;
            for (int i = 0; i != 144; ++i)
                g_artifactTraits[i].m_comboType = g_artifactTraits[i].m_comboType == -1 ? 0 : -1;
        }
    }
};
''']
    for i, body in enumerate(forms):
        program += ['struct Writer' + str(i) + ' { int m_mapVersion; void write(Sink* outfile) {\n', body, '} };\n']
    program += [r'''
template<class Writer> bool check() {
    for (int initialVersion = 0; initialVersion != 3; ++initialVersion)
    for (int pattern = 0; pattern != 320; ++pattern)
    for (int mutation = 0; mutation != 2; ++mutation) {
        int traits[144];
        const int comboValues[3] = {-2, 0, 7};
        for (int bit = 0; bit != 144; ++bit) {
            bool disabled = pattern < 144 ? bit == pattern :
                pattern < 288 ? bit != pattern - 144 : (bit * 7 + pattern) % 11 < 5;
            traits[bit] = g_artifactTraits[bit].m_comboType = disabled ? comboValues[bit % 3] : -1;
        }
        Writer owner; owner.m_mapVersion = initialVersion;
        Sink sink(&owner.m_mapVersion, mutation); owner.write(&sink);
        int version = mutation ? (initialVersion + 1) % 3 : initialVersion;
        std::vector<unsigned char> expected;
        std::vector<unsigned> sizes;
        if (initialVersion >= 1) { expected.insert(expected.end(), 4, 0); sizes.push_back(4); }
        if (initialVersion >= 2) { expected.push_back(0); sizes.push_back(1); }
        expected.insert(expected.end(), 31, 0); sizes.push_back(31);
        if (version >= 1) {
            unsigned bits = version >= 2 ? 144 : 129, bytes = (bits + 7) / 8;
            unsigned offset = expected.size(); expected.insert(expected.end(), bytes, 0); sizes.push_back(bytes);
            for (unsigned bit = 0; bit != bits; ++bit) {
                bool disabled = (traits[bit] != -1) != (mutation != 0);
                if (bit == 0 || bit == 63 || disabled)
                    expected[offset + bit / 8] += static_cast<unsigned char>(1u << (bit % 8));
            }
        }
        if (sink.data != expected || sink.sizes != sizes || owner.m_mapVersion != version) return false;
        for (int bit = 0; bit != 144; ++bit)
            if (g_artifactTraits[bit].m_comboType != (mutation ? (traits[bit] == -1 ? 0 : -1) : traits[bit])) return false;
    }
    return true;
}
int main() {
''']
    for i in range(len(forms)):
        program.append('if (' + ('!' if i < count else '') + 'check<Writer' + str(i) + '>()) { std::printf("failed form ' + str(i) + '\\n"); return 1; }\n')
    program.append('std::puts("' + str(count) + ' artifact regions: 1920 cases each; five bad controls rejected");\n}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-header-artifact-oracle-') as folder:
        cpp, exe = Path(folder) / 'oracle.cpp', Path(folder) / 'oracle'
        cpp.write_text(''.join(program))
        subprocess.run(['g++', '-std=c++98', '-O2', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
