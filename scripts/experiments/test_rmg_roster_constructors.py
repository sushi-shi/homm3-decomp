#!/usr/bin/env python3
"""Check actual generated constructor bodies against independent field values.

Fixtures retain the actual scalar declarations, constructor parameters, base
initializers and bodies. Unrelated virtual operations are replaced by one
fixture marker. No claim about retail ABI, virtual dispatch, EH or inlining is
made here; VC6 retained-body and full-consumer comparisons establish those.
"""
import argparse
import os
import subprocess
import tempfile
from pathlib import Path

from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator


CHECKS = r'''
#include <assert.h>
static void base(const type_treasure_def& p,int object,int subtype,int value,int density) {
    assert(p.m_objectType==object && p.m_subtype==subtype);
    assert(p.m_value==value && p.m_density==density);
}
int main() {
    const int values[]={(-2147483647-1),-1,0,1,2147483647};
    for(int i=0;i<5;++i) for(int j=0;j<5;++j)
    for(int k=0;k<5;++k) for(int l=0;l<5;++l) {
        int a=values[i],b=values[j],c=values[k],d=values[l];
        type_treasure_def plain(a,b,c,d); base(plain,a,b,c,d);
        type_black_box_experience_def xp(a,b); base(xp,6,0,a,20);
        assert(xp.m_experience==b);
        type_black_box_gold_def gold(a,b); base(gold,6,0,a,5);
        assert(gold.m_gold==b);
        type_black_box_spells_def spells(a,b,c,d); base(spells,6,0,a,2);
        assert(spells.m_minimumLevel==b && spells.m_maximumLevel==c && spells.m_schoolMask==d);
        type_prison_def prison(a,b); base(prison,62,0,a,30);
        assert(prison.m_experience==b);
        type_quest_experience_def qxp(a,b,c); base(qxp,83,a,b,10);
        assert(qxp.m_experience==c);
        type_quest_gold_def qgold(a,b,c); base(qgold,83,a,b,10);
        assert(qgold.m_gold==c);
    }
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    root = Path(os.environ['HOMM3_DIR'])
    module = generator('generate-rmg-roster-constructor-family.py')
    _, originals, axes = load_manifest(args.manifest, root)
    cases = []
    for choice in range(len(axes[0].options)):
        sources = render(originals, axes, (choice,))
        header = sources['include/rmg.h']
        source = sources['src/rmg.cpp']
        start = header.index('class type_treasure_def {')
        end = header.index('\n    // Shared caller', start)
        text = header[start:end] + '\n    virtual int fixtureMarker() { return 0; }\n};\n'
        text += module.definition(source, 'type_treasure_def::type_treasure_def') + '\n'
        for kind in module.KINDS:
            text += module.class_prefix(header, kind) + '\n};\n'
        cases.append((str(choice), text + CHECKS, True))
    positive = cases[-1][1]
    for name, old, new in (
        ('base_density', 'm_density(newDensity)', 'm_density(newValue)'),
        ('experience_payload', 'm_experience(experience)', 'm_experience(value)'),
        ('spell_range', 'm_minimumLevel(minimumLevel)', 'm_minimumLevel(maximumLevel)'),
        ('gold_payload', 'm_gold(gold)', 'm_gold(0)'),
    ):
        assert old in positive
        cases.append((name, positive.replace(old, new, 1), False))
    with tempfile.TemporaryDirectory(prefix='rmg-roster-constructors-') as tmp:
        folder = Path(tmp)
        for name, text, expected in cases:
            source, binary = folder / (name + '.cpp'), folder / name
            source.write_text(text)
            subprocess.run(['g++', '-std=c++98', '-O1', str(source), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True)
            assert (result.returncode == 0) == expected, (name, result.stderr.decode())
    print('%d actual generated constructor sets pass 625 field scenarios each; 4 negative controls rejected'
          % (len(cases) - 4))


if __name__ == '__main__':
    main()
