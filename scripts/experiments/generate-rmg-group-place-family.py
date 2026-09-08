#!/usr/bin/env python3
"""Generate 60 source states for canPlaceTreasureGroup at 0x546c70.

Retail's 1106-byte body validates objects, a guard's 3x3 neighborhood, a
trait-selected entrance range, the connected outline and occupied bounds.
The 90.8130% baseline retains all three calls but has a 0x3c/0x48 frame gap
and a differently lowered direction-range join. Cross five guard-position
construction forms, four direction-initialization lifetimes and three
canonical neighbor-point constructions. Preserve all predicates, snapshots,
live object bounds, ordered lookups, helper declarations and retained calls.
No pasted helper bodies, dummy work, inline pins or invented type layouts.
"""
import argparse
import functools
import hashlib
import importlib.util
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::canPlaceTreasureGroup"
PARENT_EVIDENCE = (
    "Ten reproduced group-placement parents plus a current-source control "
    "and fresh lifetime/join forms. Point-plus-vector restores all 65 blocks "
    "and reaches 97.1072%, but the guard constructor argument loads and the "
    "outline-policy join still differ; the frame is 0x40 versus retail 0x48. "
    "Preserve the constructor and both map helper interfaces. Test the real "
    "guard-point snapshot, neighbor-result lifetime and explicit policy arms, "
    "without moving reads ahead of the initial placement-blocking calls.")
LIFETIME_EVIDENCE = (
    "Ten reproduced 98.7506% frontier parents plus a current-source control "
    "and fresh compositions. The real guard-point copy restores every guard "
    "constructor argument load and adds four frame bytes; the allow-else join "
    "independently restores the outline-policy branch destinations. Preserve "
    "both findings and test their combination, the guard scan as a real TPoint, "
    "and value-result assignment. Retail reuses the guard scan X home for the "
    "later two-dimensional neighbor, with a 0x48 frame versus the copy parent 0x44.")
FRAME_EVIDENCE = (
    "Ten reproduced 99.9102% lifetime parents plus a current-source control "
    "and new coordinate-scan compositions. Guard copy plus the allow arm "
    "matches all 65 blocks and the ordered helper calls; stack homes remain "
    "shifted and the frame is four bytes short. Test the guard scan as a "
    "three-coordinate position carrying its real map level, with both canonical "
    "map lookup overloads, and the bounds snapshot assignment. No padding, "
    "dummy local, altered bounds or false inline declaration enters the family.")
SHARED_EVIDENCE = (
    "Ten reproduced 99.9825% frame parents plus a current-source control and "
    "new working-position lifetimes. The three-coordinate guard scan restores "
    "retail's 0x48 frame and all guard, policy and final-scan instructions. "
    "Retail places the initial object-position and later entrance-position "
    "copies in the guard-position home, while the parent reuses the scan home. "
    "Test one real working position shared by selected phases, preserving the "
    "getter, constructor, argument order and all original snapshot boundaries.")


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_group_place_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


BASELINE = """unsigned char type_random_map_generator::canPlaceTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position, TRmgZone* zone)
{
    TRmgZoneBounds bounds = group->m_bounds;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    for (unsigned int i = 0; i < group->m_objects.size(); ++i) {
        type_object* object = group->m_objects[i];
        TRmgMapPosition objectPosition = object->getPosition();
        TRmgObjectPropertiesRef* properties = object->m_properties;
        objectPosition.m_x += position.m_x;
        objectPosition.m_y += position.m_y;
        objectPosition.m_z = position.m_z;
        if (m_map.isPlacementBlocked(properties, objectPosition, zoneIndex, 1))
            return 0;
    }
    if (group->m_hasGuard) {
        TRmgMapPosition guardPosition;
        guardPosition = TRmgMapPosition(group->m_guardPosition.m_x + position.m_x,
            group->m_guardPosition.m_y + position.m_y, position.m_z);
        if (guardPosition.m_x < 1 || guardPosition.m_x + 1 >= m_map.m_mapWidth
            || guardPosition.m_y < 1 || guardPosition.m_y + 1 >= m_map.m_mapHeight)
            return 0;
        for (int x = guardPosition.m_x - 1; x <= guardPosition.m_x + 1; ++x) {
            for (int y = guardPosition.m_y - 1; y <= guardPosition.m_y + 1; ++y) {
                TRmgMapItem* item = m_map.getMapItem(x, y, guardPosition.m_z);
                if (item->isRoadEntrance()
                    && item->m_objects[0]->m_properties->m_prototype->m_objectType == MONSTER)
                    return 0;
            }
        }
    }
    int lastDirection = RMG_DIRECTION_COUNT;
    int firstDirection = 0;
    unsigned char waterZone = zone->m_terrain == eTerrainWater;
    type_object* lastObject = group->m_objects.back();
    TObjectType* prototype = lastObject->m_properties->m_prototype;
    TRmgMapPosition entrance = lastObject->getPosition();
    entrance.m_x -= prototype->m_triggerCell.m_x;
    entrance.m_y -= prototype->m_triggerCell.m_y;
    if (!g_adventureObjectLandBlocked[prototype->m_objectType][1]) {
        firstDirection = 1;
        lastDirection = 4;
    }
    int direction;
    for (direction = firstDirection; direction < lastDirection; ++direction) {
        TPoint point;
        point.m_x = entrance.m_x + g_rmgDirections[direction].m_x;
        point.m_y = entrance.m_y + g_rmgDirections[direction].m_y;
        TRmgMapItem* source = group->m_map.getMapItem(point.m_x, point.m_y, 0);
        if (!source->hasSubterraneanGate() || !source->m_tileData.m_roadPassable
            || source->m_tile.m_landType == eTerrainRock || source->isRoadEntrance()
            || !source->isPlacementOutline())
            continue;
        point.m_x += position.m_x;
        point.m_y += position.m_y;
        if (point.m_x < 0 || point.m_x >= m_map.m_mapWidth
            || point.m_y < 0 || point.m_y >= m_map.m_mapHeight)
            continue;
        TRmgMapItem* destination = m_map.getMapItem(point.m_x, point.m_y, position.m_z);
        if ((destination->m_tile.m_landType == eTerrainWater) == waterZone
            && destination->m_tileData.m_roadPassable
            && destination->m_tile.m_landType != eTerrainRock
            && !destination->isRoadEntrance() && destination->hasSubterraneanGate())
            break;
    }
    if (direction == lastDirection)
        return 0;
    int allowEntrances;
    for (unsigned int objectIndex = 0; objectIndex < group->m_objects.size(); ++objectIndex) {
        int objectType = group->m_objects[objectIndex]->m_properties->m_prototype->m_objectType;
        if (!g_adventureObjectLandBlocked[objectType][2])
            goto disallowEntrances;
    }
    if (group->m_hasGuard)
        goto disallowEntrances;
    allowEntrances = 1;
    goto checkOutline;
disallowEntrances:
    allowEntrances = 0;
checkOutline:
    if (!m_map.hasConnectedOutline(group->m_outline, position, allowEntrances, zone, 1))
        return 0;
    TPoint point;
    for (point.m_y = bounds.m_minimumY; point.m_y < bounds.m_maximumY; ++point.m_y) {
        for (point.m_x = bounds.m_minimumX; point.m_x < bounds.m_maximumX; ++point.m_x) {
            TRmgMapItem* source = group->m_map.getMapItem(point.m_x, point.m_y, 0);
            if (!source->hasSubterraneanGate()) {
                int x = point.m_x + position.m_x;
                int y = point.m_y + position.m_y;
                if (x < m_map.m_mapWidth && y < m_map.m_mapHeight
                    && m_map.getMapItem(x, y, position.m_z)->isRoadEntrance())
                    return 0;
            }
        }
    }
    return 1;
}"""


def body(construction, direction, query):
    text = BASELINE
    arguments = ("group->m_guardPosition.m_x + position.m_x,\n"
                 "            group->m_guardPosition.m_y + position.m_y, position.m_z")
    value = "TRmgMapPosition(" + arguments + ")"
    constructors = (
        "        TRmgMapPosition guardPosition;\n        guardPosition = " + value + ";\n",
        "        TRmgMapPosition guardPosition = " + value + ";\n",
        "        TRmgMapPosition guardPosition(" + arguments + ");\n",
        "        const TRmgMapPosition& guardPosition = " + value + ";\n",
        "        TRmgMapPosition translatedGuard(" + arguments + ");\n"
        "        TRmgMapPosition guardPosition = translatedGuard;\n",
    )
    text = text.replace(constructors[0], constructors[construction])
    if direction == 1:
        text = text.replace("    int lastDirection = RMG_DIRECTION_COUNT;\n    int firstDirection = 0;",
                            "    int firstDirection = 0;\n    int lastDirection = RMG_DIRECTION_COUNT;")
    elif direction in (2, 3):
        start = text.index("    if (!g_adventureObjectLandBlocked[prototype->m_objectType][1]) {")
        end = text.index("        TPoint point;", start)
        selection = "    int direction" + (" = firstDirection;\n" if direction == 2 else ";\n")
        selection += ("    if (!g_adventureObjectLandBlocked[prototype->m_objectType][1]) {\n"
                      "        direction = 1;\n        lastDirection = 4;\n    }")
        if direction == 3:
            selection += " else {\n        direction = firstDirection;\n    }"
        selection += "\n    for (; direction < lastDirection; ++direction) {\n"
        text = text[:start] + selection + text[end:]
    point = ("        TPoint point;\n"
             "        point.m_x = entrance.m_x + g_rmgDirections[direction].m_x;\n"
             "        point.m_y = entrance.m_y + g_rmgDirections[direction].m_y;\n")
    queries = (point,
               "        TPoint point(entrance.m_x + g_rmgDirections[direction].m_x,\n"
               "            entrance.m_y + g_rmgDirections[direction].m_y);\n",
               "        TPoint point = g_rmgDirections[direction] + TRmgVector(entrance.m_x, entrance.m_y);\n")
    return text.replace(point, queries[query])


def forms():
    for construction, direction, query in itertools.product(range(5), range(4), range(3)):
        label = ("assigned", "copy_initialized", "direct_initialized", "const_reference", "named_copy")[construction]
        label += "+" + ("range_order", "first_before_last", "index_initialized", "index_arms")[direction]
        label += "+" + ("point_fields", "point_constructed", "direction_plus_vector")[query]
        yield label, body(construction, direction, query)


def refinements(text):
    policy = ("    if (group->m_hasGuard)\n        goto disallowEntrances;\n"
              "    allowEntrances = 1;\n    goto checkOutline;\n"
              "disallowEntrances:\n    allowEntrances = 0;\ncheckOutline:\n")
    if text.count(policy) != 1:
        raise ValueError("review the original shared outline-policy boundary")
    alternatives = (
        ("explicit_allow_arm", "    if (!group->m_hasGuard) {\n"
         "        allowEntrances = 1;\n        goto checkOutline;\n    }\n"
         "disallowEntrances:\n    allowEntrances = 0;\ncheckOutline:\n"),
        ("reject_arm_join", "    if (group->m_hasGuard) {\n"
         "disallowEntrances:\n        allowEntrances = 0;\n    } else {\n"
         "        allowEntrances = 1;\n    }\ncheckOutline:\n"),
        ("allow_else_join", "    if (!group->m_hasGuard) {\n"
         "        allowEntrances = 1;\n    } else {\n"
         "disallowEntrances:\n        allowEntrances = 0;\n    }\ncheckOutline:\n"),
    )
    for label, replacement in alternatives:
        yield label, text.replace(policy, replacement)
    guard = "    if (group->m_hasGuard) {\n"
    if text.count(guard) != 1:
        raise ValueError("review the guard-coordinate construction boundary")
    # A real coordinate snapshot after all opaque placement calls, not an
    # early cache across calls that may alter group state.
    for label, declaration in (
            ("guard_point_copy", "        TPoint localGuard = group->m_guardPosition;\n"),
            ("guard_point_reference", "        const TPoint& localGuard = group->m_guardPosition;\n")):
        copied = text.replace("group->m_guardPosition.m_x", "localGuard.m_x")
        copied = copied.replace("group->m_guardPosition.m_y", "localGuard.m_y")
        yield label, copied.replace(guard, guard + declaration)
    vector = "        TPoint point = g_rmgDirections[direction] + TRmgVector(entrance.m_x, entrance.m_y);\n"
    if vector in text:
        for label, replacement in (
                ("neighbor_assigned", "        TPoint point;\n"
                 "        point = g_rmgDirections[direction] + TRmgVector(entrance.m_x, entrance.m_y);\n"),
                ("neighbor_direct", "        TPoint point(g_rmgDirections[direction] + TRmgVector(entrance.m_x, entrance.m_y));\n"),
                ("neighbor_direction_copy", "        TPoint point = g_rmgDirections[direction];\n"
                 "        point += TRmgVector(entrance.m_x, entrance.m_y);\n")):
            yield label, text.replace(vector, replacement)


def policy_section(text):
    at = text.index("    int allowEntrances;\n")
    start = text.index("\n    }\n", at) + len("\n    }\n")
    end = text.index("    if (!m_map.hasConnectedOutline(", start)
    return text[start:end]


def lifetime_refinements(text):
    policies = [("policy_original", BASELINE), *list(refinements(BASELINE))[:3]]
    original = policy_section(text)
    if original not in {policy_section(body) for _, body in policies}:
        raise ValueError("review the selected outline-policy arm")
    for label, body in policies:
        policy = policy_section(body)
        if policy != original:
            yield label, text.replace(original, policy)
    naked = text
    for declaration in ("TPoint localGuard", "const TPoint& localGuard"):
        line = "        " + declaration + " = group->m_guardPosition;\n"
        if line in naked:
            naked = naked.replace(line, "").replace("localGuard.m_x", "group->m_guardPosition.m_x")
            naked = naked.replace("localGuard.m_y", "group->m_guardPosition.m_y")
    for label, declaration in (("guard_copy", "TPoint"), ("guard_const_copy", "const TPoint"),
                               ("guard_reference", "const TPoint&")):
        changed = naked.replace("group->m_guardPosition.m_x", "localGuard.m_x")
        changed = changed.replace("group->m_guardPosition.m_y", "localGuard.m_y")
        guard = "    if (group->m_hasGuard) {\n"
        # An outline-policy arm may have the same opener. Only the first
        # occurrence is the guard-neighborhood block.
        changed = changed.replace(guard, guard + "        " + declaration + " localGuard = group->m_guardPosition;\n", 1)
        if changed != text:
            yield label, changed
    start = text.index("        for (int x = guardPosition.m_x - 1;")
    end = text.index("    }\n    int ", start)
    scan = text[start:end]
    changed = scan.replace("for (int x = guardPosition.m_x - 1; x <= guardPosition.m_x + 1; ++x)",
        "for (point.m_x = guardPosition.m_x - 1; point.m_x <= guardPosition.m_x + 1; ++point.m_x)")
    changed = changed.replace("for (int y = guardPosition.m_y - 1; y <= guardPosition.m_y + 1; ++y)",
        "for (point.m_y = guardPosition.m_y - 1; point.m_y <= guardPosition.m_y + 1; ++point.m_y)")
    changed = changed.replace("getMapItem(x, y, guardPosition.m_z)",
                              "getMapItem(point.m_x, point.m_y, guardPosition.m_z)")
    yield "guard_scan_point", text[:start] + "        TPoint point;\n" + changed + text[end:]
    result = "        TRmgMapPosition objectPosition = object->getPosition();\n"
    if text.count(result) != 1:
        raise ValueError("review the initial object-position value boundary")
    yield "object_position_assigned", text.replace(result,
        "        TRmgMapPosition objectPosition;\n        objectPosition = object->getPosition();\n")


def frame_refinements(text):
    policies = [("policy_original", BASELINE), *list(refinements(BASELINE))[:3]]
    original = policy_section(text)
    for label, body in policies:
        policy = policy_section(body)
        if policy != original:
            yield label, text.replace(original, policy)
    guard = text.index("    if (group->m_hasGuard) {\n")
    start = text.index("        for (", guard)
    prefix = "        TPoint point;\n"
    if text[:start].endswith(prefix):
        start -= len(prefix)
    end = text.index("    }\n    int ", start)
    original_scan = text[start:end]
    scalar_start = BASELINE.index("        for (int x = guardPosition.m_x - 1;")
    scalar_end = BASELINE.index("    }\n    int ", scalar_start)
    scalar = BASELINE[scalar_start:scalar_end]
    point = scalar.replace("for (int x = guardPosition.m_x - 1; x <= guardPosition.m_x + 1; ++x)",
        "for (point.m_x = guardPosition.m_x - 1; point.m_x <= guardPosition.m_x + 1; ++point.m_x)")
    point = point.replace("for (int y = guardPosition.m_y - 1; y <= guardPosition.m_y + 1; ++y)",
        "for (point.m_y = guardPosition.m_y - 1; point.m_y <= guardPosition.m_y + 1; ++point.m_y)")
    point = point.replace("getMapItem(x, y, guardPosition.m_z)",
                          "getMapItem(point.m_x, point.m_y, guardPosition.m_z)")
    if original_scan not in (scalar, prefix + point):
        raise ValueError("review the current scalar or point-valued guard scan")
    position = point.replace("point.m_y, guardPosition.m_z", "point.m_y, point.m_z")
    copied = position.replace("for (point.m_x = guardPosition.m_x - 1;", "for (;")
    forms = (
        ("scan_point", prefix + point),
        ("scan_position_fields", "        TRmgMapPosition point;\n"
         "        point.m_z = guardPosition.m_z;\n" + position),
        ("scan_position_copy", "        TRmgMapPosition point = guardPosition;\n"
         "        --point.m_x;\n" + copied),
        ("scan_position_value_query", "        TRmgMapPosition point;\n"
         "        point.m_z = guardPosition.m_z;\n" + position.replace(
             "getMapItem(point.m_x, point.m_y, point.m_z)", "getMapItem(point)")),
        ("scan_position_copy_value_query", "        TRmgMapPosition point = guardPosition;\n"
         "        --point.m_x;\n" + copied.replace(
             "getMapItem(point.m_x, point.m_y, point.m_z)", "getMapItem(point)")),
    )
    for label, scan in forms:
        if scan != original_scan:
            yield label, text[:start] + scan + text[end:]
    before = "    TRmgZoneBounds bounds = group->m_bounds;\n"
    if text.count(before) != 1:
        raise ValueError("review the entry-time bounds snapshot")
    yield "bounds_assigned", text.replace(before,
        "    TRmgZoneBounds bounds;\n    bounds = group->m_bounds;\n")


def shared_refinements(text):
    for phases, name in ((5, "object_entrance"), (7, "all_positions"), (6, "guard_entrance"),
                         (3, "object_guard"), (1, "object_hoisted"), (4, "entrance_hoisted"), (2, "guard_hoisted")):
        changed = text
        if phases & 1:
            initialized = "        TRmgMapPosition objectPosition = object->getPosition();\n"
            assigned = "        TRmgMapPosition objectPosition;\n        objectPosition = object->getPosition();\n"
            if initialized in changed:
                changed = changed.replace(initialized, "        objectPosition = object->getPosition();\n")
            elif assigned in changed:
                changed = changed.replace(assigned, "        objectPosition = object->getPosition();\n")
            else:
                raise ValueError("review the object-position declaration before sharing")
            changed = re.sub(r"\bobjectPosition\b", "workingPosition", changed)
        if phases & 2:
            empty = "        TRmgMapPosition guardPosition;\n"
            if empty in changed:
                changed = changed.replace(empty, "")
            elif "        const TRmgMapPosition& guardPosition = " in changed:
                changed = changed.replace("        const TRmgMapPosition& guardPosition = ",
                                          "        guardPosition = ")
            elif "        TRmgMapPosition guardPosition = " in changed:
                changed = changed.replace("        TRmgMapPosition guardPosition = ", "        guardPosition = ")
            elif "        TRmgMapPosition guardPosition(" in changed:
                changed = changed.replace("        TRmgMapPosition guardPosition(",
                                          "        guardPosition = TRmgMapPosition(")
            else:
                raise ValueError("review the guard-position declaration before sharing")
            changed = re.sub(r"\bguardPosition\b", "workingPosition", changed)
        if phases & 4:
            entrance = "    TRmgMapPosition entrance = lastObject->getPosition();\n"
            if changed.count(entrance) != 1:
                raise ValueError("review the entrance-position declaration before sharing")
            changed = changed.replace(entrance, "    entrance = lastObject->getPosition();\n")
            changed = re.sub(r"\bentrance\b", "workingPosition", changed)
        for placement in ("after_zone", "before_bounds"):
            if placement == "after_zone":
                anchor = "    int zoneIndex = zone->m_slot->m_zoneIndex;\n"
                replacement = anchor + "    TRmgMapPosition workingPosition;\n"
            else:
                anchor = "{\n"
                replacement = anchor + "    TRmgMapPosition workingPosition;\n"
            yield name + "+" + placement, changed.replace(anchor, replacement, 1)


@functools.lru_cache(maxsize=1)
def admitted_bodies():
    result = set()
    for _, text in forms():
        result.add(text)
        refined = [candidate for _, candidate in refinements(text)]
        result.update(refined)
        for parent in [text, *refined]:
            result.update(candidate for _, candidate in lifetime_refinements(parent))
    for parent in list(result):
        result.update(candidate for _, candidate in frame_refinements(parent))
    return frozenset(result)


@functools.lru_cache(maxsize=None)
def is_admitted(text):
    if text in admitted_bodies():
        return True
    # Sharing is the final grammar layer. Search it lazily rather than
    # retaining thousands of unused, full-function string combinations.
    return any(text == candidate for parent in admitted_bodies()
               for _, candidate in shared_refinements(parent))


def make_axes(source):
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    options = list(forms())
    if not is_admitted(original):
        raise ValueError("review current group-placement operations before rebasing")
    if original not in dict(options).values():
        options = options[:-1]  # current-source control plus 59 original forms
    return [helper.axis("group_place_lifetimes", SOURCE, original, options)]


def load_parents(checkpoint_path, source, *, frontier=False, lifetime=False, frame=False):
    directory = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                    axes=make_axes(source), evidence=__doc__)
    if frame:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source, lifetime=True), frame=True),
                        evidence=FRAME_EVIDENCE, parent_checkpoint=str(prior.resolve()))
    elif lifetime:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source, frontier=True), lifetime=True),
                        evidence=LIFETIME_EVIDENCE, parent_checkpoint=str(prior.resolve()))
    elif frontier:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source)),
                        evidence=PARENT_EVIDENCE, parent_checkpoint=str(prior.resolve()))
    if payload != expected:
        raise ValueError("review the completed group-placement family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review parents against changed source: " + relative)
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
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


def make_parent_axes(source, parents, *, lifetime=False, frame=False, shared=False):
    if len(parents) != 10:
        raise ValueError("expected ten reproduced parents")
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    retained = [(label, helper.definition(text, FUNCTION)) for label, text in parents]
    if len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct parent sources")
    options = helper.axis("group_place_frontier", SOURCE, original, retained)["options"]
    seen = {option["replace"] for option in options}
    inputs = [("baseline", original), *retained]
    successors = []
    for index, (_, text) in enumerate(inputs):
        mutations = list((shared_refinements if shared else frame_refinements if frame
                          else lifetime_refinements if lifetime else refinements)(text))
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
                return [dict(name="group_place_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("frontier does not supply 60 distinct states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    parser.add_argument("--lifetime-parents-from", type=Path)
    parser.add_argument("--frame-parents-from", type=Path)
    parser.add_argument("--shared-parents-from", type=Path)
    args = parser.parse_args()
    if sum(bool(value) for value in (args.parents_from, args.lifetime_parents_from,
                                    args.frame_parents_from, args.shared_parents_from)) > 1:
        parser.error("select only one parent stage")
    source = (HOMM3_DIR / SOURCE).read_text()
    checkpoint = args.shared_parents_from or args.frame_parents_from or args.lifetime_parents_from or args.parents_from
    lifetime = bool(args.lifetime_parents_from)
    frame = bool(args.frame_parents_from)
    shared = bool(args.shared_parents_from)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=(make_parent_axes(source, load_parents(checkpoint, source, frontier=lifetime, lifetime=frame, frame=shared),
                                          lifetime=lifetime, frame=frame, shared=shared)
                         if checkpoint else make_axes(source)),
                   evidence=SHARED_EVIDENCE if shared else FRAME_EVIDENCE if frame else LIFETIME_EVIDENCE if lifetime
                            else PARENT_EVIDENCE if checkpoint else __doc__)
    if checkpoint:
        payload["parent_checkpoint"] = str(checkpoint.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure-group placement states ->", args.output)


if __name__ == "__main__":
    main()
