#!/usr/bin/env python3
"""Rule construction and parsed-value lifetimes at retail 0x536560.

The fresh comparison has 62 matching CFG blocks except the append block,
where push_back expands to count-insert instead of retaining single-insert.
Earlier families varied the three auxiliary vectors, not the parsed rule's
construction. Test ordinary default/value initialization and real
parsed scalar lifetimes while preserving parsing order and library calls.
No Dreamcast RMG counterpart exists.
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


def variants(source):
    original = generator("generate-rmg-position-family.py").definition(source, NAME)
    initial = "        TRmgObjectPlacementRule rule;\n        rule.m_index = row - 3;"
    initialization = (
        initial,
        "        TRmgObjectPlacementRule rule = TRmgObjectPlacementRule();\n        rule.m_index = row - 3;",
    )
    parsed = """        objectType = atoi(values[3]);
        subtype = atoi(values[4]);
        terrain = atoi(values[6]);
        objectTypes.push_back(objectType);
        terrains.push_back(terrain);
        subtypes.push_back(subtype);"""
    local = """        int parsedObjectType = atoi(values[3]);
        int parsedSubtype = atoi(values[4]);
        int parsedTerrain = atoi(values[6]);
        objectTypes.push_back(parsedObjectType);
        terrains.push_back(parsedTerrain);
        subtypes.push_back(parsedSubtype);"""
    parse_forms = (parsed, local, "        {\n" + "\n".join("    " + line for line in local.splitlines()) + "\n        }")
    append = "        m_placementRules.push_back(rule);"
    append_forms = (
        append,
        "        m_placementRules.insert(m_placementRules.end(), rule);",
        "        const TRmgObjectPlacementRule& parsedRule = rule;\n        m_placementRules.push_back(parsedRule);",
        "        std::vector<TRmgObjectPlacementRule>& rules = m_placementRules;\n        rules.push_back(rule);",
        "        std::vector<TRmgObjectPlacementRule>::iterator end = m_placementRules.end();\n        m_placementRules.insert(end, rule);",
    )
    for anchor in (initial, parsed, append):
        if original.count(anchor) != 1:
            raise ValueError("review rule-reader anchor: " + anchor)
    # VC6 rejects aggregate initialization of this vector-bearing struct
    # (C2552), although the C++98 host oracle accepts it. The initial
    # 60-state opposite-corner control failed; those forms are excluded.
    for init, lifetime, insertion in itertools.product(range(2), range(3), range(5)):
        body = original.replace(initial, initialization[init]).replace(parsed, parse_forms[lifetime]).replace(append, append_forms[insertion])
        yield dict(name=f"rule_{init}+values_{lifetime}+append_{insertion}", replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    options = list(variants((HOMM3_DIR / SOURCE).read_text()))
    assert len({row["replace"] for row in options}) == 30
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="rule_construction_lifetime", source=SOURCE,
        find=options[0]["replace"], options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 30 rule-construction/lifetime states")


if __name__ == "__main__":
    main()
