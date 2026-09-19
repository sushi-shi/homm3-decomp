#!/usr/bin/env python3
"""Shared connection-count ownership in both filterZonePositions expansions.

Retail0x53b2f0 loads the connection's destination through EAX before forming
the zone-vector receiver from spilled this. The canonical ordinary helper
currently allocates that chain and receiver differently in both expansions.
Compare slot pointer/reference ownership, the actual intermediate destination
or connection record, and live vector receiver scope. No helper clones,
false inline, cached vector size, or altered eligibility/call ordering.
RMG has no Dreamcast compiland. Current scalar input control is unchanged.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from experiments._support import generator


def models(original):
    for slot,owner,receiver in itertools.product(range(3),range(5),range(4)):
        body=original
        if slot:
            kind='TRmgTownSlot&' if slot==1 else 'const TRmgTownSlot&'
            body=body.replace('TRmgTownSlot* slot = zone->m_slot;',f'{kind} slot = *zone->m_slot;').replace('slot->','slot.')
        spelling='slot->m_connections[connection]' if slot==0 else 'slot.m_connections[connection]'
        original_read=f'int destination = {spelling}.m_destination->m_zoneIndex;'
        lines=[original_read]
        if owner in (1,2):
            kind='TRmgTownSlot*' if owner==1 else 'const TRmgTownSlot&'
            expr=spelling+'.m_destination' if owner==1 else '*'+spelling+'.m_destination'
            access='->' if owner==1 else '.'
            lines=[f'{kind} destinationSlot = {expr};',f'int destination = destinationSlot{access}m_zoneIndex;']
        elif owner in (3,4):
            kind='const TRmgZoneConnection*' if owner==3 else 'const TRmgZoneConnection&'
            expr='&'+spelling if owner==3 else spelling;access='->' if owner==3 else '.'
            lines=[f'{kind} edge = {expr};',f'int destination = edge{access}m_destination->m_zoneIndex;']
        body=body.replace(original_read,'\n        '.join(lines))
        if receiver:
            typename='std::vector<TRmgZone*>'
            decl=f'const {typename}'+('* zones = &m_zones;' if receiver==2 else '& zones = m_zones;')
            body=body.replace('destination < m_zones.size() && m_zones[destination]->canConnect(zone)',
                              'destination < zones->size() && (*zones)[destination]->canConnect(zone)' if receiver==2 else
                              'destination < zones.size() && zones[destination]->canConnect(zone)')
            if receiver==3:
                marker='        if (destination <';body=body.replace(marker,'        '+decl+'\n'+marker)
            else:
                marker='    for (int connection';body=body.replace(marker,'    '+decl+'\n'+marker)
        yield dict(name=f'slot_{slot}+destination_{owner}+receiver_{receiver}',replace=body)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args()
    source=(HOMM3_DIR/'src/rmg.cpp').read_text();original=generator('generate-rmg-position-family.py').definition(source,'type_random_map_generator::countPlacedZoneConnections')
    options=list(models(original));assert len(options)==60 and options[0]['replace']==original
    payload=dict(schema=1,units=['rmg'],evidence=__doc__,axes=[dict(name='connection_count_owner',source='src/rmg.cpp',find=original,options=options)])
    args.output.write_text(json.dumps(payload,indent=2)+'\n');load_manifest(args.output,HOMM3_DIR)
    print('60 shared helper ownership/scope models')


if __name__=='__main__':main()
