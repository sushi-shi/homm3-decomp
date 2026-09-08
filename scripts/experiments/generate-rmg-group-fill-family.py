#!/usr/bin/env python3
"""Generate 60 treasure-group fill states from retail 0x546520.

The current 97.7697% body agrees with retail's control flow and helper calls,
but has a four-byte frame surplus and a different first-object centering load
order. Cross five centering value forms, four actual receiver/value bindings,
and three position lifetimes. All reads remain after successful creation and
before insertion. Keep unsigned 32-bit center sums, public push_back, canonical
factory/accessors, independent creation/fit budgets, and virtual cleanup.
Complete-only: no Dreamcast counterpart is claimed for this function.
"""
import argparse
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::fillTreasureGroup"
FRONTIER_EVIDENCE = (
    "The first 60 states reproduce 97.7697% with 48 distinct objects. "
    "Reusing the initial position for later creation calls expands the body "
    "from 438 to 821 bytes; scoping primary creation alone stays flat. "
    "Retail's centering sequence loads the group receiver between property "
    "lookup and the accepted-pointer store, then reads map width before "
    "prototype width. Retain ten reproduced parents; vary real map receiver "
    "bindings, coordinate assignment order and accepted-object scope. "
    "No opaque call is crossed and no canonical helper declaration changes.")
CENTER_EVIDENCE = (
    "The second 60 states reproduce 97.8371%: scoping the accepted object "
    "removes the four-byte frame surplus with the other 339 scores unchanged. "
    "Retail still reads map width before prototype width and prefetches the "
    "insertion end between dimension reads. Retain ten reproduced parents "
    "and test meaningful unsigned accumulation and coordinate read-back "
    "forms without crossing creation or insertion, adding dead work, or "
    "changing the public push_back/helper boundaries.")
BASELINE = """int type_random_map_generator::fillTreasureGroup(TRmgZone* zone,
    TRmgTreasureGroup* group, unsigned char alternate, int value)
{
    int objectValue = 0;
    int attempts;
    type_object* selected;
    TRmgMapPosition position;
    position.m_x = -1;
    position.m_y = -1;
    position.m_z = -1;
    for (attempts = 0; attempts < RMG_TREASURE_ATTEMPTS; ++attempts) {
        selected = createTreasureObject(zone, value / 4, value, &objectValue,
            1, 1, alternate, position);
        if (selected)
            break;
    }
    if (!selected)
        return 0;
    TObjectType* prototype = selected->m_properties->m_prototype;
    type_object* object = selected;
    position.m_x = (group->m_map.m_mapWidth + static_cast<unsigned>(prototype->getWidth())) / 2;
    position.m_y = (group->m_map.m_mapHeight + static_cast<unsigned>(prototype->getHeight())) / 2;
    position.m_z = 0;
    group->m_objects.push_back(object);
    group->m_map.addObject(object, position);
    int total = objectValue;
    while (total < value) {
        int remainder = value - total;
        if (remainder < RMG_TREASURE_MINIMUM_REMAINDER && remainder < total / 2)
            break;
        type_object* nextObject;
        for (attempts = 0; ; ) {
            int creationAttempts;
            for (creationAttempts = 0; creationAttempts < RMG_TREASURE_ATTEMPTS;
                 ++creationAttempts) {
                TRmgMapPosition unspecified;
                unspecified.m_x = -1;
                unspecified.m_y = -1;
                unspecified.m_z = -1;
                nextObject = createTreasureObject(zone, remainder / 4, 5 * remainder / 4,
                    &objectValue, 0, 1, alternate, unspecified);
                if (nextObject)
                    break;
            }
            if (!nextObject)
                break;
            if (group->tryAddObject(nextObject))
                break;
            nextObject->unknownOperation();
            delete nextObject;
            if (++attempts >= RMG_TREASURE_ATTEMPTS)
                goto groupFilled;
        }
        if (!nextObject)
            break;
        total += objectValue;
    }
groupFilled:
    group->updateBounds();
    return total;
}"""


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_group_fill_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def forms():
    center = "\n".join(line for line in BASELINE.splitlines() if line.startswith("    position.m_") and "-1" not in line) + "\n"
    binding = ("    TObjectType* prototype = selected->m_properties->m_prototype;\n"
               "    type_object* object = selected;\n")
    centers = [center,
        "    int mapWidth = group->m_map.m_mapWidth;\n" + center.replace("group->m_map.m_mapWidth", "mapWidth"),
        "    int mapWidth = group->m_map.m_mapWidth;\n    int mapHeight = group->m_map.m_mapHeight;\n"
        + center.replace("group->m_map.m_mapWidth", "mapWidth").replace("group->m_map.m_mapHeight", "mapHeight"),
        "    unsigned centerX = group->m_map.m_mapWidth + static_cast<unsigned>(prototype->getWidth());\n"
        "    unsigned centerY = group->m_map.m_mapHeight + static_cast<unsigned>(prototype->getHeight());\n"
        "    position.m_x = centerX / 2;\n    position.m_y = centerY / 2;\n    position.m_z = 0;\n",
        "    TRmgVector dimensions(group->m_map.m_mapWidth, group->m_map.m_mapHeight);\n"
        + center.replace("group->m_map.m_mapWidth", "dimensions.m_x").replace("group->m_map.m_mapHeight", "dimensions.m_y")]
    bindings = [binding,
        "    type_object* object = selected;\n    TObjectType* prototype = selected->m_properties->m_prototype;\n",
        "    TRmgObjectPropertiesRef* properties = selected->m_properties;\n"
        "    type_object* object = selected;\n    TObjectType* prototype = properties->m_prototype;\n",
        binding + "    std::vector<type_object*>& objects = group->m_objects;\n"]
    for c, b, lifetime in itertools.product(range(5), range(4), range(3)):
        body = BASELINE.replace(binding, bindings[b]).replace(center, centers[c])
        if b == 3:
            body = body.replace("group->m_objects.push_back(object);", "objects.push_back(object);")
        if lifetime == 1:
            start = body.index("    TRmgMapPosition position;")
            end = body.index("    if (!selected)")
            block = body[start:end]
            body = body[:start] + "    {\n" + "".join("    " + line + "\n" for line in block.splitlines()) + "    }\n" + body[end:]
            guard = "    if (!selected)\n        return 0;\n"
            body = body.replace(guard, guard + "    TRmgMapPosition position;\n")
        elif lifetime == 2:
            body = body.replace("                TRmgMapPosition unspecified;\n", "").replace("unspecified", "position")
        yield "center_" + str(c) + "+binding_" + str(b) + "+lifetime_" + str(lifetime), body


def refinements(body):
    start = body.index("    TObjectType* prototype")
    if "    type_object* object = selected;\n" in body[:start]:
        start = body.index("    type_object* object = selected;")
    if "    TRmgObjectPropertiesRef* properties" in body[:start]:
        start = body.index("    TRmgObjectPropertiesRef* properties")
    end = body.index("    int total = objectValue;")
    first_object = body[start:end]
    for label, declaration, receiver in (
            ("map_reference", "    type_random_map& map = group->m_map;\n", "map."),
            ("map_pointer", "    type_random_map* map = &group->m_map;\n", "map->")):
        yield label, body[:start] + declaration + first_object.replace("group->m_map.", receiver) + body[end:]
    scoped = "    {\n" + "".join("    " + line + "\n" for line in first_object.splitlines()) + "    }\n"
    yield "accepted_object_scope", body[:start] + scoped + body[end:]
    yield "const_prototype", body.replace("TObjectType* prototype", "const TObjectType* prototype")
    lines = body.splitlines(True)
    x = next(i for i, line in enumerate(lines) if line.startswith("    position.m_x = ") and "-1" not in line)
    y = next(i for i, line in enumerate(lines) if line.startswith("    position.m_y = ") and "-1" not in line)
    lines[x], lines[y] = lines[y], lines[x]
    yield "center_yx", "".join(lines)
    # The output total survives the initial-object phase, but its selected
    # pointer and position do not need to. Keep the common retry counter alive.
    start = body.index("    type_object* selected;")
    end = body.index("    while (total < value)")
    initial = body[start:end].replace("    int total = objectValue;", "    total = objectValue;")
    surviving_position = ""
    if "0, 1, alternate, position);" in body:
        initial = initial.replace("    TRmgMapPosition position;\n", "", 1)
        surviving_position = "    TRmgMapPosition position;\n"
    scoped = surviving_position + "    int total;\n    {\n" + "".join("    " + line + "\n" for line in initial.splitlines()) + "    }\n"
    yield "initial_object_phase_scope", body[:start] + scoped + body[end:]


def admitted_bodies():
    first = {body for _, body in forms()}
    second = first | {body for parent in first for _, body in refinements(parent)}
    return second | {body for parent in second for _, body in center_refinements(parent)}


def center_refinements(body):
    at = body.index("TObjectType* prototype")
    match = re.search(r"(?m)^( +)(?:int mapWidth = |unsigned centerX = |TRmgVector dimensions\(|position\.m_[xy] = )", body[at:])
    if match is None:
        raise ValueError("review the first-object centering values")
    start = at + match.start()
    stop = body.index("position.m_z = 0;", start) + len("position.m_z = 0;\n")
    original = body[start:stop]
    receiver_match = re.search(r"(group->m_map\.|map\.|map->)m_mapWidth", original)
    if receiver_match is None:
        raise ValueError("review the first-object map receiver")
    receiver = receiver_match.group(1)
    width, height = receiver + "m_mapWidth", receiver + "m_mapHeight"
    x, y = "prototype->getWidth()", "prototype->getHeight()"
    stores = ["position.m_x = centerX / 2;", "position.m_y = centerY / 2;"]
    dims = ["unsigned centerX = " + width + ";", "unsigned centerY = " + height + ";"]
    additions = ["centerX += " + x + ";", "centerY += " + y + ";"]
    field_dims = ["position.m_x = " + width + ";", "position.m_y = " + height + ";"]
    field_halves = ["position.m_x = (static_cast<unsigned>(position.m_x) + static_cast<unsigned>(" + x + ")) / 2;",
                   "position.m_y = (static_cast<unsigned>(position.m_y) + static_cast<unsigned>(" + y + ")) / 2;"]
    alternatives = (
        ("axis_accumulators", [dims[0], additions[0], stores[0], dims[1], additions[1], stores[1]]),
        ("dimension_accumulators", dims + additions + stores),
        ("prototype_accumulators", ["unsigned centerX = " + x + ";", "unsigned centerY = " + y + ";",
                                     "centerX += " + width + ";", "centerY += " + height + ";"] + stores),
        ("halved_accumulators", dims + ["centerX = (centerX + " + x + ") / 2;",
                                         "centerY = (centerY + " + y + ") / 2;",
                                         "position.m_x = centerX;", "position.m_y = centerY;"]),
        ("coordinate_readback", field_dims + field_halves),
        ("axis_readback", [field_dims[0], field_halves[0], field_dims[1], field_halves[1]]),
        ("dimension_accumulators_yx", list(reversed(dims)) + additions + stores),
        ("prototype_additions_yx", dims + list(reversed(additions)) + stores),
        ("coordinate_stores_yx", dims + additions + list(reversed(stores))),
        ("axis_prototype_accumulators", ["unsigned centerX = " + x + ";", "centerX += " + width + ";", stores[0],
                                           "unsigned centerY = " + y + ";", "centerY += " + height + ";", stores[1]]),
        ("staged_axis_sums", [dims[0], additions[0], dims[1], additions[1]] + stores),
        ("named_dimension_accumulators", ["int mapWidth = " + width + ";", "int mapHeight = " + height + ";",
                                             "unsigned centerX = mapWidth;", "unsigned centerY = mapHeight;"] + additions + stores),
    )
    for label, lines in alternatives:
        replacement = "".join(match.group(1) + line + "\n" for line in lines + ["position.m_z = 0;"])
        yield label, body[:start] + replacement + body[stop:]


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in admitted_bodies():
        raise ValueError("review the current fill helper before generating")
    result = helpers().axis("group_fill", SOURCE, original, alternatives)
    result["options"] = result["options"][:60]
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct fill states")
    return [result]


def load_parents(checkpoint_path, source, *, frontier=False):
    directory = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                    axes=make_axes(source), evidence=__doc__)
    if frontier:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source)),
                        evidence=FRONTIER_EVIDENCE, parent_checkpoint=str(prior.resolve()))
    if payload != expected:
        raise ValueError("review the completed fill family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review fill parents against changed source: " + relative)
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {
            p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review fill parents against changed header population")
    for path in saved.rglob("*"):
        if path.is_file() and path.read_bytes() != (current / path.relative_to(saved)).read_bytes():
            raise ValueError("review fill parents against changed header: " + str(path.relative_to(saved)))
    if len(checkpoint["records"]) != 60 or any(not row.get("scores") for row in checkpoint["records"]) or len(checkpoint["elites"]) != 10:
        raise ValueError("expected all 60 scored states and ten reproduced elites")
    parents = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("fill parent did not reproduce: " + row["id"])
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("fill parent source hash changed: " + row["id"])
        parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents, *, centering=False):
    if len(parents) != 10:
        raise ValueError("expected ten reproduced fill parents")
    original = helpers().definition(source, FUNCTION)
    retained = [(label, helpers().definition(text, FUNCTION)) for label, text in parents]
    if len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct fill parent sources")
    options = helpers().axis("group_fill_frontier", SOURCE, original, retained)["options"]
    seen = {option["replace"] for option in options}
    inputs = [("baseline", original), *retained]
    successors = []
    for index, (_, text) in enumerate(inputs):
        mutations = list((center_refinements if centering else refinements)(text))
        rotation = index % len(mutations)
        successors.append(mutations[rotation:] + mutations[:rotation])
    for step in itertools.zip_longest(*successors):
        for (parent, _), entry in zip(inputs, step):
            if entry is None:
                continue
            label, text = entry
            if text not in seen:
                options.append(dict(name=parent + "+" + label, replace=text))
                seen.add(text)
            if len(options) == 60:
                return [dict(name="group_fill_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("fill frontier does not supply 60 distinct states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    parser.add_argument("--centering-parents-from", type=Path)
    args = parser.parse_args()
    if args.parents_from and args.centering_parents_from:
        parser.error("select only one parent stage")
    source = (HOMM3_DIR / SOURCE).read_text()
    checkpoint = args.centering_parents_from or args.parents_from
    centering = bool(args.centering_parents_from)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=make_parent_axes(source, load_parents(checkpoint, source, frontier=centering), centering=centering)
                        if checkpoint else make_axes(source),
                   evidence=CENTER_EVIDENCE if centering else FRONTIER_EVIDENCE if checkpoint else __doc__)
    if checkpoint:
        payload["parent_checkpoint"] = str(checkpoint.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure-group fill states ->", args.output)


if __name__ == "__main__":
    main()
