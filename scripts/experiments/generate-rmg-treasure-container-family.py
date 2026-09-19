#!/usr/bin/env python3
"""Treasure mask proxy and parallel-container ownership at 0x546190.

Verified C2 trace: passable test expands at depth two with budget81/cost58;
both clear->erase expansions have budgets91/89 and cost69. Retail retains
those three boundaries. Test existing public STL proxy/reset interfaces and
actual joint ownership of the two vectors, preserving the two independent
sequences and their first/second construction/destruction order. No fake
helper, inline directive or dummy operation. RMG has no Dreamcast counterpart.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    for mask, ownership, reset in itertools.product(range(5), range(3), range(4)):
        body = original
        passable = "prototype->m_passableMask[CObjectType::getBitPos(x, y)]"
        if mask == 1:
            body = body.replace("!" + passable, "~" + passable)
        elif mask == 2:
            body = body.replace(passable, "prototype->m_passableMask.at(CObjectType::getBitPos(x, y))")
        elif mask in (3, 4):
            decl = "bool" if mask == 3 else "std::bitset<48>::reference"
            body = body.replace("                    if (!" + passable,
                "                    const " + decl + " passable = " + passable + ";\n                    if (!passable")
        if ownership:
            old = "    std::vector<type_treasure_def*> candidates;\n    std::vector<TRmgObjectPropertiesRef*> properties;"
            if ownership == 1:
                setup = "    std::pair<std::vector<type_treasure_def*>,\n        std::vector<TRmgObjectPropertiesRef*> > choices;"
                first, second = "choices.first", "choices.second"
            else:
                setup = ("    struct TreasureChoices {\n"
                         "        std::vector<type_treasure_def*> candidates;\n"
                         "        std::vector<TRmgObjectPropertiesRef*> properties;\n"
                         "    } choices;")
                first, second = "choices.candidates", "choices.properties"
            setup += ("\n    std::vector<type_treasure_def*>& candidates = " + first + ";"
                      "\n    std::vector<TRmgObjectPropertiesRef*>& properties = " + second + ";")
            assert body.count(old) == 1
            body = body.replace(old, setup)
        for name, typ in (("candidates", "type_treasure_def"), ("properties", "TRmgObjectPropertiesRef")):
            operation = [name + ".clear();", name + ".erase(" + name + ".begin(), " + name + ".end());",
                         name + ".resize(0);", name + ".assign(0, static_cast<" + typ + "*>(0));"][reset]
            body = body.replace(name + ".clear();", operation)
        yield dict(name=f"mask_{mask}+ownership_{ownership}+reset_{reset}", replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-treasure-create-family.py")
    original = helper.helpers().definition((HOMM3_DIR / helper.SOURCE).read_text(), helper.FUNCTION)
    options = list(variants(original))
    assert len(options) == len({x['replace'] for x in options}) == 60
    assert options[0]['replace'] == original
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="treasure_container_ownership", source=helper.SOURCE, find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("60 treasure proxy/container/reset states")


if __name__ == "__main__":
    main()
