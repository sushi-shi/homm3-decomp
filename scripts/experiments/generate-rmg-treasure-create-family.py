#!/usr/bin/env python3
"""60 retail-grounded createTreasureObject states (0x546190).

Cross the public mask interfaces (subscript/test), independent vector
clear/erase operations, and actual footprint value bindings/lifetimes.
Retail retains passability test and both erase calls. No Dreamcast counterpart
exists in the current roster; preserve the retail-only filtering, weighted
selection, parallel containers and virtual factory/value boundaries.
"""
import argparse
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::createTreasureObject"
FRONTIER_EVIDENCE = (
    "First 60 states yield 56 distinct objects, peak unchanged at 81.6894%. "
    "Public erase on the candidates vector still expands its copy loop; "
    "direct test expands exception construction. Preserve ten reproduced "
    "parents and explore actual scan/type/footprint lifetimes, the generator "
    "receiver, a separate normalized score and public insertion boundaries. "
    "No helper is flattened, fabricated, pinned or redeclared.")
BASELINE = """type_object* type_random_map_generator::createTreasureObject(TRmgZone* zone,
    int minimum, int maximum, int* value, unsigned char primary,
    unsigned char allowTerrainDependent, unsigned char compact,
    TRmgMapPosition position)
{
    int zoneIndex = zone->m_slot->m_zoneIndex;
    int totalWeight = 0;
    std::vector<type_treasure_def*> candidates;
    std::vector<TRmgObjectPropertiesRef*> properties;
    int bestValuePerCell = 0;
    for (unsigned int index = 0; index < m_objectGenerators.size(); ++index) {
        type_treasure_def* definition = m_objectGenerators[index];
        int objectType = definition->m_objectType;
        if (!primary && g_adventureObjectLandBlocked[objectType][0]
            && !g_adventureObjectLandBlocked[objectType][2])
            continue;
        if (!allowTerrainDependent && definition->isTerrainDependent())
            continue;
        if (m_objectCountByType[objectType] >= g_rmgMapObjectLimits[objectType])
            continue;
        if (zone->m_objectCountByType[objectType] >= g_rmgZoneObjectLimits[objectType])
            continue;
        int objectValue = definition->getValue(zone, this);
        if (objectValue < 0 || objectValue < minimum || objectValue > maximum)
            continue;
        TRmgObjectPropertiesRef* candidate = selectObjectPrototype(
            zone->m_terrain, definition->m_objectType, definition->m_subtype);
        if (!candidate)
            continue;
        if (position.m_x >= 0 && m_map.isPlacementBlocked(candidate, position, zoneIndex, 1))
            continue;
        if (compact) {
            TObjectType* prototype = candidate->m_prototype;
            int occupied = 0;
            for (unsigned int x = 0; x < prototype->getWidth(); ++x) {
                for (unsigned int y = 0; y < prototype->getHeight(); ++y) {
                    if (!prototype->m_passableMask[CObjectType::getBitPos(x, y)]
                        || prototype->m_triggerMask[CObjectType::getBitPos(x, y)])
                        ++occupied;
                }
            }
            objectValue /= occupied;
            if (objectValue < 3 * bestValuePerCell / 4)
                continue;
            if (bestValuePerCell < 3 * objectValue / 4) {
                totalWeight = 0;
                candidates.clear();
                properties.clear();
                bestValuePerCell = objectValue;
            }
        }
        totalWeight += definition->m_density;
        candidates.push_back(definition);
        properties.push_back(candidate);
    }
    if (!candidates.size())
        return 0;
    int selected = rand() % totalWeight;
    unsigned int selectedIndex;
    for (selectedIndex = 0; selectedIndex < candidates.size(); ++selectedIndex) {
        selected -= candidates[selectedIndex]->m_density;
        if (selected < 0)
            break;
    }
    type_treasure_def* definition = candidates[selectedIndex];
    *value = definition->getValue(zone, this);
    return definition->generate(properties[selectedIndex], this, zone);
}"""


def helpers():
    spec = importlib.util.spec_from_file_location("treasure_create_helpers",
        Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def forms():
    for mask, clear, binding in itertools.product(range(3), range(4), range(5)):
        body = BASELINE
        for name, test in (("passable", mask >= 1), ("trigger", mask == 2)):
            if test:
                body = body.replace("m_" + name + "Mask[CObjectType::getBitPos(x, y)]",
                                    "m_" + name + "Mask.test(CObjectType::getBitPos(x, y))")
        for name, bit in (("candidates", 1), ("properties", 2)):
            if clear & bit:
                body = body.replace(name + ".clear();", name + ".erase(" + name + ".begin(), " + name + ".end());")
        prototype = "            TObjectType* prototype = candidate->m_prototype;\n"
        count = "            int occupied = 0;\n"
        if binding == 1:
            body = body.replace(prototype + count, count + prototype)
        elif binding == 2:
            body = body.replace(prototype, prototype.replace("TObjectType*", "const TObjectType*"))
        elif binding == 3:
            body = body.replace("CObjectType::getBitPos(x, y)", "bitIndex")
            body = body.replace("                    if (!prototype", "                    unsigned bitIndex = CObjectType::getBitPos(x, y);\n                    if (!prototype")
        elif binding == 4:
            body = body.replace(count, count + "            const std::bitset<48>& passable = prototype->m_passableMask;\n"
                "            const std::bitset<48>& trigger = prototype->m_triggerMask;\n")
            body = body.replace("!prototype->m_passableMask", "!passable").replace("|| prototype->m_triggerMask", "|| trigger")
        yield "mask_" + str(mask) + "+clear_" + str(clear) + "+binding_" + str(binding), body


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in {body for _, body in alternatives}:
        raise ValueError("review the current treasure creation body")
    result = helpers().axis("treasure_create", SOURCE, original, alternatives)
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct treasure creation states")
    return [result]


def refinements(body):
    def scope(start, stop, text=body):
        first, last = text.index(start), text.index(stop)
        indent = start[:len(start) - len(start.lstrip())]
        return text[:first] + indent + "{\n" + "".join("    " + line + "\n" for line in text[first:last].splitlines()) + indent + "}\n" + text[last:]

    yield "object_type_scope", scope("        int objectType =", "        int objectValue =")
    yield "scan_scope", scope("    int bestValuePerCell =", "    if (!candidates.size())")
    declaration = "    std::vector<type_treasure_def*>& generators = m_objectGenerators;\n"
    altered = body.replace("m_objectGenerators", "generators")
    yield "generator_reference", altered.replace("    int bestValuePerCell =", declaration + "    int bestValuePerCell =")
    altered = body.replace("            int occupied = 0;\n", "")
    marker = "            const TObjectType* prototype" if "const TObjectType* prototype" in altered else "            TObjectType* prototype"
    altered = scope(marker, "            objectValue /= occupied;", altered)
    yield "footprint_scope", altered.replace("        if (compact) {\n", "        if (compact) {\n            int occupied = 0;\n")
    first, last = body.index("            objectValue /= occupied;"), body.index("        totalWeight +=")
    block = body[first:last].replace("objectValue /= occupied;", "int valuePerCell = objectValue / occupied;")
    block = block.replace("3 * objectValue", "3 * valuePerCell").replace("if (objectValue <", "if (valuePerCell <").replace("= objectValue;", "= valuePerCell;")
    yield "named_normalized_value", body[:first] + block + body[last:]
    yield "public_count_insert", body.replace("candidates.push_back(definition);", "candidates.insert(candidates.end(), 1, definition);").replace("properties.push_back(candidate);", "properties.insert(properties.end(), 1, candidate);")


def load_parents(path, source):
    directory = path.parent
    checkpoint = json.loads(path.read_text())
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=make_axes(source), evidence=__doc__)
    if payload != expected:
        raise ValueError("review the completed creation family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review changed creation source: " + relative)
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {
            p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review changed header population")
    for p in saved.rglob("*"):
        if p.is_file() and p.read_bytes() != (current / p.relative_to(saved)).read_bytes():
            raise ValueError("review changed header: " + str(p))
    if len(checkpoint["records"]) != 60 or any(not r.get("scores") for r in checkpoint["records"]) or len(checkpoint["elites"]) != 10:
        raise ValueError("expected 60 scored states and ten reproduced elites")
    parents = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("parent failed reproduction: " + row["id"])
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("parent source changed: " + row["id"])
        parents.append((row["id"], helpers().definition(text, FUNCTION)))
    return parents


def make_parent_axes(source, parents):
    if len(parents) != 10 or len({body for _, body in parents}) != 10:
        raise ValueError("expected ten distinct parents")
    original = helpers().definition(source, FUNCTION)
    result = helpers().axis("treasure_create_frontier", SOURCE, original, parents)
    seen = {option["replace"] for option in result["options"]}
    successors = []
    for i, (_, body) in enumerate(parents):
        options = list(refinements(body))
        rotation = i % len(options)
        successors.append(options[rotation:] + options[:rotation])
    for step in zip(*successors):
        for (parent, _), (label, body) in zip(parents, step):
            if body not in seen:
                result["options"].append(dict(name=parent + "+" + label, replace=body))
                seen.add(body)
            if len(seen) == 60:
                return [result]
    raise ValueError("frontier requires 60 distinct states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
        axes=make_parent_axes(source, load_parents(args.parents_from, source)) if args.parents_from else make_axes(source),
        evidence=FRONTIER_EVIDENCE if args.parents_from else __doc__)
    if args.parents_from:
        payload["parent_checkpoint"] = str(args.parents_from.resolve())
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure creation states ->", args.output)


if __name__ == "__main__":
    main()
