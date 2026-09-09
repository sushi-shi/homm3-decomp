#!/usr/bin/env python3
"""Provisional shared color-reservation boundary across four retail expansions.

Retail placeBorderObject +0x1ff, placeKeyTentGuard +0x155/+0x2c3 and
removeObject +0xea all store a disabled byte then restart the same first-free
color scan. These copies support testing one ordinary member helper, not an
inline qualifier or forced call. Test setter+scan versus scan-only boundaries
and natural loop/receiver forms across reproduced guard-placement parents.
Every affected caller and all seven header consumers are compiled.
"""
import argparse
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

DECLARATION_ANCHOR = "    unsigned char placeKeyTentGuard(type_object* object, int maxValue);"
DEFINITION_ANCHOR = "// The shipyard caller at 0x541fc5 passes its entrance, count 3 and destination."


def helper(form):
    scan_only = form in (2, 3)
    name = "refreshKeyTentColor" if scan_only else "setKeyTentDisabled"
    parameters = "" if scan_only else "int color, unsigned char disabled"
    declaration = "    void " + name + "(" + parameters + ");"
    body = "void type_random_map_generator::" + name + "(" + parameters + ")\n{\n"
    if form == 4:
        body += "    std::vector<unsigned char>& colors = m_disabledKeyTents;\n"
    if not scan_only:
        body += "    m_disabledKeyTents[color] = disabled;\n"
    if form in (1, 3):
        body += "    for (m_nextKeyTentColor = 0; m_nextKeyTentColor < m_disabledKeyTents.size()\n        && m_disabledKeyTents[m_nextKeyTentColor]; ++m_nextKeyTentColor) {}\n"
    else:
        body += "    m_nextKeyTentColor = 0;\n    while (m_nextKeyTentColor < m_disabledKeyTents.size()\n        && m_disabledKeyTents[m_nextKeyTentColor])\n        ++m_nextKeyTentColor;\n"
    body += "}\n"
    if form == 4:
        body = body.replace("m_disabledKeyTents[", "colors[").replace("m_disabledKeyTents.size()", "colors.size()")
    return declaration, body


def replace_runs(body, form, expected_count):
    pattern = re.compile(r"(?m)^( +)((?:m_disabledKeyTents|disabled)\[([^\n]+)\] = ([01]);)\n +m_nextKeyTentColor = 0;\n +while \(m_nextKeyTentColor < (?:m_disabledKeyTents|disabled)\.size\(\)\n +&& (?:m_disabledKeyTents|disabled)\[m_nextKeyTentColor\]\)\n +\+\+m_nextKeyTentColor;")
    def replacement(match):
        indent, store, color, value = match.groups()
        if form in (2, 3):
            return indent + store + "\n" + indent + "refreshKeyTentColor();"
        return indent + "setKeyTentDisabled(" + color + ", " + value + ");"
    result, count = pattern.subn(replacement, body)
    if count != expected_count:
        raise ValueError("review color-run sites: expected %d, got %d" % (expected_count, count))
    unused = "    std::vector<unsigned char>& disabled = m_disabledKeyTents;\n"
    if unused in result and "disabled[" not in result:
        result = result.replace(unused, "")
    return result


def build(source, header, checkpoint):
    key = generator("generate-rmg-key-tent-family.py")
    definition = generator("generate-rmg-position-family.py").definition
    original = key.definition(source)
    border = definition(source, "type_random_map_generator::placeBorderObject")
    removal = definition(source, "type_random_map_generator::removeObject")
    retained = key.parents(source, checkpoint)
    options = [dict(name="unchanged", replace=original)]
    seen = {(original, "")}
    for identity, parent in retained:
        if (parent, "") not in seen:
            seen.add((parent, ""));options.append(dict(name=identity+"+parent", replace=parent))
    for form in range(5):
        declaration, body = helper(form)
        extra = [
            dict(source="include/rmg.h", insert_before=DECLARATION_ANCHOR, text=declaration+"\n"),
            dict(source="src/rmg.cpp", insert_before=DEFINITION_ANCHOR, text=
                 "// Provisional shared color reservation: retail 0x540d60+0x1ff,\n"
                 "// 0x54b8c0+0x155/+0x2c3 and 0x54bc50+0xea repeat this scan.\n"
                 "// No Dreamcast name/inline declaration or retained body is claimed.\n"+body+"\n"),
            dict(source="src/rmg.cpp", find=border, replace=replace_runs(border,form,1)),
            dict(source="src/rmg.cpp", find=removal, replace=replace_runs(removal,form,1)),
        ]
        for identity,parent in retained:
            child=replace_runs(parent,form,2)
            if (child,body) not in seen:
                seen.add((child,body))
                options.append(dict(name=identity+"+color_helper_%d"%form,replace=child,extra_edits=extra))
            if len(options)==60:
                return dict(schema=1,units=generator("generate-rmg-map-accessor-family.py").UNITS,evidence=__doc__,axes=[dict(name="key_color_boundary",source="src/rmg.cpp",find=original,options=options)])
    raise ValueError("review finite family size")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint",type=Path)
    parser.add_argument("output",type=Path)
    args=parser.parse_args()
    payload=build((HOMM3_DIR/"src/rmg.cpp").read_text(),(HOMM3_DIR/"include/rmg.h").read_text(),args.checkpoint)
    args.output.write_text(json.dumps(payload,indent=2)+"\n")
    load_manifest(args.output,HOMM3_DIR)
    print(len(payload["axes"][0]["options"]),"key-color boundary states; seven consumers")


if __name__=="__main__":
    main()
