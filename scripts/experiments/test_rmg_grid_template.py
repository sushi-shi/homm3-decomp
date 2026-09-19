#!/usr/bin/env python3
"""Native semantic oracle for the actual two generated point definitions.

Exercises unsigned coordinate bounds, aliasing inputs, signed conversions,
modular translation, ordering and set insertion/find/erase. This is not a
claim about VC6 EH or register allocation; the retained-object audit owns it.
"""
import argparse
import os
import subprocess
import tempfile
from pathlib import Path

from homm3.vc6.source_families import load_manifest, render


def body(source, needle):
    start = source.index(needle)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


FIXTURE = r'''
#include <assert.h>
#include <stdint.h>
#include <set>
#define VA(a,b)
struct TPoint { int m_x,m_y; TPoint(int x,int y):m_x(x),m_y(y){} };
'''
CHECKS = r'''
int main() {
    assert(sizeof(TRmgGridPoint)==8);
    const unsigned int values[]={0,1,0x7fffffffU,0x80000000U,0xffffffffU};
    const int offsets[]={-2,-1,0,1,2,(-2147483647-1)};
    std::set<TRmgGridPoint> points;
    for(int i=4;i>=0;--i) for(int j=4;j>=0;--j) {
        TRmgGridPoint p(values[i],values[j]);
        assert(p.getX()==values[i] && p.getY()==values[j]);
        TRmgGridPoint alias(values[i],values[i]);
        assert(alias.getX()==values[i] && alias.getY()==values[i]);
        assert(points.insert(p).second);
        assert(!points.insert(p).second);
        TRmgGridPoint assigned;
        assigned.setY(values[j]); assigned.setX(values[i]);
        assert(assigned.getX()==values[i] && assigned.getY()==values[j]);
        for(int k=0;k<6;++k) {
            TRmgGridPoint moved=p;
            TRmgGridPoint* address=&(moved+=TPoint(offsets[k],offsets[5-k]));
            assert(address==&moved);
            assert(moved.getX()==(uint32_t)((uint64_t)values[i]+(uint32_t)offsets[k]));
            assert(moved.getY()==(uint32_t)((uint64_t)values[j]+(uint32_t)offsets[5-k]));
        }
        for(int a=0;a<5;++a) for(int b=0;b<5;++b) {
            TRmgGridPoint q(values[a],values[b]);
            uint64_t pk=((uint64_t)values[j]<<32)|values[i];
            uint64_t qk=((uint64_t)values[b]<<32)|values[a];
            assert((p<q)==(pk<qk));
        }
    }
    assert(points.size()==25);
    std::set<TRmgGridPoint>::iterator it=points.begin();
    for(int y=0;y<5;++y) for(int x=0;x<5;++x,++it) {
        assert(it!=points.end());
        assert(it->getX()==values[x] && it->getY()==values[y]);
    }
    assert(it==points.end());
    for(int x=0;x<5;++x) for(int y=0;y<5;++y) {
        TRmgGridPoint p(values[x],values[y]);
        assert(points.find(p)!=points.end());
        assert(points.erase(p)==1 && points.erase(p)==0);
    }
    for(int a=0;a<6;++a) for(int b=0;b<6;++b) {
        TPoint signedPoint(offsets[a],offsets[b]);
        TRmgGridPoint p(signedPoint);
        assert(p.getX()==(unsigned int)offsets[a]);
        assert(p.getY()==(unsigned int)offsets[b]);
        TPoint restored=p;
        assert(restored.m_x==offsets[a] && restored.m_y==offsets[b]);
    }
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path, nargs='?')
    parser.add_argument('--authored', action='store_true',
                        help='test the currently authored generic point definition')
    parser.add_argument('--generic-population', action='store_true',
                        help='all options of a one-axis manifest use the generic owner')
    args = parser.parse_args()
    if args.authored == (args.manifest is not None):
        parser.error('supply either a manifest or --authored')
    root = Path(os.environ['HOMM3_DIR'])
    if args.authored:
        populations = [(1, {name: (root / name).read_text()
                            for name in ('include/rmg.h', 'src/rmg_terrain.cpp')})]
    else:
        _, originals, axes = load_manifest(args.manifest, root)
        count = len(axes[0].options) if args.generic_population else 2
        populations = [(1 if args.generic_population else choice,
                        render(originals, axes, (choice,)))
                       for choice in range(count)]
    cases = []
    for number, (choice, sources) in enumerate(populations):
        header = sources.get('include/rmg.h', (root / 'include/rmg.h').read_text())
        source = sources.get('src/rmg_terrain.cpp')
        if source is None:
            source = (root / 'src/rmg_terrain.cpp').read_text()
        start = header.index('template<class Coordinate> struct TRmgCoordinatePoint {'
                             if choice else 'struct TRmgGridPoint {')
        end = header.index('\nstruct TRmgZoneBounds {', start)
        prefix = 'template<class Coordinate>\n' if choice else ''
        constructor = ('TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint(const TPoint& point)'
                       if choice else 'TRmgGridPoint::TRmgGridPoint(const TPoint& point)')
        comparator = ('bool operator<(const TRmgCoordinatePoint<Coordinate>& left,'
                      if choice else 'bool operator<(const TRmgGridPoint& left,')
        text = FIXTURE + header[start:end] + '\n' + prefix + body(source, constructor)
        text += '\n' + prefix + body(source, comparator) + '\n' + CHECKS
        cases.append((str(number), text, True))
    positive = cases[0][1]
    comparison_start = positive.rindex('bool operator<(')
    comparison = body(positive[comparison_start:], 'bool operator<(')
    signature = comparison[:comparison.index('{')]
    for name, expression in [
        ('wrong_order', 'left.m_y > right.m_y || (left.m_y == right.m_y && left.m_x < right.m_x)'),
        ('duplicate_equal', 'left.m_y < right.m_y || (left.m_y == right.m_y && left.m_x <= right.m_x)'),
    ]:
        wrong = signature + '{ return ' + expression + '; }'
        cases.append((name, positive.replace(comparison, wrong, 1), False))
    for name, old, new in [
        ('constructor_alias', 'm_y(newY)', 'm_y(newX)'),
        ('wrong_translation', 'm_y += offset.m_y', 'm_y += offset.m_x'),
    ]:
        assert old in positive
        cases.append((name, positive.replace(old, new, 1), False))
    with tempfile.TemporaryDirectory(prefix='rmg-grid-oracle-') as tmp:
        directory = Path(tmp)
        for name, text, expected in cases:
            source = directory / (name + '.cpp')
            binary = directory / name
            source.write_text(text)
            subprocess.run(['g++', '-std=c++98', '-O1', str(source), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True)
            assert (result.returncode == 0) == expected, (name, result.stderr.decode())
    print('%d actual source models pass; 4 negative controls rejected' % len(populations))


if __name__ == '__main__':
    main()
