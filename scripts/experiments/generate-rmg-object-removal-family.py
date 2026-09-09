#!/usr/bin/env python3
"""Retail removeObject 0x54bc50: coordinate/index ownership.

Retail recomputes the outer y subtraction and the inner eight-times-y mask
offset; our unsigned scalar loop carries both as separate induction values.
The frame differs by four bytes. Test signed/unsigned scalar and real point
indices, input-position copy lifetimes and scalar/point map coordinates.
Keep canonical accessors, masks, public erase and the observed null tests.
No Dreamcast counterpart is mapped; all rmg siblings are scored.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

NAME="type_random_map_generator::removeObject"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source,NAME)


def scalar_seed(original):
    """Keep the reviewed scalar control reproducible after coordinate adoption."""
    if "    TRmgGridPoint cell;" not in original:
        return original
    original=original.replace("    TRmgMapPosition position;\n    position = object->m_position;", "    TRmgMapPosition position = object->m_position;")
    original=original.replace("    TRmgGridPoint cell;\n    TRmgMapPosition mapPosition;\n    mapPosition.m_z = position.m_z;\n", "")
    original=original.replace("cell.m_x", "x").replace("cell.m_y", "y")
    original=original.replace("for (y = 0;", "for (unsigned int y = 0;").replace("for (x = 0;", "for (unsigned int x = 0;")
    original=original.replace("mapPosition.m_y = position.m_y - y;", "int mapY = position.m_y - y;").replace("mapPosition.m_x = position.m_x - x;", "int mapX = position.m_x - x;")
    original=original.replace("mapPosition.m_y", "mapY").replace("mapPosition.m_x", "mapX")
    return original.replace("m_map.getMapItem(mapPosition)", "m_map.getMapItem(mapX, mapY, position.m_z)")


def variants(original):
    original=scalar_seed(original)
    position="    TRmgMapPosition position = object->m_position;"
    scan="    for (unsigned int y = 0; y < prototype->getHeight(); ++y) {"
    if original.count(position)!=1 or original.count(scan)!=1:
        raise ValueError("review changed removal coordinate anchors")
    positions=[position,
        "    TRmgMapPosition position;\n    position = object->m_position;",
        "    const TRmgMapPosition position = object->m_position;",
        "    const TRmgMapPosition& position = object->m_position;",
        "    TRmgMapPosition position;\n    position.m_x = object->m_position.m_x;\n    position.m_y = object->m_position.m_y;\n    position.m_z = object->m_position.m_z;",
    ]
    prefix,loop=original.split(scan)
    loop=scan+loop
    for ownership,index,coordinate in itertools.product(range(5),range(6),range(2)):
        leading=prefix.replace(position,positions[ownership])
        tail=loop
        if index<4:
            if index&1: tail=tail.replace("unsigned int y = 0", "int y = 0")
            if index&2: tail=tail.replace("unsigned int x = 0", "int x = 0")
        else:
            leading += "    " + ("TPoint" if index==4 else "TRmgGridPoint") + " cell;\n"
            tail=tail.replace("unsigned int y = 0", "y = 0").replace("unsigned int x = 0", "x = 0")
            tail=re.sub(r"\by\b","cell.m_y",tail)
            tail=re.sub(r"\bx\b","cell.m_x",tail)
        if coordinate:
            leading += "    TPoint mapPosition;\n"
            tail=tail.replace("int mapY =", "mapPosition.m_y =").replace("int mapX =", "mapPosition.m_x =")
            tail=re.sub(r"\bmapY\b","mapPosition.m_y",tail)
            tail=re.sub(r"\bmapX\b","mapPosition.m_x",tail)
        yield "position_%d+index_%d+coordinate_%d"%(ownership,index,coordinate),leading+tail


def level_refinement(parent,form):
    body=parent
    declaration="    TPoint mapPosition;"
    if declaration in body:
        body=body.replace(declaration,"    TRmgMapPosition mapPosition;")
    else:
        marker="    for ("
        offset=body.index(marker)
        body=body[:offset]+"    TRmgMapPosition mapPosition;\n"+body[offset:]
        body=body.replace("int mapY =","mapPosition.m_y =").replace("int mapX =","mapPosition.m_x =")
        body=re.sub(r"\bmapY\b","mapPosition.m_y",body)
        body=re.sub(r"\bmapX\b","mapPosition.m_x",body)
    lookup="m_map.getMapItem(mapPosition.m_x, mapPosition.m_y, position.m_z)"
    if body.count(lookup)!=1:
        raise ValueError("review removal map-coordinate lookup")
    if form in (0,1):
        body=body.replace("    TRmgMapPosition mapPosition;", "    TRmgMapPosition mapPosition;\n    mapPosition.m_z = position.m_z;")
    elif form==2:
        marker="            if (mapPosition.m_x < 0"
        if body.count(marker)!=1: raise ValueError("review removal x guard")
        body=body.replace(marker,"            mapPosition.m_z = position.m_z;\n"+marker)
    elif form in (3,4):
        body=body.replace("    TRmgMapPosition mapPosition;","    TRmgMapPosition mapPosition = position;")
    else:
        raise ValueError("unknown removal level family")
    replacement="m_map.getMapItem(mapPosition)" if form in (1,4) else lookup.replace("position.m_z","mapPosition.m_z")
    return body.replace(lookup,replacement)


def counter_refinement(parent,form):
    before="        int zone = m_map.getMapItem("
    decrement="            --m_zones[zone]->m_objectCountByType[prototype->m_objectType];"
    if parent.count(before)!=1 or parent.count(decrement)!=1:
        raise ValueError("review removal zone-counter anchors")
    body=parent
    if form==0:
        return body.replace(decrement,"            m_zones[zone]->m_objectCountByType[prototype->m_objectType]--;")
    if form in (1,2):
        body=body.replace(before,"        int objectType = prototype->m_objectType;\n"+before)
        replacement="            --m_zones[zone]->m_objectCountByType[objectType];" if form==1 else "            m_zones[zone]->m_objectCountByType[objectType]--;"
        return body.replace(decrement,replacement)
    if form in (3,4):
        replacement="            m_zones[zone]->m_objectCountByType[prototype->m_objectType] -= 1;" if form==3 else "            m_zones[zone]->m_objectCountByType[prototype->m_objectType] = m_zones[zone]->m_objectCountByType[prototype->m_objectType] - 1;"
        return body.replace(decrement,replacement)
    raise ValueError("unknown removal counter family")


def zone_refinement(parent,form):
    query="        int zone = m_map.getMapItem(position.m_x - prototype->m_triggerCell.m_x,\n            position.m_y - prototype->m_triggerCell.m_y, position.m_z)->m_zoneState.m_zone;"
    tail="        if (zone >= 0)\n            --m_zones[zone]->m_objectCountByType[prototype->m_objectType];"
    if parent.count(query)!=1 or parent.count(tail)!=1:
        raise ValueError("review removal zone lookup")
    if form in (0,1):
        return parent.replace(query,query.replace("int zone",("signed char" if form==0 else "short")+" zone"))
    if form==2:
        return parent.replace(tail,"        int objectType = prototype->m_objectType;\n"+tail.replace("[prototype->m_objectType]","[objectType]"))
    if form==3:
        return parent.replace(query,query.replace("int zone =", "TRmgMapItem* entranceItem =").replace("->m_zoneState.m_zone;",";\n        int zone = entranceItem->m_zoneState.m_zone;"))
    if form==4:
        return parent.replace(tail,"        if (zone >= 0) {\n            TRmgZone* owner = m_zones[zone];\n            --owner->m_objectCountByType[prototype->m_objectType];\n        }")
    raise ValueError("unknown removal zone family")


def frontier(source,checkpoint,refine=level_refinement):
    retained=generator("generate-rmg-key-tent-family.py").parents(source,checkpoint,definition)
    current=definition(source);forms=[("unchanged",current)];seen={current}
    for identity,parent in retained:
        if parent not in seen:
            seen.add(parent);forms.append((identity+"+parent",parent))
    for form in range(5):
        for identity,parent in retained:
            body=refine(parent,form)
            if body not in seen:
                seen.add(body);forms.append((identity+"+"+refine.__name__+"_%d"%form,body))
            if len(forms)==60:return forms
    return forms


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output",type=Path)
    parser.add_argument("--levels-from",type=Path)
    parser.add_argument("--counters-from",type=Path)
    parser.add_argument("--zones-from",type=Path)
    args=parser.parse_args()
    source=(HOMM3_DIR/"src/rmg.cpp").read_text()
    original=definition(source)
    if sum(bool(x) for x in (args.levels_from,args.counters_from,args.zones_from))>1:
        parser.error("choose one refinement")
    forms=frontier(source,args.zones_from,zone_refinement) if args.zones_from else frontier(source,args.counters_from,counter_refinement) if args.counters_from else frontier(source,args.levels_from) if args.levels_from else variants(original)
    forms=list(forms)
    if all(body!=original for _,body in forms):
        forms.insert(0,("unchanged",original))
    payload=dict(schema=1,units=["rmg"],evidence=__doc__,axes=[generator("generate-rmg-position-family.py").axis("object_removal_coordinates","src/rmg.cpp",original,forms)])
    args.output.write_text(json.dumps(payload,indent=2)+"\n")
    load_manifest(args.output,HOMM3_DIR)
    print(len(payload["axes"][0]["options"]),"object-removal coordinate states")


if __name__=="__main__":
    main()
