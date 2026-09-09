#!/usr/bin/env python3
"""60 source states for createRivers, retail 0x549870.

Retail and baseline have identical control flow and named helper calls.
Retail captures both trigger components before copying the wheel position.
Cross five real trigger-offset forms, four object/prototype bindings, and
three position lifetimes. Preserve the canonical coordinate classes and
ordinary compound-subtraction helper, preparation order, live vector bounds,
post-routing progress lookup and two-cell displacement. No DC counterpart.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::createRivers"
BASELINE = """void type_random_map_generator::createRivers()
{
    markRiverObjectTargets();
    markRiverTargets();
    for (unsigned int index = 0; index < m_positions.size(); ++index) {
        type_object* object = m_positions[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->m_objectType == WATER_WHEEL) {
            TRmgMapPosition position = object->m_position;
            position.m_x -= prototype->m_triggerCell.m_x;
            position.m_y -= prototype->m_triggerCell.m_y;
            createRiverToObject(position);
            position.m_x -= 2;
            createRiver(position);
            if (m_progress)
                m_progress->advance(1000);
        }
    }
}"""


def helpers():
    spec = importlib.util.spec_from_file_location("river_coordinator_helpers",
        Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def forms():
    position = "            TRmgMapPosition position = object->m_position;\n"
    original = position + "            position.m_x -= prototype->m_triggerCell.m_x;\n" + "            position.m_y -= prototype->m_triggerCell.m_y;\n"
    offsets = [original,
        "            TObjectType::TPoint trigger = prototype->m_triggerCell;\n" + position +
        "            position.m_x -= trigger.m_x;\n            position.m_y -= trigger.m_y;\n",
        "            const TObjectType::TPoint& trigger = prototype->m_triggerCell;\n" + position +
        "            position.m_x -= trigger.m_x;\n            position.m_y -= trigger.m_y;\n",
        "            int triggerX = prototype->m_triggerCell.m_x;\n            int triggerY = prototype->m_triggerCell.m_y;\n" + position +
        "            position.m_x -= triggerX;\n            position.m_y -= triggerY;\n",
        position + "            position -= TPoint(prototype->m_triggerCell.m_x, prototype->m_triggerCell.m_y);\n"]
    for offset, binding, lifetime in itertools.product(range(5), range(4), range(3)):
        body = BASELINE.replace(original, offsets[offset])
        if binding == 1:
            body = body.replace("TObjectType* prototype", "const TObjectType* prototype")
        elif binding == 2:
            body = body.replace("type_object* object", "const type_object* object").replace("TObjectType* prototype", "const TObjectType* prototype")
        elif binding == 3:
            body = body.replace("TObjectType* prototype = object->m_properties->m_prototype;",
                "TObjectType& prototype = *object->m_properties->m_prototype;").replace("prototype->", "prototype.")
        if lifetime:
            body = body.replace(position, "            position = object->m_position;\n")
            if lifetime == 1:
                body = body.replace("        " + ("const " if binding == 2 else "") + "type_object* object",
                    "        TRmgMapPosition position;\n        " + ("const " if binding == 2 else "") + "type_object* object")
            else:
                body = body.replace("    for (unsigned int index", "    TRmgMapPosition position;\n    for (unsigned int index")
        yield "offset_" + str(offset) + "+binding_" + str(binding) + "+lifetime_" + str(lifetime), body


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in {body for _, body in alternatives}:
        raise ValueError("review the current river coordinator")
    result = helpers().axis("river_coordinator", SOURCE, original, alternatives)
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct river coordinator states")
    return [result]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
        axes=make_axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 river coordinator states ->", args.output)


if __name__ == "__main__":
    main()
