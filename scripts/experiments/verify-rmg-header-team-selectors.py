#!/usr/bin/env python3
"""Check actual team-count normalization before the untouched assignment calls."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render


def extract(source):
    start = source.index('        if (!m_computerTeamCount)', source.index('void type_random_map_generator::writeMapHeader'))
    end = source.index('            assignRmgTeams(', start)
    return source[start:end] + '            return true;\n        }\n        return false;\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    _, originals, axes = load_manifest(args.manifest, HOMM3_DIR)
    if len(axes) != 1:
        raise ValueError('expected a single header selector axis')
    bodies = [extract(render(originals, axes, (i,))['src/rmg.cpp']) for i in range(len(axes[0].options))]
    good = len(bodies)
    lower = ('std::_cpp_max(teamCount, 2)' if 'std::_cpp_max(teamCount, 2)' in bodies[0]
             else 'max(m_humanTeamCount, 2)')
    upper = next(line for line in bodies[0].splitlines() if 'm_humanTeamCount = ' in line
                 and ('_cpp_min(' in line or '= min(' in line))
    for old, new in (
            ('if (!m_computerTeamCount)', 'if (m_computerTeamCount)'),
            (lower, lower.replace(', 2)', ', 3)')),
            ('m_humanTeamCount >= m_humanPlayerCount', 'm_humanTeamCount > m_humanPlayerCount'),
            (upper, upper.replace('_cpp_min(', '_cpp_max(').replace('= min(', '= max('))):
        if bodies[0].count(old) != 1:
            raise ValueError('stale negative control: ' + old)
        bodies.append(bodies[0].replace(old, new))
    program = [r'''
#include <cstdio>
#include <cstring>
namespace std {
template<class T> const T& _cpp_min(const T& a, const T& b) { return b < a ? b : a; }
template<class T> const T& _cpp_max(const T& a, const T& b) { return a < b ? b : a; }
}
#include "homm3_minmax.h"
struct Sink { int calls; bool bad;
    Sink(): calls(0), bad(false) {}
    void write(const void* value, unsigned size) { ++calls; if (size != 1 || *static_cast<const char*>(value) != 0) bad = true; }
};
struct Owner { int m_humanPlayerCount, m_computerPlayerCount, m_humanTeamCount, m_computerTeamCount; };
''']
    for i, body in enumerate(bodies):
        program += ['struct Generator' + str(i) + ' : Owner { bool normalize(Sink* outfile) {\n', body, '} };\n']
    program += [r'''
template<class Generator> bool check() {
    for (int humans = -1; humans <= 9; ++humans) for (int computers = -1; computers <= 9; ++computers)
    for (int humanTeams = -1; humanTeams <= 9; ++humanTeams) for (int computerTeams = -1; computerTeams <= 9; ++computerTeams) {
        int expectedHuman = humanTeams == 0 ? humans : humanTeams;
        int expectedComputer = computerTeams == 0 ? computers : computerTeams;
        if (computers == 0 && expectedHuman < 2) expectedHuman = 2;
        bool assign = expectedHuman < humans || expectedComputer < computers;
        if (assign) {
            if (expectedHuman < 1) expectedHuman = 1;
            if (expectedComputer < 1) expectedComputer = 1;
            if (expectedHuman > humans) expectedHuman = humans;
            if (expectedComputer > computers) expectedComputer = computers;
        }
        Generator actual;
        actual.m_humanPlayerCount = humans; actual.m_computerPlayerCount = computers;
        actual.m_humanTeamCount = humanTeams; actual.m_computerTeamCount = computerTeams;
        Sink sink;
        if (actual.normalize(&sink) != assign || actual.m_humanTeamCount != expectedHuman ||
            actual.m_computerTeamCount != expectedComputer || actual.m_humanPlayerCount != humans ||
            actual.m_computerPlayerCount != computers || sink.bad || sink.calls != !assign) return false;
    }
    return true;
}
int main() {
''']
    for i in range(len(bodies)):
        program.append('    if (' + ('!' if i < good else '') + 'check<Generator' + str(i) + '>()) { std::printf("failed form ' + str(i) + '\\n"); return 1; }\n')
    program.append('    std::puts("' + str(good) + ' team normalizations: 14641 cases each; four bad controls rejected");\n}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-header-team-selectors-') as folder:
        cpp, exe = Path(folder) / 'oracle.cpp', Path(folder) / 'oracle'
        cpp.write_text(''.join(program))
        subprocess.run(['g++', '-std=c++98', '-O2', '-I', str(HOMM3_DIR / 'include'), str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
