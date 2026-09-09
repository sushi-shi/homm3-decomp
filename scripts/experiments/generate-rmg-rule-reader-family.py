#!/usr/bin/env python3
"""Placement-rule reader's retained single-insert boundary (retail 0x536560).

The current 99.7862% body has all branch destinations and stack homes correct;
only the append expands one level further and adds a count-one argument.
No Dreamcast counterpart was found. Cross real container/end bindings, terrain
array bindings and equivalent range fills without altering canonical helpers,
rule construction/destruction, parsing order or the later reverse rule lookup.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "TRmgGeneratorBase::readObjectPlacementRules"


def variants(source, direct=False):
    original = generator("generate-rmg-position-family.py").definition(source, NAME)
    begin = "        for (terrain = 0; terrain <= eTerrainWater; ++terrain)"
    fill = "        for (; terrain < 10; ++terrain)\n            rule.m_terrainScores[terrain] = RMG_PLACEMENT_INVALID;"
    append = "        m_placementRules.push_back(rule);"
    if any(original.count(anchor) != 1 for anchor in (begin, fill, append)):
        raise ValueError("review changed rule-reader source")
    appends = [append,
        "        std::vector<TRmgObjectPlacementRule>& rules = m_placementRules;\n        rules.push_back(rule);",
        "        std::vector<TRmgObjectPlacementRule>::iterator end = m_placementRules.end();\n        m_placementRules.insert(end, rule);",
        "        std::vector<TRmgObjectPlacementRule>& rules = m_placementRules;\n        std::vector<TRmgObjectPlacementRule>::iterator end = rules.end();\n        rules.insert(end, rule);",
        "        const TRmgObjectPlacementRule& parsedRule = rule;\n        m_placementRules.push_back(parsedRule);"]
    if direct:
        yield dict(name="unchanged", replace=original)
        appends = ["        m_placementRules.insert(m_placementRules.end(), rule);"]
    fills = [fill,
        "        std::fill(rule.m_terrainScores + terrain, rule.m_terrainScores + 10, RMG_PLACEMENT_INVALID);\n        terrain = 10;",
        "        std::fill_n(rule.m_terrainScores + terrain, 10 - terrain, RMG_PLACEMENT_INVALID);\n        terrain = 10;"]
    # The shared terrain scalar is assigned by the original tail loop. Keep
    # that final value explicitly in the range-algorithm alternatives.
    for receiver, array, tail in itertools.product(range(len(appends)), range(4), range(3)):
        body = original.replace(fill, fills[tail]).replace(append, appends[receiver])
        if array in (1, 2):
            declaration = "int* scores" if array == 1 else "int (&scores)[10]"
            body = body.replace(begin, "        " + declaration + " = rule.m_terrainScores;\n" + begin)
            start = body.index(begin)
            end = body.index(appends[receiver], start)
            body = body[:start] + body[start:end].replace("rule.m_terrainScores", "scores") + body[end:]
        elif array == 3:
            body = body.replace(begin, "        TRmgObjectPlacementRule& parsed = rule;\n" + begin)
            start = body.index(begin)
            end = body.index(appends[receiver], start)
            body = body[:start] + body[start:end].replace("rule.m_terrainScores", "parsed.m_terrainScores") + body[end:]
        yield dict(name="append_%d+array_%d+fill_%d" % (receiver, array, tail), replace=body)


def constructor_frontier(source):
    original = next(variants(source))["replace"]
    for object_type, terrain, subtype, direct in itertools.product(range(3), range(3), range(3), range(2)):
        body = original
        for name, form in zip(("objectTypes", "terrains", "subtypes"), (object_type, terrain, subtype)):
            before = "    std::vector<int> " + name + ";"
            if body.count(before) != 1:
                raise ValueError("review changed scalar-vector declaration")
            body = body.replace(before, "    std::vector<int> " + name + (";", "(0);", "(0, 0);")[form])
        if direct:
            body = body.replace("m_placementRules.push_back(rule);", "m_placementRules.insert(m_placementRules.end(), rule);")
        yield dict(name="construct_%d%d%d+direct_%d" % (object_type, terrain, subtype, direct), replace=body)


def empty_initialization_frontier(source):
    original = next(variants(source))["replace"]
    # VC6 rejected the parenthesized direct-temporary opposite corner.
    # Keep only default/copy initialization in this finite family.
    for object_type, terrain, subtype, shared in itertools.product(range(2), repeat=4):
        body = original
        if shared:
            body = body.replace("    std::vector<int> objectTypes;", "    std::allocator<int> vectorAllocator;\n    std::vector<int> objectTypes;")
        for name, form in zip(("objectTypes", "terrains", "subtypes"), (object_type, terrain, subtype)):
            argument = "vectorAllocator" if shared else ""
            construction = "std::vector<int>(" + argument + ")"
            suffix = (("(" + argument + ");" if shared else ";"),
                      " = " + construction + ";")[form]
            body = body.replace("    std::vector<int> " + name + ";", "    std::vector<int> " + name + suffix)
        yield dict(name="empty_%d%d%d+shared_allocator_%d" % (object_type, terrain, subtype, shared), replace=body)


def row_frontier(source):
    original = next(variants(source))["replace"]
    declaration = "        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);"
    guard = "        if (values[0][0] == ' ' || values[0][0] == 0)"
    for marker, receiver, lifetime in itertools.product(range(5), range(4), range(3)):
        boundary = original.index("    int ruleCount =")
        phase, rest = original[:boundary], original[boundary:]
        if marker:
            if marker == 1:
                capture, expression = "const char* marker = values[0];", "marker[0]"
            else:
                typename = {2: "char", 3: "unsigned char", 4: "const char&"}[marker]
                capture, expression = typename + " marker = values[0][0];", "marker"
            phase = phase.replace(guard, "        " + capture + "\n" + guard.replace("values[0][0]", expression))
        if receiver == 1:
            phase = phase.replace(declaration, "        const TSpreadsheetResource::TStringVector* values = &sheet->getRow(row);")
            phase = phase.replace("values[", "(*values)[")
        elif receiver == 2:
            phase = phase.replace(declaration, "        TSpreadsheetResource::TStringVector::const_iterator values = sheet->getRow(row).begin();")
        elif receiver == 3:
            phase = phase.replace(declaration + "\n", "").replace("values[", "sheet->getRow(row)[")
        if lifetime == 1:
            phase = phase.replace("    int row = 3;\n", "")
            phase = phase.replace("    int terrain;", "    int terrain;\n    int row = 3;")
        elif lifetime == 2:
            phase = phase.replace("    int row = 3;", "    int row;")
            phase = phase.replace("    for (; row < sheet->getNumberOfRows();)", "    for (row = 3; row < sheet->getNumberOfRows();)")
        yield dict(name="marker_%d+row_receiver_%d+lifetime_%d" % (marker, receiver, lifetime), replace=phase + rest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--direct", action="store_true", help="twelve direct single-insert controls without a named end iterator")
    parser.add_argument("--constructors", action="store_true", help="empty scalar-vector constructor forms and the rule append API")
    parser.add_argument("--empty-initializers", action="store_true", help="empty temporary initialization and shared allocator lifetimes")
    parser.add_argument("--rows", action="store_true", help="leading-character capture and spreadsheet-row receiver lifetimes")
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    if sum((args.direct, args.constructors, args.empty_initializers, args.rows)) > 1:
        parser.error("choose one frontier")
    options = list(row_frontier(source) if args.rows else empty_initialization_frontier(source) if args.empty_initializers else
                   constructor_frontier(source) if args.constructors else variants(source, args.direct))
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="rule_reader_bindings", source=SOURCE, find=options[0]["replace"], options=options)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "rule-reader states")


if __name__ == "__main__":
    main()
