#!/usr/bin/env python3
"""Actual connection/vector-length bodies against integer-square-root oracle.

Finite non-overflowing signed dimensions and offsets; actual coordinate class
and ordinary helper body imported for each rendered state. VC6 audits own
floating-point instruction, ABI and retained-call validation.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path)
    args=parser.parse_args()
    extract=generator('generate-rmg-position-family.py').definition
    header=(HOMM3_DIR/'include/rmg.h').read_text()
    start=header.index('struct TRmgVector {');vector=header[start:header.index('\n};',start)+3]
    states=[{p:(HOMM3_DIR/p).read_text() for p in ('src/rmg.cpp','src/rmg_support.cpp')}]
    if args.manifest:
        _,originals,axes=source_families.load_manifest(args.manifest,HOMM3_DIR)
        states += [source_families.render(originals,axes,(i,)) for i in range(len(axes[0].options))]
    forms=[]
    for state in states:
        owners=[s for s in state.values() if 'int TRmgVector::length() const\n{' in s]
        assert len(owners)==1
        forms.append((extract(state['src/rmg.cpp'],'TRmgZone::canConnect'),extract(owners[0],'TRmgVector::length')))
    forms=list(dict.fromkeys(forms));positive=len(forms)
    original,helper=forms[0]
    forms += [(original.replace('m_z != m_levelPosition.m_z','m_z == m_levelPosition.m_z'),helper),
              (original.replace('combinedSize > minimumSize','combinedSize >= minimumSize'),helper),
              (original.replace('otherSize < minimumSize','otherSize > minimumSize'),helper)]
    # Use a caller that invokes the actual helper to reject a broken norm.
    for caller,length in forms[:positive]:
        if 'delta.length()' in caller:
            forms.append((caller,length.replace('m_x * m_x + m_y * m_y','m_x * m_x')))
            assert forms[-1]!=(caller,length)
            break
    text='#include <cmath>\n#include <cstdio>\nusing std::sqrt;\nstruct TRmgTownSlot { int m_size; }; struct Position { int m_x,m_y,m_z; };\n'
    for i,(caller,length) in enumerate(forms):
        text += 'namespace Case%d {\n'%i + vector+'\n'+length+'\nstruct TRmgZone { Position m_levelPosition; TRmgTownSlot* m_slot;\n'
        text += caller.replace('TRmgZone::','')+'\n};\n}\n'
    oracle=Path(__file__).with_name('test_rmg_connect_owners.py').read_text()
    start=oracle.index('template<class T> bool check() {');end=oracle.index('\nint main() {',start)
    text += oracle[start:end]+'\nint main() {\n'
    for i in range(len(forms)):
        text += 'if (%scheck<Case%d::TRmgZone>()) { std::printf("failed %d\\n"); return 1; }\n'%('!' if i<positive else '',i,i)
    text += 'return 0;}\n'
    with tempfile.TemporaryDirectory(prefix='rmg-connect-length-oracle-') as folder:
        source,binary=Path(folder)/'oracle.cpp',Path(folder)/'oracle';source.write_text(text)
        subprocess.run(['g++','-std=c++98','-O2',str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    print('%d unique actual caller/helper pairs x122018 scenarios plus same-object checks; %d wrong controls rejected'%(positive,len(forms)-positive))


if __name__=='__main__':
    main()
