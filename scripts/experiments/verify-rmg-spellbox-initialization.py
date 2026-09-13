#!/usr/bin/env python3
"""Check rendered spell-box constructors, including their actual base stores.

The actual class declarations and both constructor definitions are imported.
Unused virtual factories get fixture stubs solely to complete native vtables;
this checks the seven initialized fields, not virtual factory behavior or ABI.
"""
import argparse
import subprocess
import tempfile
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--snapshot', type=Path)
    args = parser.parse_args()
    root = args.snapshot or HOMM3_DIR
    _, originals, axes = source_families.load_manifest(args.manifest, root)
    if len(axes) != 1 or set(originals) != {'include/rmg.h'}:
        raise ValueError('expected one header-only constructor axis')
    headers = [source_families.render(originals, axes, (index,))['include/rmg.h']
               for index in range(len(axes[0].options))]
    headers.append((HOMM3_DIR / 'include/rmg.h').read_text())
    headers = list(dict.fromkeys(headers))
    positive = len(headers)
    model = generator('generate-rmg-spellbox-initialization.py')
    original = model.constructor(headers[0])
    for before, after in (
            ('this->m_minimumLevel = minimumLevel;', 'this->m_minimumLevel = maximumLevel;'),
            ('this->m_schoolMask = schoolMask;', 'this->m_schoolMask = schoolMask ^ 1;'),
            ('type_treasure_def(6, 0, value, 2)', 'type_treasure_def(6, 0, value, 3)'),
            ('type_treasure_def(6, 0, value, 2)', 'type_treasure_def(6, 0, minimumLevel, 2)')):
        if before not in original:
            raise ValueError('negative control anchor absent')
        headers.append(headers[0].replace(original, original.replace(before, after)))
    helper = generator('generate-rmg-position-family.py')
    base = helper.definition((root / 'src/rmg.cpp').read_text(), 'type_treasure_def::type_treasure_def')
    program = ['#include <cstdio>\n#include <climits>\n']
    for index, header in enumerate(headers):
        def block(name):
            start = header.index('class ' + name + ' :') if name != 'type_treasure_def' else header.index('class type_treasure_def {')
            return header[start:header.index('\n};', start) + 3]
        program.append('namespace Case' + str(index) + ' {\n'
                       'struct TRmgObjectPropertiesRef; class type_random_map_generator; struct TRmgZone; class type_object;\n'
                       + block('type_treasure_def') + '\n' + block('type_black_box_spells_def') + '\n' + base + '\n')
        program.append(r'''
type_object* type_treasure_def::generate(TRmgObjectPropertiesRef*, type_random_map_generator*, TRmgZone*) { return 0; }
int type_treasure_def::getValue(TRmgZone*, type_random_map_generator*) { return 0; }
unsigned char type_treasure_def::isTerrainDependent() { return 0; }
type_object* type_black_box_spells_def::generate(TRmgObjectPropertiesRef*, type_random_map_generator*, TRmgZone*) { return 0; }
bool check() {
    const int values[]={INT_MIN,-1,0,1,15000,30000,INT_MAX};
    const int levels[]={-1,0,1,2,5,6};
    const int schools[]={0,1,2,4,8,15,16,255};
    for(int v=0;v<7;++v) for(int low=0;low<6;++low)
    for(int high=0;high<6;++high) for(int mask=0;mask<8;++mask) {
        type_black_box_spells_def object(values[v],levels[low],levels[high],schools[mask]);
        if(object.m_objectType!=6 || object.m_subtype!=0 || object.m_value!=values[v]
           || object.m_density!=2 || object.m_minimumLevel!=levels[low]
           || object.m_maximumLevel!=levels[high] || object.m_schoolMask!=schools[mask]) return false;
    }
    return true;
}
}
''')
    program.append('int main() {\n')
    for index in range(len(headers)):
        program.append('if (' + ('!' if index < positive else '') + 'Case' + str(index)
                       + '::check()) { std::fprintf(stderr,"failed case ' + str(index) + '\\n"); return 1; }\n')
    program.append('}\n')
    with tempfile.TemporaryDirectory(prefix='rmg-spellbox-') as raw:
        path = Path(raw) / 'oracle.cpp'
        path.write_text(''.join(program))
        executable = Path(raw) / 'oracle'
        subprocess.run(['g++', '-std=c++98', '-O1', str(path), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print(positive, 'constructor models passed 2016 field-value cases each; four incorrect controls rejected')


if __name__ == '__main__':
    main()
