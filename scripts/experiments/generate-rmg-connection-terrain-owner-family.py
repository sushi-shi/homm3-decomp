#!/usr/bin/env python3
"""Recover ownership of the connection seed's terrain read at retail0x540701.

The canonical six-bit field is signed (retained adapter/river extraction proof),
but this connection pass uses only equality with Water/Rock and reads low six
bits in retail. The existing unsigned-byte getLandType accessor is used in
road/river consumers; test its actual call boundary together with natural
unsigned/int/byte/terrain-enum local ownership. No field/interface changes or
invented masking accessor. The prior explicit-mask-only family is a control
for a different hypothesis, not proof the field declaration must be changed.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text()
    original=generator('generate-rmg-position-family.py').definition(source,'type_random_map_generator::buildZoneConnectionPaths')
    old='unsigned terrain = current->m_tile.m_landType;';assert original.count(old)==1
    options=[]
    for accessor,kind in itertools.product(range(2),('unsigned','int','unsigned char','TTerrainType')):
        expression='current->getLandType()' if accessor else 'current->m_tile.m_landType'
        if kind=='TTerrainType':expression='static_cast<TTerrainType>('+expression+')'
        options.append(dict(name=f'accessor_{accessor}+type_{kind}',replace=original.replace(old,f'{kind} terrain = {expression};')))
    assert options[0]['replace']==original
    payload=dict(schema=1,units=['rmg'],evidence=__doc__,axes=[dict(name='connection_terrain_owner',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n');load_manifest(args.output,HOMM3_DIR)
    # All 64 bit patterns have identical predicate outcomes under these conversions.
    # The zone comparison is reached only for Water; invalid negative field values
    # cannot become Water/Rock by unsigned-byte conversion.
    for field in range(-32,32):
        values=(field%(2**32),field,field%256,field,field%256)
        for zone in range(-32,64):
            predicates=[((v!=8 or zone==v),v!=9) for v in values]
            assert len(set(predicates))==1,(field,zone,predicates)
    print('eight existing-accessor/local-type models; terrain predicates equal for all64 field patterns ×96 zone values')


if __name__=='__main__':main()
