"""Compare actual noise-record models by sample lattice, masks and RNG traces."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render
from homm3.vc6.test_rmg_families import generator

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('manifest',type=Path)
args=parser.parse_args()
_, originals, axes=load_manifest(args.manifest,HOMM3_DIR)
helper=generator('generate-rmg-position-family.py')
cases=[render(originals,axes,(index,)) for index in range(len(axes[0].options))]
positive_count=len(cases)
# Constructor mapping, quadrant sample, variation, and RNG consumption controls.
wrong=dict(cases[1]);wrong['src/rmg.cpp']=wrong['src/rmg.cpp'].replace('m_minYValue(minYValue)','m_minYValue(maxYValue)',1);cases.append(wrong)
for before,after in (('part.m_corners[0] = centerValue;','part.m_corners[0] = midpoints.m_minXValue;'),
                     ('patch.m_variation = (range - 1) / 2 + 1;','patch.m_variation = range / 2 + 1;'),
                     ('center += rand() % range - half;','center += rand() % range - half; rand();')):
    wrong=dict(cases[0]);assert before in wrong['src/rmg.cpp'];wrong['src/rmg.cpp']=wrong['src/rmg.cpp'].replace(before,after,1);cases.append(wrong)
program=['#include <vector>\n#include <algorithm>\n#include <cstdio>\nusing std::min; using std::max;\n#define __fastcall\nunsigned int randomState;\nstd::vector<int> draws;\nint scriptedRand() { randomState=randomState*214013U+2531011U; int value=(randomState>>16)&32767; draws.push_back(value); return value; }\n#define rand scriptedRand\n']
base_oracle=(HOMM3_DIR/'scripts/experiments/rmg-noise-midpoint-oracle.cpp').read_text()
lattice=base_oracle[base_oracle.index('typedef void'):base_oracle.index('// @CANDIDATES@')]

def block(header,name):
    start=header.index('struct '+name+' {')
    return header[start:header.index('\n};',start)+3]

for index,files in enumerate(cases):
    header,source=files['include/rmg.h'],files['src/rmg.cpp']
    types='\n'.join(block(header,name) for name in ('TRmgVector','TPoint','TRmgZoneBounds','TRmgNoiseRegion','TRmgNoiseMidpoints'))
    constructor=''
    test=lattice
    if 'TRmgNoiseMidpoints::TRmgNoiseMidpoints' in source:
        constructor=helper.definition(source,'TRmgNoiseMidpoints::TRmgNoiseMidpoints')
        before='''        TRmgNoiseMidpoints mids;
        mids.m_minYValue = -53; mids.m_minXValue = 37;
        mids.m_maxYValue = 71; mids.m_maxXValue = -19;'''
        after='''        TRmgNoiseMidpoints mids(-53, 37, 71, -19);
        if (mids.m_minYValue != -53 || mids.m_minXValue != 37
            || mids.m_maxYValue != 71 || mids.m_maxXValue != -19) return false;'''
        assert before in test;test=test.replace(before,after)
    subdivide=helper.definition(source,'subdivideRmgNoiseRegion')
    caller=helper.definition(source,'generateRmgIslandMask')
    program+=['namespace Case'+str(index)+' {\n',types,'\n',constructor,'\n',subdivide,'\n',caller,'\n',test,'}\n']
program.append(r'''
typedef void (*MaskGenerator)(unsigned char*, int, int);
bool masks(MaskGenerator candidate) {
    int sizes[]={0,1,2,3,5,8,13};
    for(int wi=0;wi<7;++wi) for(int hi=0;hi<7;++hi) for(unsigned seed=0;seed<16;++seed) {
        int width=sizes[wi],height=sizes[hi],count=width*height;
        std::vector<unsigned char> expected(count+32,0xcd),observed(count+32,0xcd);
        randomState=(seed+1)*1103515245U;draws.clear();
        Case0::generateRmgIslandMask(&expected[16],width,height);
        std::vector<int> expectedDraws=draws;
        randomState=(seed+1)*1103515245U;draws.clear();
        candidate(&observed[16],width,height);
        if(observed!=expected || draws!=expectedDraws) return false;
        for(int i=0;i<16;++i) if(observed[i]!=0xcd || observed[count+16+i]!=0xcd) return false;
    }
    return true;
}
int main() {
''')
for index in range(len(cases)):
    expression='Case%d::check(Case%d::subdivideRmgNoiseRegion) && masks(Case%d::generateRmgIslandMask)'%(index,index,index)
    condition=('!('+expression+')') if index<positive_count else '('+expression+')'
    program.append('    if ('+condition+') { std::printf("failed model '+str(index)+'\\n"); return 1; }\n')
program.append('    std::puts("'+str(positive_count)+' noise models: 3136 sample lattices and 784 complete masks/RNG traces each; four negative controls rejected");\n}\n')
with tempfile.TemporaryDirectory(prefix='rmg-noise-construction-') as tmp:
    path=Path(tmp);(path/'oracle.cpp').write_text(''.join(program))
    subprocess.run(['g++','-std=c++98','-O2',str(path/'oracle.cpp'),'-o',str(path/'oracle')],check=True)
    subprocess.run([str(path/'oracle')],check=True)
