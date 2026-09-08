#!/usr/bin/env python3
"""Generate 60 complete terrain-selector source states for retail 0x532ab0.

Complete-only RMG has no Dreamcast counterpart. The selector's byte tests,
two eight-terrain scans, one conditional rand call and final underground
restriction are retail evidence. Cross three template bindings, four count
loop structures and five rank-selection forms, preserving that behavior.
In particular z==1 is not a general nonzero test, native town terrain bypasses
the allowed mask, and the eligible list excludes subterranean at other levels.
No inlining gates, helper flattening or fabricated compiler-budget operations.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "TRmgZone::chooseTerrain"


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_zone_terrain_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def body(binding, counting, selection):
    setup = ("", "    TRmgTownSlot* slot = m_slot;\n", "    const TRmgTownSlot& slot = *m_slot;\n")[binding]
    access = ("m_slot->", "slot->", "slot.")[binding]
    result = "void TRmgZone::chooseTerrain()\n{\n" + setup
    result += "    if (" + access + "m_useNativeTerrain && m_alignment != -1) {\n"
    result += "        m_terrain = g_rmgTownNativeTerrains[m_alignment];\n    } else {\n        int count = 0;\n"
    eligible = access + "m_allowedTerrain[terrain]\n                && (terrain != eTerrainSubterranean || m_levelPosition.m_z == 1)"
    if counting == 1:
        result += "        int terrain = 0;\n        while (terrain < eTerrainWater) {\n"
    else:
        result += "        for (int terrain = 0; terrain < eTerrainWater; ++terrain) {\n"
    if counting == 2:
        result += "            if (!" + access + "m_allowedTerrain[terrain])\n                continue;\n"
        result += "            if (terrain == eTerrainSubterranean && m_levelPosition.m_z != 1)\n                continue;\n"
        result += "            ++count;\n"
    elif counting == 3:
        result += "            if (" + access + "m_allowedTerrain[terrain]) {\n"
        result += "                if (terrain != eTerrainSubterranean || m_levelPosition.m_z == 1)\n                    ++count;\n            }\n"
    else:
        result += "            if (" + eligible + ")\n                ++count;\n"
    if counting == 1:
        result += "            ++terrain;\n"
    result += "        }\n        if (!count) {\n            m_terrain = eTerrainDirt;\n        } else {\n"
    result += "            int selected = rand() % count;\n            int terrain;\n"
    result += "            for (terrain = 0; terrain < eTerrainWater; ++terrain) {\n"
    inner_eligible = eligible.replace("\n                ", "\n                    ")
    if selection == 3:
        result += "                if (" + inner_eligible + "\n                    && selected-- <= 0)\n                    break;\n"
    else:
        result += "                if (" + inner_eligible + ") {\n"
        condition = ("selected-- <= 0", "--selected < 0", "selected-- == 0", "", "selected == 0")[selection]
        result += "                    if (" + condition + ")\n                        break;\n"
        if selection == 4:
            result += "                    --selected;\n"
        result += "                }\n"
    result += "            }\n            m_terrain = terrain;\n        }\n    }\n"
    result += "    if (m_levelPosition.m_z == 1 && m_terrain != eTerrainLava)\n        m_terrain = eTerrainSubterranean;\n}"
    return result


def forms():
    bindings = ("member_slot", "slot_pointer", "slot_reference")
    counts = ("count_for", "count_while", "count_continue", "count_nested")
    selections = ("post_nonpositive", "pre_negative", "post_zero", "combined", "zero_then_decrement")
    for binding, counting, selection in itertools.product(range(3), range(4), range(5)):
        yield bindings[binding] + "+" + counts[counting] + "+" + selections[selection], body(binding, counting, selection)


def make_axes(source):
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    options = list(forms())
    if original not in dict(options).values():
        raise ValueError("review the current terrain-selector operations before rebasing")
    return [helper.axis("zone_terrain_selection", SOURCE, original, options)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=make_axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 zone terrain-selection states ->", args.output)


if __name__ == "__main__":
    main()
