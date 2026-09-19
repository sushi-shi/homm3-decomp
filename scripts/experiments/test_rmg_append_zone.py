#!/usr/bin/env python3
"""Independent geometry/event oracle for actual appendZonePositions bodies.

Uses actual point constructor and ordinary zone getter/setter definitions.
Fixed 32-direction doubles and bounded integers avoid conversion overflow.
An independent enumerator predicts all candidate queries and accepted values,
including center/zone aliasing and a predicate that changes the position before
returning. Native arithmetic does not establish VC6 /Op rounding, ABI or EH.
"""
import argparse
import math
import subprocess
import tempfile
from pathlib import Path

from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


FIXTURE = r'''
#include <vector>
#include <cstdio>
struct TRmgMapPosition {
    int m_x,m_y,m_z;
    TRmgMapPosition() {}
    TRmgMapPosition(int newX,int newY,int newZ);
};
struct Slot { int m_size; };
struct TRmgZone {
    Slot* m_slot;
    TRmgMapPosition m_levelPosition;
    TRmgMapPosition getLevelPosition() const;
    void setLevelPosition(TRmgMapPosition position);
    SIZE_ACCESSOR
};
static bool same(const TRmgMapPosition& a,const TRmgMapPosition& b) {
    return a.m_x==b.m_x && a.m_y==b.m_y && a.m_z==b.m_z;
}
static bool accept(int event,int mode) {
    switch(mode) {
    case 0: return false;
    case 1: return true;
    case 2: return event<32;
    case 3: return event==32;
    case 4: return event>32;
    default: return event%3==1;
    }
}
static TRmgMapPosition changed(TRmgMapPosition p,int event,bool mutate) {
    if(mutate) { p.m_x+=101+event; p.m_y-=73+event; p.m_z=1-p.m_z; }
    return p;
}
struct type_random_map_generator {
    struct Map { int m_numberLevels; } m_map;
    int mode;
    bool mutate;
    std::vector<TRmgMapPosition> queried;
    bool canPlaceZone(TRmgZone* zone) {
        int n=queried.size();
        queried.push_back(zone->m_levelPosition);
        zone->m_levelPosition=changed(zone->m_levelPosition,n,mutate);
        return accept(n,mode);
    }
    DECLARATIONS
};
'''

CHECKS = r'''
typedef void(type_random_map_generator::*Method)(TRmgZone*,TRmgZone*,std::vector<TRmgMapPosition>&);
static bool check(Method method) {
    const int origins[3][2]={{-20,15},{0,0},{120,3}};
    const int sizes[3][2]={{0,0},{1,5},{9,2}};
    for(int origin=0;origin<3;++origin) for(int size=0;size<3;++size)
    for(int levels=1;levels<=2;++levels) for(int z=0;z<2;++z)
    for(int alias=0;alias<2;++alias) for(int mutate=0;mutate<2;++mutate)
    for(int mode=0;mode<6;++mode) {
        Slot centerSlot={sizes[size][0]}, zoneSlot={sizes[size][1]};
        TRmgZone center, separate;
        center.m_slot=&centerSlot; separate.m_slot=&zoneSlot;
        center.m_levelPosition=TRmgMapPosition(origins[origin][0],origins[origin][1],z);
        separate.m_levelPosition=TRmgMapPosition(-31,47,1-z);
        TRmgZone* zone=alias?&center:&separate;
        TRmgMapPosition start=center.m_levelPosition;
        int zoneSize=zone->m_slot->m_size;
        int radii[2]={centerSlot.m_size+zoneSize,
                      centerSlot.m_size>zoneSize?centerSlot.m_size:zoneSize};
        std::vector<TRmgMapPosition> expectedQueries,expectedAccepted,actualAccepted;
        TRmgMapPosition sentinel(-999,888,7);
        expectedAccepted.push_back(sentinel); actualAccepted.push_back(sentinel);
        // Independently enumerate first ring, opposite-level center, second ring.
        int total=levels==1?32:65;
        TRmgMapPosition finalPosition;
        for(int event=0;event<total;++event) {
            TRmgMapPosition p;
            if(event==32) p=TRmgMapPosition(start.m_x,start.m_y,1-start.m_z);
            else {
                int ring=event<32?0:1, direction=event<32?event:event-33;
                p.m_x=(int)((double)start.m_x+radii[ring]*g_rmgDirectionCosines[direction]);
                p.m_y=(int)((double)start.m_y+radii[ring]*g_rmgDirectionSines[direction]);
                p.m_z=ring?1-start.m_z:start.m_z;
            }
            expectedQueries.push_back(p);
            finalPosition=changed(p,event,mutate!=0);
            if(accept(event,mode)) expectedAccepted.push_back(finalPosition);
        }
        type_random_map_generator object;
        object.m_map.m_numberLevels=levels; object.mode=mode; object.mutate=mutate!=0;
        (object.*method)(&center,zone,actualAccepted);
        if(object.queried.size()!=expectedQueries.size() || actualAccepted.size()!=expectedAccepted.size()) return false;
        for(unsigned i=0;i<expectedQueries.size();++i)
            if(!same(object.queried[i],expectedQueries[i])) return false;
        for(unsigned i=0;i<expectedAccepted.size();++i)
            if(!same(actualAccepted[i],expectedAccepted[i])) return false;
        if(!same(zone->m_levelPosition,finalPosition)) return false;
        if(!alias && !same(center.m_levelPosition,start)) return false;
    }
    return true;
}
int main() {
    RUNS
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path, nargs='?')
    parser.add_argument('--authored', action='store_true',
                        help='check the current authored body without a manifest')
    args = parser.parse_args()
    if args.authored == (args.manifest is not None):
        parser.error('provide either a manifest or --authored')
    module = generator('generate-rmg-append-zone-family.py')
    root = module.HOMM3_DIR
    extract = generator('generate-rmg-position-family.py').definition
    if args.authored:
        bodies = [module.definition((root / 'src/rmg.cpp').read_text())]
    else:
        _, originals, axes = load_manifest(args.manifest, root)
        assert len(axes) == 1
        bodies = [module.definition(render(originals, axes, (n,))['src/rmg.cpp'])
                  for n in range(len(axes[0].options))]
    original = bodies[0]
    negatives = [
        ('wrong_coordinate', original.replace('radius * g_rmgDirectionSines[direction]',
                                              'radius * g_rmgDirectionCosines[direction]')),
        ('wrong_level', original.replace('int level = 1 - position.m_z;', 'int level = position.m_z;')),
        ('wrong_ring_extent', original.replace('direction < 32', 'direction < 31')),
        ('lost_accepted_reload', original.replace('candidates.push_back(zone->getLevelPosition());',
                                                'candidates.push_back(candidate);')),
        ('lost_query_level_branch', original.replace('m_map.m_numberLevels == 1',
                                                     'm_map.m_numberLevels == 2')),
    ]
    assert all(body != original for _, body in negatives)
    cases = [(str(n), body, True) for n, body in enumerate(bodies)]
    cases += [(name, body, False) for name, body in negatives]
    text = FIXTURE.replace('DECLARATIONS', '\n'.join(
        'void case%d(TRmgZone*,TRmgZone*,std::vector<TRmgMapPosition>&);' % n
        for n in range(len(cases))))
    source = (root / 'src/rmg.cpp').read_text()
    header = (root / 'include/rmg.h').read_text()
    size_start = header.index('    int getSize() const\n')
    size_end = header.index('\n    }', size_start) + len('\n    }')
    text = text.replace('SIZE_ACCESSOR', header[size_start:size_end])
    # Import the actual canonical selector and its by-value integer wrapper;
    # neither fixture-side std::max nor a hand-written substitute owns it.
    text += 'template<class T>\n' + extract(
        (root / 'include/DC_precompiledheaders.h').read_text(), 'cppMax') + '\n'
    text += extract((root / 'include/includes.h').read_text(), 'max') + '\n'
    # The ordinary constructor can migrate between these inferred TUs. Require
    # one actual owner rather than silently substituting a fixture definition.
    position_module = generator('generate-rmg-position-family.py')
    constructor_sources = [
        (path, (root / path).read_text())
        for path in ('src/rmg.cpp', 'src/rmg_support.cpp')]
    constructor_count = sum(len(position_module._source.find_definitions(
        content, 'TRmgMapPosition::TRmgMapPosition'))
        for _, content in constructor_sources)
    assert constructor_count == 1, 'expected exactly one map-position constructor owner'
    constructor_source = next(content for _, content in constructor_sources
                              if position_module._source.find_definitions(
                                  content, 'TRmgMapPosition::TRmgMapPosition'))
    text += extract(constructor_source, 'TRmgMapPosition::TRmgMapPosition') + '\n'
    for name in ('TRmgZone::getLevelPosition', 'TRmgZone::setLevelPosition'):
        text += extract(source, name) + '\n'
    for name, operation in (('Sines', math.sin), ('Cosines', math.cos)):
        values = [operation(2*math.pi*i/32) for i in range(32)]
        text += 'static const double g_rmgDirection%s[32]={%s};\n' % (
            name, ','.join(format(value, '.17g') for value in values))
    for n, (_, body, _) in enumerate(cases):
        # VC6's for-declaration scope extends through the enclosing function;
        # express that same scope explicitly for the native C++98 compiler.
        assert body.count('for (int direction = ') == 1
        body = body.replace('for (int direction = ', 'int direction;\n    for (direction = ')
        text += body.replace(module.NAME, 'type_random_map_generator::case%d' % n) + '\n'
    runs = '\n'.join('if(check(&type_random_map_generator::case%d)!=%s){std::printf("case %s failed\\n");return 1;}'
                     % (n, 'true' if expected else 'false', name)
                     for n, (name, _, expected) in enumerate(cases))
    text += CHECKS.replace('RUNS', runs)
    with tempfile.TemporaryDirectory(prefix='rmg-append-zone-oracle-') as tmp:
        cpp, binary = Path(tmp) / 'oracle.cpp', Path(tmp) / 'oracle'
        cpp.write_text(text)
        subprocess.run(['g++', '-std=c++98', '-O1', str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print('%d actual bodies pass 864 geometry/event scenarios each; %d negative controls rejected'
          % (len(bodies), len(negatives)))


if __name__ == '__main__':
    main()
