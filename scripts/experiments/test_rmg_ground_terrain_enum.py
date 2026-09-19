#!/usr/bin/env python3
"""Check actual packed ground structs under the shared terrain enum domain.

This host check covers representation, signed NONE extraction and preservation
of adjacent fields. VC6 retail comparison remains the ABI/codegen verdict.
Values outside the declared terrain enum domain are deliberately not claimed.
"""
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR

header=(HOMM3_DIR/'include/rmg.h').read_text();start=header.index('struct TRmgGroundTile {');end=header.index('\n};',start)+3
signed=header[start:end];enum=signed.replace('TRmgGroundTile','EnumGroundTile').replace('signed m_landType : 6;','TTerrainType m_landType : 6;')
cpp='#include <cassert>\n#include <cstring>\n'+(HOMM3_DIR/'include/terrain_type.h').read_text()+'\n'+signed+'\n'+enum+r'''
int main() {
    assert(sizeof(TRmgGroundTile)==4 && sizeof(EnumGroundTile)==4);
    for(int land=-1;land<=9;++land) for(int pattern=0;pattern<4096;++pattern) {
        TRmgGroundTile a;EnumGroundTile b;
        unsigned bits=unsigned(pattern)*2654435761u;
        memcpy(&a,&bits,4);memcpy(&b,&bits,4);
        a.m_landType=land;b.m_landType=static_cast<TTerrainType>(land);
        assert(memcmp(&a,&b,4)==0);
        assert(int(a.m_landType)==land && int(b.m_landType)==land);
        assert(a.m_terrainFrame==b.m_terrainFrame);
        assert(a.m_riverType==b.m_riverType && a.m_riverFrame==b.m_riverFrame);
        assert(a.m_roadType==b.m_roadType && a.m_unknown30==b.m_unknown30);
    }
}
'''
with tempfile.TemporaryDirectory(prefix='rmg-ground-terrain-enum-') as tmp:
    p=Path(tmp)/'test';p.with_suffix('.cpp').write_text(cpp)
    subprocess.run(['g++','-std=c++98','-O1','-fsanitize=undefined',str(p.with_suffix('.cpp')),'-o',str(p)],check=True)
    r=subprocess.run([str(p)],capture_output=True);assert r.returncode==0 and not r.stderr,r.stderr.decode()
print('45,056 actual packed-field assignments; terrain domain/NONE/adjacent-field representation agree; UBSan clean')
