#!/usr/bin/env python3
"""Test request/generator seat-flag declarations as one atomic source family.

The request constructor and lobby write only 0/1, and the worker only tests
the input; the generator constructor clears its array, the worker writes 1,
and all remaining consumers test it as a predicate. Retail proves byte
storage, not signedness or bool. Test those natural boolean declarations
against the persistent-one register in request worker 0x54bf60, scoring all
consumers of rmg.h and rmg_request.h, including kb and advmgr.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

INPUT = "    unsigned char m_isHumanSeat[8];"
OUTPUT = "    unsigned char m_fixedHumanPlayers[8];"


def build(source, checkpoint):
    worker = generator("generate-rmg-request-worker-family.py")
    retained = generator("generate-rmg-key-tent-family.py").parents(source, checkpoint, worker.definition)
    original = worker.definition(source)
    options = [dict(name="unchanged", replace=original)]
    seen = {(original,"unsigned char","unsigned char")}
    for identity,parent in retained:
        key=(parent,"unsigned char","unsigned char")
        if key not in seen:
            seen.add(key);options.append(dict(name=identity+"+parent",replace=parent))
    for source_type,destination_type in (("unsigned char","signed char"),("unsigned char","bool"),
                                        ("char","unsigned char"),("bool","unsigned char"),("bool","bool")):
        for identity,parent in retained:
            body=parent
            if destination_type!="unsigned char":
                body=body.replace("unsigned char& human = generator.m_fixedHumanPlayers[seat];",destination_type+"& human = generator.m_fixedHumanPlayers[seat];")
            key=(body,source_type,destination_type)
            if key not in seen:
                seen.add(key)
                edits=[]
                if source_type!="unsigned char":
                    edits.append(dict(source="include/rmg_request.h",find=INPUT,replace=INPUT.replace("unsigned char",source_type)))
                if destination_type!="unsigned char":
                    edits.append(dict(source="include/rmg.h",find=OUTPUT,replace=OUTPUT.replace("unsigned char",destination_type)))
                options.append(dict(name=identity+"+input_"+source_type.replace(" ","_")+"+output_"+destination_type.replace(" ","_"),replace=body,extra_edits=edits))
            if len(options)==60:
                return dict(schema=1,units=generator("generate-rmg-map-accessor-family.py").UNITS+["kb","advmgr"],
                    evidence=__doc__,axes=[dict(name="request_byte_types",source="src/rmg.cpp",find=original,options=options)])
    raise ValueError("review finite request flag family")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint",type=Path)
    parser.add_argument("output",type=Path)
    args=parser.parse_args()
    payload=build((HOMM3_DIR/"src/rmg.cpp").read_text(),args.checkpoint)
    args.output.write_text(json.dumps(payload,indent=2)+"\n")
    load_manifest(args.output,HOMM3_DIR)
    print("60 request flag declaration states; nine consumers")


if __name__=="__main__":
    main()
