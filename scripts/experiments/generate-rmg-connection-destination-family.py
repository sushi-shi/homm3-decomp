#!/usr/bin/env python3
"""Recover an expanded destination accessor in the shared connection counter.

Both retail count expansions load connection->destination->zoneIndex through
EAX. Sixty pointer/reference ownership controls leave the raw field chain
unchanged. Test canonical ordinary getters at the slot, connection, or composed
boundary; each exposes the actual field read and is called by the one shared
counter, hence both expansions. No explicit inline, clone or dummy call.
Names and boundaries are source hypotheses; no retail claims are invented.
Cross live vector receiver scope because it interacts with returned values.
"""
import argparse
import itertools
import json
import subprocess
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator

SLOT='int TRmgTownSlot::getZoneIndex() const\n{\n    return m_zoneIndex;\n}\n'
DEST='TRmgTownSlot* TRmgZoneConnection::getDestination() const\n{\n    return m_destination;\n}\n'
INDEX='int TRmgZoneConnection::getDestinationZoneIndex() const\n{\n    return m_destination->m_zoneIndex;\n}\n'
MARKER='// Both connection-count passes in FilterZonePositions retain the same\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();header=(HOMM3_DIR/'include/rmg.h').read_text()
    original=generator('generate-rmg-position-family.py').definition(source,'type_random_map_generator::countPlacedZoneConnections')
    parents=list(generator('generate-rmg-connection-count-owner-family.py').models(original))
    options=[]
    for accessor,receiver in itertools.product(range(5),(0,1,3)):
        body=parents[receiver]['replace'];edits=[];definitions='';slotdecl='';edgedecl=''
        expr='slot->m_connections[connection].m_destination->m_zoneIndex'
        replacement=expr
        if accessor in (1,3):slotdecl='    int getZoneIndex() const;\n';definitions+=SLOT+'\n'
        if accessor in (2,3):edgedecl='    TRmgTownSlot* getDestination() const;\n';definitions+=DEST+'\n'
        if accessor==1:replacement='slot->m_connections[connection].m_destination->getZoneIndex()'
        elif accessor==2:replacement='slot->m_connections[connection].getDestination()->m_zoneIndex'
        elif accessor==3:replacement='slot->m_connections[connection].getDestination()->getZoneIndex()'
        elif accessor==4:
            edgedecl='    int getDestinationZoneIndex() const;\n';definitions=INDEX+'\n';replacement='slot->m_connections[connection].getDestinationZoneIndex()'
        assert body.count(expr)==1;body=body.replace(expr,replacement)
        for owner,decl in (('TRmgTownSlot',slotdecl),('TRmgZoneConnection',edgedecl)):
            if decl:
                anchor='struct '+owner+' {\n';assert header.count(anchor)==1
                edits.append(dict(source='include/rmg.h',find=anchor,replace=anchor+decl))
        if definitions:edits.append(dict(source='src/rmg.cpp',find=MARKER,replace=definitions+MARKER))
        options.append(dict(name=f'accessor_{accessor}+receiver_{receiver}',replace=body,extra_edits=edits))
    assert options[0]['replace']==original and not options[0]['extra_edits']
    deps=subprocess.check_output(['ninja','-t','deps'],cwd=HOMM3_DIR,text=True);units=[]
    for block in deps.split('\n\n'):
        lines=block.splitlines()
        if lines and str(HOMM3_DIR/'include/rmg.h') in [line.strip() for line in lines[1:]]:units.append(Path(lines[0].split(':',1)[0]).stem)
    units=sorted(set(units));assert {'rmg','rmg_support','rmg_terrain'}<=set(units)
    payload=dict(schema=1,units=units,evidence=__doc__,axes=[dict(name='connection_destination',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n');load_manifest(args.output,HOMM3_DIR)
    print('15 ordinary destination-accessor/vector-owner models;',units)


if __name__=='__main__':main()
