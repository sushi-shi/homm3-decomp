#!/usr/bin/env python3
"""Compare allocation/registration traces for roster pointer-lifetime edits.

Factory spies record constructor identity and every argument; real std::vector
operations execute the rendered roster. Compare with the unchanged direct-new
source over map versions, creature filters, prototype counts and existing lists.
This verifies roster transformations, not the unchanged factory implementations.
"""
import argparse
import re
import subprocess
import tempfile
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifests', type=Path, nargs='+')
    parser.add_argument('--snapshot', type=Path, help='frozen source-family snapshot for historical manifests')
    args = parser.parse_args()
    helper = generator('generate-rmg-position-family.py')
    root = args.snapshot or HOMM3_DIR
    source = (root / 'src/rmg.cpp').read_text()
    original = helper.definition(source, 'type_random_map_generator::initializeObjectGenerators')
    bodies = [original]
    for manifest in args.manifests:
        _, originals, axes = source_families.load_manifest(manifest, root)
        if len(axes) != 1 or set(originals) != {'src/rmg.cpp'}:
            raise ValueError('expected a source-only roster axis')
        for index in range(len(axes[0].options)):
            rendered = source_families.render(originals, axes, (index,))['src/rmg.cpp']
            bodies.append(helper.definition(rendered, 'type_random_map_generator::initializeObjectGenerators'))
    bodies.append(helper.definition((HOMM3_DIR / 'src/rmg.cpp').read_text(),
                                    'type_random_map_generator::initializeObjectGenerators'))
    bodies = list(dict.fromkeys(bodies))
    positive = len(bodies)
    for before, after in (
            ('m_level >= 0', 'm_level > 0'),
            ('new type_black_box_experience_def(6000, 5000)', 'new type_black_box_experience_def(5000, 6000)'),
            ('m_disabledKeyTents[player] = 0;', 'm_disabledKeyTents[player] = 1;'),
            ('quest < m_objectPrototypes[83].size()', 'quest + 1 < m_objectPrototypes[83].size()')):
        if before not in original:
            raise ValueError('negative control anchor absent')
        bodies.append(original.replace(before, after))
    factories = {}
    for match in re.finditer(r'new (type_\w+)\(([^()]*)\)', original):
        kind, arguments = match.groups()
        count = len(arguments.split(',')) if arguments.strip() else 0
        if kind in factories and factories[kind] != count:
            raise ValueError('factory overload needs separate trace identity')
        factories[kind] = count
    base = 'type_treasure_def'
    names = [base] + sorted(name for name in factories if name != base)
    program = [r'''
#include <vector>
#include <cstdio>
struct type_treasure_def;
std::vector<type_treasure_def*> allocations;
struct Record {
    int tag, count, values[4];
    Record(): tag(-1), count(0) { for (int i=0;i<4;++i) values[i]=0; }
    bool operator==(const Record& b) const {
        if(tag!=b.tag || count!=b.count) return false;
        for(int i=0;i<4;++i) if(values[i]!=b.values[i]) return false;
        return true;
    }
};
''']
    for tag, name in enumerate(names):
        count = factories[name]
        arguments = ', '.join('int a' + str(i) for i in range(count))
        assignments = 'record.tag=%d; record.count=%d;' % (tag, count)
        assignments += ''.join('record.values[%d]=a%d;' % (i, i) for i in range(count))
        if name == base:
            program.append('struct ' + name + ' { Record record;\n'
                           'type_treasure_def() { allocations.push_back(this); }\n'
                           'virtual ~type_treasure_def() {}\n'
                           + name + '(' + arguments + ') { allocations.push_back(this); ' + assignments + '}\n};\n')
        else:
            program.append('struct ' + name + ': type_treasure_def { ' + name + '(' + arguments + ') { '
                           + assignments + '} };\n')
    program.append(r'''
struct Trait { int m_level; } g_creatureTypeTraits[145];
struct Root {
    int m_mapVersion;
    std::vector<int> m_objectPrototypes[84];
    std::vector<type_treasure_def*> m_objectGenerators;
    std::vector<unsigned char> m_disabledKeyTents;
};
struct Trace {
    std::vector<Record> allocated, registered;
    std::vector<unsigned char> disabled;
    bool operator==(const Trace& b) const {
        return allocated==b.allocated && registered==b.registered && disabled==b.disabled;
    }
};
template<class T> Trace run(int version,int pattern,int colors,int quests,int existing) {
    T subject;
    subject.m_mapVersion=version;
    subject.m_objectPrototypes[10].resize(colors);
    subject.m_objectPrototypes[83].resize(quests);
    subject.m_disabledKeyTents.assign(5, 0x7f);
    for(int i=0;i<145;++i)
        g_creatureTypeTraits[i].m_level=pattern==0 ? -1 : pattern==1 ? 0 : (i+pattern)%3-1;
    for(int i=0;i<existing;++i)
        subject.m_objectGenerators.push_back(new type_treasure_def(-10-i,20+i,30-i,40+i));
    subject.initializeObjectGenerators();
    Trace result;
    for(unsigned i=0;i<allocations.size();++i) result.allocated.push_back(allocations[i]->record);
    for(unsigned i=0;i<subject.m_objectGenerators.size();++i)
        result.registered.push_back(subject.m_objectGenerators[i]->record);
    result.disabled=subject.m_disabledKeyTents;
    for(unsigned i=0;i<allocations.size();++i) delete allocations[i];
    allocations.clear();
    return result;
}
''')
    for index, body in enumerate(bodies):
        name = 'Case' + str(index)
        program.append('struct ' + name + ': Root { void initializeObjectGenerators(); };\n'
                       + body.replace('type_random_map_generator::', name + '::') + '\n')
    program.append(r'''
template<class T> bool check() {
    for(int version=0;version<3;++version)
    for(int pattern=0;pattern<4;++pattern)
    for(int colors=0;colors<4;++colors)
    for(int quests=0;quests<3;++quests)
    for(int existing=0;existing<2;++existing)
        if(!(run<Case0>(version,pattern,colors,quests,existing)
             ==run<T>(version,pattern,colors,quests,existing))) return false;
    return true;
}
int main() {
''')
    for index in range(len(bodies)):
        program.append('if (' + ('!' if index < positive else '') + 'check<Case' + str(index)
                       + '>()) { std::fprintf(stderr,"failed case ' + str(index) + '\\n"); return 1; }\n')
    program.append('}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-roster-pointer-') as raw:
        path = Path(raw) / 'oracle.cpp'
        path.write_text(''.join(program))
        executable = Path(raw) / 'oracle'
        subprocess.run(['g++', '-std=c++98', '-O1', str(path), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print(positive, 'roster bodies passed 288 allocation/registration/key-state scenarios each; four incorrect controls rejected')


if __name__ == '__main__':
    main()
