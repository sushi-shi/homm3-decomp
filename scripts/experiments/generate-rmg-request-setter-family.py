#!/usr/bin/env python3
"""Provisional ordinary generator setters for request worker 0x54bf60.

Retail's persistent byte-capable one spans strength repair and seat setup.
Direct expressions, flag locals and byte/bool fields have not recovered it.
Test encapsulated human/town/seat setup through ordinary methods, whose
retail expansions would have to remain inline. Names/boundaries are hypotheses,
not Dreamcast recoveries. Do not mark them inline or retain diagnostic pins.
Score every consumer of their one owning rmg.h declaration.
"""
import argparse
import copy
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

HEADER = "    void removeObject(type_object* object);"
BEFORE = "// Retail 0x54bf60 constructs the 0x14e0-byte generator on its stack."
AFTER = "// Complete-only request wrapper, called by the lobby at 0x586422. Retail"


def setter(parent, form):
    body=parent
    target=re.compile(r"(?m)^(            )(generator\.m_fixedHumanPlayers\[seat\]|human) = (1|humanFlag);$")
    if len(target.findall(body))!=1:
        raise ValueError("review request seat store")
    if form in (0,1):
        declaration="    void setHumanPlayer(int seat);"
        helper="void type_random_map_generator::setHumanPlayer(int seat)\n{\n    m_fixedHumanPlayers[seat] = 1;\n}\n"
        body=target.sub("            generator.setHumanPlayer(seat);",body)
    elif form==2:
        declaration="    void setHumanPlayer(int seat, unsigned char enabled);"
        helper="void type_random_map_generator::setHumanPlayer(int seat, unsigned char enabled)\n{\n    m_fixedHumanPlayers[seat] = enabled;\n}\n"
        body=target.sub(lambda m:"            generator.setHumanPlayer(seat, "+m[3]+");",body)
    elif form==3:
        declaration="    void setTownChoice(int seat, int town);"
        helper="void type_random_map_generator::setTownChoice(int seat, int town)\n{\n    m_townChoices[seat] = town;\n}\n"
        marker="        generator.m_townChoices[seat] = m_townType[seat];"
        if body.count(marker)!=1:raise ValueError("review request town store")
        body=body.replace(marker,"        generator.setTownChoice(seat, m_townType[seat]);")
    elif form==4:
        declaration="    void setPlayerOptions(int seat, unsigned char human, int town);"
        helper="void type_random_map_generator::setPlayerOptions(int seat, unsigned char human, int town)\n{\n    if (human)\n        m_fixedHumanPlayers[seat] = 1;\n    m_townChoices[seat] = town;\n}\n"
        start=body.index("    for (int seat = 0; seat < 8; ++seat) {")
        end=body.index("\n    }",start)+6
        body=body[:start]+"    for (int seat = 0; seat < 8; ++seat)\n        generator.setPlayerOptions(seat, m_isHumanSeat[seat], m_townType[seat]);"+body[end:]
    else:
        raise ValueError("unknown request setter form")
    if form in (0,1,2):
        body=body.replace("        unsigned char& human = generator.m_fixedHumanPlayers[seat];\n","")
    if form in (0,1,4):
        body=re.sub(r"(?m)^    (unsigned char|int|bool) humanFlag = (1|true);\n","",body)
    return body,declaration,helper


def build(source, checkpoint):
    worker=generator("generate-rmg-request-worker-family.py")
    retained=generator("generate-rmg-key-tent-family.py").parents(source,checkpoint,worker.definition)
    original=worker.definition(source)
    options=[dict(name="unchanged",replace=original)]
    seen={(original,"","")}
    for identity,parent in retained:
        if (parent,"","") not in seen:
            seen.add((parent,"",""));options.append(dict(name=identity+"+parent",replace=parent))
    for form in range(5):
        for identity,parent in retained:
            body,declaration,helper=setter(parent,form)
            anchor=AFTER if form==1 else BEFORE
            key=(body,helper,anchor)
            if key not in seen:
                seen.add(key)
                options.append(dict(name=identity+"+setter_%d"%form,replace=body,extra_edits=[
                    dict(source="include/rmg.h",insert_before=HEADER,text=declaration+"\n"),
                    dict(source="src/rmg.cpp",insert_before=anchor,text="// Provisional request seat setter: retail 0x54bf60 retains no call.\n"+helper+"\n"),
                ]))
            if len(options)==60:
                return dict(schema=1,units=generator("generate-rmg-map-accessor-family.py").UNITS,
                    evidence=__doc__,axes=[dict(name="request_setter_boundary",source="src/rmg.cpp",find=original,options=options)])
    return dict(schema=1,units=generator("generate-rmg-map-accessor-family.py").UNITS,
        evidence=__doc__,axes=[dict(name="request_setter_boundary",source="src/rmg.cpp",find=original,options=options)])


def paired_parents(source, checkpoint):
    worker=generator("generate-rmg-request-worker-family.py")
    verified=dict(generator("generate-rmg-key-tent-family.py").parents(source,checkpoint,worker.definition))
    recorded=json.loads(checkpoint.read_text())
    manifest=json.loads((checkpoint.parent/"input.json").read_text())
    if len(manifest["axes"])!=1:
        raise ValueError("review setter parent axes")
    result=[]
    for elite in recorded["elites"]:
        if len(elite["choices"])!=1:
            raise ValueError("review setter parent choices")
        option=copy.deepcopy(manifest["axes"][0]["options"][elite["choices"][0]])
        if option["replace"]!=verified[elite["id"]]:
            raise ValueError("setter caller differs from reproduced parent")
        edits=option.get("extra_edits",[])
        if len(edits)!=2 or edits[0].get("source")!="include/rmg.h" or edits[0].get("insert_before")!=HEADER or edits[1].get("source")!="src/rmg.cpp" or edits[1].get("insert_before") not in (BEFORE,AFTER):
            raise ValueError("review paired setter declaration/body ownership")
        if "setHumanPlayer(int seat" not in edits[0]["text"] or "::setHumanPlayer(int seat" not in edits[1]["text"]:
            raise ValueError("refinements require the reproduced human setter")
        option["name"]=elite["id"]+"+parent"
        result.append(option)
    return result


def refinement(option, form):
    child=copy.deepcopy(option)
    child["name"] += "+refinement_%d"%form
    header,implementation=child["extra_edits"]
    if form==0:
        marker="        generator.m_townChoices[seat] = m_townType[seat];"
        if child["replace"].count(marker)!=1:raise ValueError("review town copy")
        child["replace"]=child["replace"].replace(marker,"        generator.setTownChoice(seat, m_townType[seat]);")
        header["text"] += "    void setTownChoice(int seat, int town);\n"
        implementation["text"] += "void type_random_map_generator::setTownChoice(int seat, int town)\n{\n    m_townChoices[seat] = town;\n}\n\n"
    elif form in (1,2):
        kind="unsigned int" if form==1 else "long"
        header["text"]=header["text"].replace("setHumanPlayer(int seat","setHumanPlayer("+kind+" seat")
        implementation["text"]=implementation["text"].replace("setHumanPlayer(int seat","setHumanPlayer("+kind+" seat")
    elif form==3:
        marker="        generator.m_townChoices[seat] = m_townType[seat];"
        if child["replace"].count(marker)!=1:raise ValueError("review town copy")
        child["replace"]=child["replace"].replace(marker,"        int town = m_townType[seat];\n        generator.m_townChoices[seat] = town;")
    elif form==4:
        marker="    for (int seat = 0; seat < 8; ++seat) {"
        if child["replace"].count(marker)!=1:raise ValueError("review seat loop")
        child["replace"]=child["replace"].replace(marker,"    int* towns = generator.m_townChoices;\n"+marker).replace("generator.m_townChoices[seat] = m_townType[seat];","towns[seat] = m_townType[seat];")
    else:
        raise ValueError("unknown setter refinement")
    return child


def frontier(source, checkpoint):
    worker=generator("generate-rmg-request-worker-family.py")
    current=worker.definition(source)
    retained=paired_parents(source,checkpoint)
    options=[dict(name="unchanged",replace=current)] + retained
    seen={(o["replace"],json.dumps(o.get("extra_edits",[]),sort_keys=True)) for o in options}
    for form in range(5):
        for parent in retained:
            child=refinement(parent,form)
            identity=(child["replace"],json.dumps(child["extra_edits"],sort_keys=True))
            if identity not in seen:
                seen.add(identity);options.append(child)
            if len(options)==60:
                return dict(schema=1,units=generator("generate-rmg-map-accessor-family.py").UNITS,
                    evidence=__doc__,axes=[dict(name="request_setter_refinement",source="src/rmg.cpp",find=current,options=options)])
    return dict(schema=1,units=generator("generate-rmg-map-accessor-family.py").UNITS,
        evidence=__doc__,axes=[dict(name="request_setter_refinement",source="src/rmg.cpp",find=current,options=options)])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint",type=Path)
    parser.add_argument("output",type=Path)
    parser.add_argument("--refine",action="store_true")
    args=parser.parse_args()
    source=(HOMM3_DIR/"src/rmg.cpp").read_text()
    payload=frontier(source,args.checkpoint) if args.refine else build(source,args.checkpoint)
    args.output.write_text(json.dumps(payload,indent=2)+"\n")
    load_manifest(args.output,HOMM3_DIR)
    print(len(payload["axes"][0]["options"]),"request setter states; seven consumers")


if __name__=="__main__":
    main()
