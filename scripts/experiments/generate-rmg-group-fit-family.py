#!/usr/bin/env python3
"""Cross 60 source states for Complete-only treasure fitting at 0x5355e0.

Retail retains two isPlacementBlocked calls and three ordered surface-neighbor
scans. The current body has a 0x10 frame versus retail's 0x18 and redirects
early failures to a later epilogue. Cross six real origin lifetimes, five
neighbor constructions and two failure-exit structures. Preserve direction
ranges, byte predicates, first entrance-object selection, all helper interfaces,
the unchanged by-value placement argument and the monster/border-guard policy.
There is no Dreamcast counterpart. No helper flattening, dummy operations,
new inline qualifiers, layout variants or pragma controls enter the candidates.
"""
import argparse
import functools
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "TRmgTreasureGroup::canFitObject"
PARENT_EVIDENCE = (
    "Ten reproduced group-fit parents plus a current-source control and fresh "
    "point/vector construction and failure-routing states. The 96.2381% parent "
    "restores retail's 0x18 frame and first two y stores, but walks a table base "
    "biased by four bytes and routes early failures to the final zero return. "
    "Keep all canonical helpers and their interfaces; explore the point-valued "
    "direction operand, returned-value assignment, compound translation and "
    "retail's first failure join, never unsigned pointer loops or inline pins.")
ENTRY_EVIDENCE = (
    "Ten reproduced 98.8042% frontier parents, current-source control and fresh "
    "entry coordinate lifetimes. Retail loads both placement coordinates before "
    "the object kind and trigger coordinates, while the parent loads placement "
    "y later. Preserve the point/vector helpers, direction scans and all exits; "
    "test actual placement and trigger snapshots and the prototype reference.")


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_group_fit_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def origins():
    kind = "    int objectType = prototype->m_objectType;\n"
    copies = "    int x = position.m_x;\n    int y = position.m_y;\n"
    translate = "    x -= prototype->m_triggerCell.m_x;\n    y -= prototype->m_triggerCell.m_y;\n"
    yield "scalar_copies", copies + kind + translate, "x", "y"
    yield "kind_first", kind + copies + translate, "x", "y"
    yield "scalar_expressions", ("    int x = position.m_x - prototype->m_triggerCell.m_x;\n"
                                 "    int y = position.m_y - prototype->m_triggerCell.m_y;\n" + kind), "x", "y"
    yield "point_fields", ("    TPoint origin;\n"
                           "    origin.m_x = position.m_x - prototype->m_triggerCell.m_x;\n"
                           "    origin.m_y = position.m_y - prototype->m_triggerCell.m_y;\n" + kind), "origin.m_x", "origin.m_y"
    yield "point_constructed", (kind + "    TPoint origin(position.m_x - prototype->m_triggerCell.m_x,\n"
                                "        position.m_y - prototype->m_triggerCell.m_y);\n"), "origin.m_x", "origin.m_y"
    yield "position_copy", ("    TRmgMapPosition origin = position;\n" + kind
                            + "    origin.m_x -= prototype->m_triggerCell.m_x;\n"
                            "    origin.m_y -= prototype->m_triggerCell.m_y;\n"), "origin.m_x", "origin.m_y"


def nearby(shape, indent, x, y):
    dx, dy = "g_rmgDirections[direction].m_x", "g_rmgDirections[direction].m_y"
    forms = (
        ("TPoint nearby;", "nearby.m_x = " + dx + " + " + x + ";", "nearby.m_y = " + dy + " + " + y + ";"),
        ("TPoint nearby(" + dx + " + " + x + ", " + dy + " + " + y + ");",),
        ("TPoint nearby = TPoint(" + dx + " + " + x + ", " + dy + " + " + y + ");",),
        ("TPoint nearby = g_rmgDirections[direction];", "nearby.m_x += " + x + ";", "nearby.m_y += " + y + ";"),
        ("TPoint nearby = TPoint(" + x + ", " + y + ") + TRmgVector(" + dx + ", " + dy + ");",),
        ("TPoint nearby = g_rmgDirections[direction] + TRmgVector(" + x + ", " + y + ");",),
        ("TPoint nearby;", "nearby = g_rmgDirections[direction] + TRmgVector(" + x + ", " + y + ");"),
        ("TPoint nearby(g_rmgDirections[direction] + TRmgVector(" + x + ", " + y + "));",),
        ("TPoint nearby = g_rmgDirections[direction];", "nearby += TRmgVector(" + x + ", " + y + ");"),
        ("TPoint nearby(" + x + ", " + y + ");", "nearby += TRmgVector(" + dx + ", " + dy + ");"),
        ("TPoint nearby;", "nearby = TPoint(" + x + ", " + y + ") + TRmgVector(" + dx + ", " + dy + ");"),
    )
    return "".join(" " * indent + line + "\n" for line in forms[shape])


def scope_front(text):
    first = text.index("    for (int direction = 0; direction < 5;")
    last = text.index("    if (objectType != MONSTER", first)
    return (text[:first] + "    {\n"
            + "".join("    " + line + "\n" for line in text[first:last].splitlines())
            + "    }\n" + text[last:])


def early_failure(text):
    if text.count("return 0;") != 5:
        raise ValueError("first-failure join requires the five original failure returns")
    text = scope_front(text).replace("return 0;", "goto placementFailure;", 2)
    before = "        if (m_map.isPlacementBlocked(properties, position, -1, 1))\n            return 0;"
    after = ("        if (m_map.isPlacementBlocked(properties, position, -1, 1)) {\n"
             "placementFailure:\n            return 0;\n        }")
    if text.count(before) != 1:
        raise ValueError("review the first placement failure boundary")
    return text.replace(before, after)


def body(origin, query, exits):
    _, setup, x, y = list(origins())[origin]
    text = """unsigned char TRmgTreasureGroup::canFitObject(TRmgObjectPropertiesRef* properties,
    TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
@ORIGIN@    if (!g_adventureObjectLandBlocked[objectType][1]) {
        for (int direction = 5; direction < RMG_DIRECTION_COUNT; ++direction) {
@QUERY12@            if (m_map.getMapItem(nearby.m_x, nearby.m_y, 0)->isRoadEntrance())
                return 0;
        }
    }
    for (int direction = 0; direction < 5; ++direction) {
@QUERY8@        TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y, 0);
        if (item->isRoadEntrance()) {
            int neighborType = item->m_objects[0]->m_properties->m_prototype->m_objectType;
            if (!g_adventureObjectLandBlocked[neighborType][2]
                || !g_adventureObjectLandBlocked[neighborType][1])
                return 0;
        }
    }
    if (objectType != MONSTER && objectType != BORDER_GUARD) {
        if (m_map.isPlacementBlocked(properties, position, -1, 1))
            return 0;
    } else {
        if (m_map.isPlacementBlocked(properties, position, -1, 0))
            return 0;
        int direction;
        for (direction = 0; direction < RMG_DIRECTION_COUNT; ++direction) {
@QUERY12@            TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y, 0);
            if (!item->isRoadEntrance() && item->m_tileData.m_roadPassable
                && item->m_tile.m_landType != eTerrainRock && !item->hasBorderObject())
                break;
        }
        if (direction == RMG_DIRECTION_COUNT)
            return 0;
    }
    return 1;
}"""
    text = text.replace("@ORIGIN@", setup).replace("@QUERY12@", nearby(query, 12, x, y)).replace(
        "@QUERY8@", nearby(query, 8, x, y))
    if exits:
        # VC6 keeps a for-initializer in the enclosing scope. Give this
        # independent scan an explicit scope before jumping to the failure
        # exit; otherwise C2362 correctly rejects skipping its initializer.
        text = scope_front(text)
        text = text.replace("return 0;", "goto cannotFit;")
        text = text[:-1] + "cannotFit:\n    return 0;\n}"
    return text


def forms():
    origin_names = [item[0] for item in origins()]
    query_names = ("fields", "constructed", "copy_initialized", "offset_copy", "translated")
    for origin, query, exits in itertools.product(range(6), range(5), range(2)):
        yield (origin_names[origin] + "+" + query_names[query] + "+" + ("shared_failure" if exits else "local_returns"),
               body(origin, query, exits))


def make_axes(source):
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    options = list(forms())
    if not known_body(original):
        raise ValueError("review current group-fit operations before rebasing")
    axis = helper.axis("group_fit_lifetimes", SOURCE, original, options)
    # After adopting a later-stage body, keep it as the unchanged-source
    # control and retain a finite 60-state first-family population.
    axis["options"] = axis["options"][:60]
    return [axis]


def refinements(original):
    matches = [(origin, query, exits) for origin, query, exits in itertools.product(range(6), range(5), range(2))
               if body(origin, query, exits) == original]
    if len(matches) != 1:
        raise ValueError("review the parent source before generating refinements")
    origin, _, exits = matches[0]
    if not exits:
        yield "first_failure", early_failure(original)
    yield "direction_point", body(origin, 5, exits)
    if not exits:
        yield "direction_point_first_failure", early_failure(body(origin, 5, exits))
        yield "front_scope", scope_front(original)
    for shape, label in enumerate(("direction_assigned", "direction_direct", "direction_compound",
                                  "origin_compound", "origin_assigned"), 6):
        yield label, body(origin, shape, exits)


def semantic_forms():
    # All first-population parents and every permitted successor, independent
    # of which ten happen to win in a particular VC6 compiler snapshot.
    seen = set()
    for label, text in forms():
        for name, candidate in [(label, text), *[(label + "+" + n, t) for n, t in refinements(text)]]:
            if candidate not in seen:
                seen.add(candidate)
                yield name, candidate


def entry_refinements(original):
    prefix, rest = original.split("    if (!g_adventureObjectLandBlocked[objectType][1]) {", 1)
    separator = "    if (!g_adventureObjectLandBlocked[objectType][1]) {"
    pointer = "    TObjectType* prototype = properties->m_prototype;\n"
    if prefix.count(pointer) != 1:
        raise ValueError("review the prototype entry binding")

    def point_position(text):
        if "TRmgMapPosition origin = position;" in text:
            text = text.replace("TRmgMapPosition origin = position;",
                                "TRmgMapPosition origin;\n    origin.m_x = position.m_x;\n"
                                "    origin.m_y = position.m_y;\n    origin.m_z = position.m_z;")
        return text.replace("position.m_x", "placement.m_x").replace("position.m_y", "placement.m_y")

    insertions = (
        ("placement_copy", "    TRmgMapPosition placement = position;\n",
         prefix.replace("position.m_", "placement.m_").replace("origin = position;", "origin = placement;")),
        ("placement_point", "    TPoint placement(position.m_x, position.m_y);\n", point_position(prefix)),
        ("trigger_copy", "    TObjectType::TPoint trigger = prototype->m_triggerCell;\n",
         prefix.replace("prototype->m_triggerCell.m_", "trigger.m_")),
        ("trigger_reference", "    const TObjectType::TPoint& trigger = prototype->m_triggerCell;\n",
         prefix.replace("prototype->m_triggerCell.m_", "trigger.m_")),
        ("point_snapshots", "    TPoint placement(position.m_x, position.m_y);\n"
                            "    TObjectType::TPoint trigger = prototype->m_triggerCell;\n",
         point_position(prefix).replace("prototype->m_triggerCell.m_", "trigger.m_")),
    )
    for label, insertion, text in insertions:
        yield label, text.replace(pointer, pointer + insertion) + separator + rest
    yield "prototype_reference", (prefix.replace(pointer, "    TObjectType& prototype = *properties->m_prototype;\n")
                                  .replace("prototype->", "prototype.") + separator + rest)


@functools.lru_cache(maxsize=1)
def admitted_bodies():
    # Admission is an exact source-family membership check, not a permissive
    # rebase over arbitrary altered placement semantics.
    result = set()
    for _, method in semantic_forms():
        result.add(method)
        result.update(text for _, text in entry_refinements(method))
    return frozenset(result)


def known_body(original):
    return original in admitted_bodies()


def load_parents(checkpoint_path, source, *, frontier=False):
    directory = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                    axes=make_axes(source), evidence=__doc__)
    if frontier:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source)),
                        evidence=PARENT_EVIDENCE, parent_checkpoint=str(prior.resolve()))
    if payload != expected:
        raise ValueError("review the completed group-fit family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review parents against changed source: " + relative)
    saved = directory / "snapshot/include"
    current = HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {
            p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review parents against changed header population")
    for path in saved.rglob("*"):
        if path.is_file() and path.read_bytes() != (current / path.relative_to(saved)).read_bytes():
            raise ValueError("review parents against changed header: " + str(path.relative_to(saved)))
    records, elites = checkpoint["records"], checkpoint["elites"]
    if len(records) != 60 or any(not row.get("scores") for row in records) or len(elites) != 10:
        raise ValueError("expected all 60 scored states and ten reproduced elites")
    parents = []
    for row in elites:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("parent did not reproduce: " + row["id"])
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("parent source hash changed: " + row["id"])
        parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents, *, entry=False):
    if len(parents) != 10:
        raise ValueError("expected ten reproduced parents")
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    retained = [(label, helper.definition(text, FUNCTION)) for label, text in parents]
    if len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct parent sources")
    # Keep the entire ten-parent frontier, not just the strongest body. The
    # baseline is an additional control when it is absent from that frontier.
    options = helper.axis("group_fit_frontier", SOURCE, original, retained)["options"]
    seen = {option["replace"] for option in options}
    mutation = entry_refinements if entry else refinements
    inputs = [("baseline", original), *retained]
    if not entry and original not in dict(forms()).values():
        inputs = retained
    successors = [list(mutation(text)) for _, text in inputs]
    labels = [label for label, _ in inputs]
    for step in itertools.zip_longest(*successors):
        for parent, entry in zip(labels, step):
            if entry is None:
                continue
            label, text = entry
            if text not in seen:
                options.append(dict(name=parent + "+" + label, replace=text))
                seen.add(text)
            if len(options) == 60:
                return [dict(name="group_fit_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("frontier does not supply 60 distinct states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    parser.add_argument("--entry-parents-from", type=Path)
    args = parser.parse_args()
    if args.parents_from and args.entry_parents_from:
        parser.error("select only one parent stage")
    source = (HOMM3_DIR / SOURCE).read_text()
    checkpoint = args.entry_parents_from or args.parents_from
    entry = bool(args.entry_parents_from)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=(make_parent_axes(source, load_parents(checkpoint, source, frontier=entry), entry=entry)
                         if checkpoint else make_axes(source)),
                   evidence=ENTRY_EVIDENCE if entry else PARENT_EVIDENCE if checkpoint else __doc__)
    if checkpoint:
        payload["parent_checkpoint"] = str(checkpoint.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure-group fit states ->", args.output)


if __name__ == "__main__":
    main()
