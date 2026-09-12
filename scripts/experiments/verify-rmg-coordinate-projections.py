import argparse
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator

helper = generator('generate-rmg-position-family.py')
parser = argparse.ArgumentParser(description='Check canonical scalar/position lookup source projections and aliasing.')
parser.add_argument('manifests', type=Path, nargs='+')
args = parser.parse_args()
cases = []
for manifest in args.manifests:
    _, originals, axes = load_manifest(manifest, HOMM3_DIR)
    for index in range(len(axes[0].options)):
        cases.append(render(originals, axes, (index,)))
positive_count = len(cases)
program = ['#include <cstdio>\nstruct TRmgMapItem { int marker; };\nstatic TRmgMapItem cells[200000];\n']
checks = []

def block(header, name):
    start = header.index('struct ' + name + ' {')
    return header[start:header.index('\n};', start)+3]

for index in range(positive_count + 4):
    negative = index >= positive_count
    files = cases[0 if negative else index]
    header, source = files['include/rmg.h'], files['src/rmg.cpp']
    position = block(header, 'TRmgMapPosition')
    scalar = generator('generate-rmg-map-accessor-family.py').definition(header)
    value = helper.definition(source, 'type_random_map::getMapItem', parameters='TRmgMapPosition point')
    if negative:
        change = index - positive_count
        if change == 0:
            value = value.replace('point.m_x, point.m_y', 'point.m_y, point.m_x')
        elif change == 1:
            scalar = scalar.replace('z * m_size.m_y', 'z * m_size.m_x')
        elif change == 2:
            value = value.replace('point.m_z);', '0);')
        else:
            value = value.replace('    return', '    ++m_size.m_x;\n    return')
    name = 'Case' + str(index)
    program += ['namespace ' + name + ' {\n', block(header, 'TRmgVector'), '\n', block(header, 'TPoint'), '\n', position, '\n',
                helper.definition(source, 'TRmgMapPosition::TRmgMapPosition'), '\n',
                'struct Map { TRmgMapItem* m_mapItems; TRmgMapPosition m_size;\n', scalar, '\n',
                'TRmgMapItem* getMapItem(TRmgMapPosition point);\n};\n', value.replace('type_random_map::', 'Map::'), '\n}\n']
    checks.append(('' if negative else '!') + 'check<' + name + '::Map,' + name + '::TRmgMapPosition>()')
program.append(r'''
template<class Map, class Position> bool check() {
    Map map; map.m_mapItems = cells + 16;
    int sizes[] = {1,2,3,7,36,72,144};
    for(int wi=0;wi<7;++wi) for(int hi=0;hi<7;++hi) {
        int width=sizes[wi], height=sizes[hi];
        map.m_size=Position(width,height,2);
        int xs[]={0,width/2,width-1}, ys[]={0,height/2,height-1};
        for(int z=0;z<2;++z) for(int xi=0;xi<3;++xi) for(int yi=0;yi<3;++yi) {
            long expected=xs[xi];
            for(int plane=0;plane<z;++plane) expected+=long(width)*height;
            for(int row=0;row<ys[yi];++row) expected+=width;
            TRmgMapItem* base=map.m_mapItems;
            Position point(xs[xi],ys[yi],z);
            if(map.getMapItem(point)!=base+expected || map.getMapItem(xs[xi],ys[yi],z)!=base+expected) return false;
            if(map.m_mapItems!=base || map.m_size.m_x!=width || map.m_size.m_y!=height || map.m_size.m_z!=2) return false;
            if(point.m_x!=xs[xi] || point.m_y!=ys[yi] || point.m_z!=z) return false;
        }
        // A by-value query may be sourced from the dimension member itself.
        TRmgMapItem* base=map.m_mapItems;
        if(map.getMapItem(map.m_size)!=base+width+3L*height*width) return false;
        if(map.m_mapItems!=base || map.m_size.m_x!=width || map.m_size.m_y!=height || map.m_size.m_z!=2) return false;
    }
    return true;
}
int main() {
''')
for index, check in enumerate(checks):
    program.append('if ('+check+') { std::printf("failed case '+str(index)+'\\n"); return 1; }\n')
program.append('std::puts("'+str(positive_count)+' coordinate projection forms: 882 coordinates and 49 aliased dimension queries; four negative controls rejected");}\n')
with tempfile.TemporaryDirectory(prefix='rmg-coordinate-accessors-') as tmp:
    p=Path(tmp);(p/'oracle.cpp').write_text(''.join(program))
    subprocess.run(['g++','-std=c++98','-O2',str(p/'oracle.cpp'),'-o',str(p/'oracle')], check=True)
    subprocess.run([str(p/'oracle')],check=True)
