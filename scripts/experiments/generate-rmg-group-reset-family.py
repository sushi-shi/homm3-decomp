#!/usr/bin/env python3
"""Generate 60 real treasure-group reset states, retaining all helper boundaries.

Retail 0x535040 expands the ordinary map clear and two-coordinate lookup,
but retains both vector _Destroy calls. Its copies in placeZoneTreasures
0x547360 retain different public vector wrappers and call getMapItem(int,int)
at 0x547625/0x547717. Restore that canonical overload and its real rmg.cpp
body before this sweep; neither a false inline keyword nor a second helper
declaration is an axis. Cross public vector emptying with actual surface-walk
lifetimes/control forms and measure all three TUs, including those callers.
"""
import argparse
import functools
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "TRmgTreasureGroup::reset"
FRONTIER_EVIDENCE = (
    "The first 60 states reproduce 80.8902% reset: the object clear now "
    "retains _Destroy, but the outline erase still expands it and leaves "
    "an EBP spill absent from retail. Both retail vectors are addressed "
    "through their own receiver. Preserve ten reproduced source parents "
    "and test actual vector-reference and begin/end iterator lifetimes "
    "without changing ownership, helper declarations or the map phases. "
    "The reset, assembly and scheduler peaks occur in different parents; "
    "all three TUs remain part of every score vector.")
LIFETIME_EVIDENCE = (
    "The second 60 states reproduce the same 80.8902% reset peak and "
    "improve assembly/scheduler combinations, but one outline _Destroy "
    "still expands. Retail keeps one byte-zero value for both flags and "
    "walks a cached surface size from a two-coordinate origin. Test those "
    "actual values, dimension-read order and the real cursor increment; "
    "do not change the reset phases, manufacture unused work, alter the "
    "canonical helpers or introduce an inline control.")
BASELINE = """void TRmgTreasureGroup::reset()
{
    m_objects.erase(m_objects.begin(), m_objects.end());
    m_outline.erase(m_outline.begin(), m_outline.end());
    m_map.clear();
    m_hasGuard = 0;
    m_ready = 0;
    TRmgMapItem* item = m_map.getMapItem(0, 0);
    int count = m_map.m_mapWidth * m_map.m_mapHeight;
    while (count--) {
        item->setTerrain(eTerrainDirt, 0, 0, 0);
        ++item;
    }
}"""
EMPTY = (("erase", "{name}.erase({name}.begin(), {name}.end());"),
         ("clear", "{name}.clear();"),
         ("resize", "{name}.resize(0);"))
WALK = """    TRmgMapItem* item = m_map.getMapItem(0, 0);
    int count = m_map.m_mapWidth * m_map.m_mapHeight;
    while (count--) {
        item->setTerrain(eTerrainDirt, 0, 0, 0);
        ++item;
    }
"""


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_group_reset_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def walks():
    yield "item_then_count", WALK
    yield "count_then_item", WALK.replace(
        "    TRmgMapItem* item = m_map.getMapItem(0, 0);\n"
        "    int count = m_map.m_mapWidth * m_map.m_mapHeight;\n",
        "    int count = m_map.m_mapWidth * m_map.m_mapHeight;\n"
        "    TRmgMapItem* item = m_map.getMapItem(0, 0);\n")
    yield "assigned_item", WALK.replace(
        "    TRmgMapItem* item = m_map.getMapItem(0, 0);\n",
        "    TRmgMapItem* item;\n    item = m_map.getMapItem(0, 0);\n")
    yield "for_postdecrement", (
        "    TRmgMapItem* item = m_map.getMapItem(0, 0);\n"
        "    for (int count = m_map.m_mapWidth * m_map.m_mapHeight; count--; ++item)\n"
        "        item->setTerrain(eTerrainDirt, 0, 0, 0);\n")
    yield "positive_for", (
        "    TRmgMapItem* item = m_map.getMapItem(0, 0);\n"
        "    for (int count = m_map.m_mapWidth * m_map.m_mapHeight; count > 0; --count) {\n"
        "        item->setTerrain(eTerrainDirt, 0, 0, 0);\n        ++item;\n    }\n")
    yield "guarded_do", (
        "    TRmgMapItem* item = m_map.getMapItem(0, 0);\n"
        "    int count = m_map.m_mapWidth * m_map.m_mapHeight;\n"
        "    if (count > 0) {\n        do {\n"
        "            item->setTerrain(eTerrainDirt, 0, 0, 0);\n            ++item;\n"
        "        } while (--count != 0);\n    }\n")
    yield "map_reference", (
        "    type_random_map& map = m_map;\n"
        + WALK.replace("m_map.", "map."))


def forms():
    # Walk-major ordering covers every public API pair before refinement;
    # the initial 60 selected states contain six or seven walks for each pair.
    for (walk_name, walk), (objects_name, objects), (outline_name, outline) in itertools.product(
            walks(), EMPTY, EMPTY):
        body = BASELINE.replace(WALK, walk)
        for name, form in (("m_objects", objects), ("m_outline", outline)):
            body = body.replace(EMPTY[0][1].format(name=name), form.format(name=name))
        yield walk_name + "+objects_" + objects_name + "+outline_" + outline_name, body


def refinements(text):
    found = {}
    for name in ("m_objects", "m_outline"):
        matches = [form.format(name=name) for _, form in EMPTY if form.format(name=name) in text]
        if len(matches) != 1:
            raise ValueError("review the current public emptying operation: " + name)
        found[name] = matches[0]
    for name, element, local in (("m_objects", "type_object*", "objects"),
                                  ("m_outline", "TPoint", "outline")):
        original = "    " + found[name] + "\n"
        reference = "    std::vector<" + element + ">& " + local + " = " + name + ";\n"
        yield local + "_reference", text.replace(original, reference + "    " + found[name].replace(name, local) + "\n")
    for upfront in (False, True):
        result, declarations = text, ""
        for name, element, local in (("m_objects", "type_object*", "objects"),
                                      ("m_outline", "TPoint", "outline")):
            declaration = "    std::vector<" + element + ">& " + local + " = " + name + ";\n"
            declarations += declaration
            result = result.replace("    " + found[name] + "\n",
                                    ("" if upfront else declaration) + "    " + found[name].replace(name, local) + "\n")
        if upfront:
            result = result.replace("\n{\n", "\n{\n" + declarations, 1)
        yield "both_references_" + ("entry" if upfront else "local"), result
    for name, element, prefix in (("m_outline", "TPoint", "outline"),
                                  ("m_objects", "type_object*", "objects")):
        original = "    " + found[name] + "\n"
        first = "    std::vector<" + element + ">::iterator " + prefix + "First = " + name + ".begin();\n"
        last = "    std::vector<" + element + ">::iterator " + prefix + "Last = " + name + ".end();\n"
        erase = "    " + name + ".erase(" + prefix + "First, " + prefix + "Last);\n"
        for label, replacement in (("first_last", first + last + erase),
                                   ("last_first", last + first + erase),
                                   ("last", last + "    " + name + ".erase(" + name + ".begin(), " + prefix + "Last);\n")):
            yield prefix + "_" + label, text.replace(original, replacement)


def lifetime_refinements(text):
    flags = "    m_hasGuard = 0;\n    m_ready = 0;\n"
    if text.count(flags) != 1:
        raise ValueError("review the two reset flag assignments")
    yield "chained_flags", text.replace(flags, "    m_ready = m_hasGuard = 0;\n")
    yield "byte_zero_flags", text.replace(flags,
        "    unsigned char cleared = 0;\n    m_hasGuard = cleared;\n    m_ready = cleared;\n")
    lookup = re.search(r"    (?:TRmgMapItem\* item = |item = )(m_map|map)\.getMapItem\(0, 0\);\n", text)
    dimensions = re.search(r"(m_map|map)\.m_mapWidth \* \1\.m_mapHeight", text)
    if lookup is None or dimensions is None:
        raise ValueError("review the surface lookup and cached dimensions")
    receiver = lookup.group(1)
    for label, initializer in (("origin_direct", "    TPoint origin(0, 0);\n"),
                               ("origin_fields", "    TPoint origin;\n    origin.m_x = 0;\n    origin.m_y = 0;\n")):
        yield label, text.replace(lookup.group(), initializer + lookup.group().replace(
            ".getMapItem(0, 0)", ".getMapItem(origin.m_x, origin.m_y)"))
    for reverse in (False, True):
        reads = ("    int width = " + receiver + ".m_mapWidth;\n",
                 "    int height = " + receiver + ".m_mapHeight;\n")
        result = text.replace(dimensions.group(), "width * height")
        # Some retained parents snapshot count before the pure lookup. Keep
        # that placement, and declare both dimensions before their first use.
        line = result.rfind("\n", 0, result.index("width * height")) + 1
        yield "dimension_copies_" + ("yx" if reverse else "xy"), (
            result[:line] + "".join(reversed(reads) if reverse else reads) + result[line:])
    yield "dimension_product_yx", text.replace(dimensions.group(),
        receiver + ".m_mapHeight * " + receiver + ".m_mapWidth")
    # Bind the existing receiver across both phases, not a duplicate map.
    mapped = text.replace("    type_random_map& map = m_map;\n", "")
    mapped = re.sub(r"\bmap\.", "m_map.", mapped).replace("m_map.", "map.")
    mapped = mapped.replace("    map.clear();\n", "    type_random_map& map = m_map;\n    map.clear();\n")
    yield "shared_map_reference", mapped
    setter = "item->setTerrain(eTerrainDirt, 0, 0, 0);"
    if text.count(setter) != 1:
        raise ValueError("review the one surface terrain setter")
    cursor = text.replace(setter, "item++->setTerrain(eTerrainDirt, 0, 0, 0);")
    if "; count--; ++item)" in cursor:
        cursor = cursor.replace("; count--; ++item)", "; count--;)")
    else:
        cursor, removed = re.subn(r"^\s*\+\+item;\n", "", cursor, count=1, flags=re.MULTILINE)
        if removed != 1:
            raise ValueError("review the existing surface cursor increment")
    yield "postincrement_receiver", cursor


@functools.lru_cache(maxsize=1)
def admitted_bodies():
    first = {body for _, body in forms()}
    second = first | {body for parent in first for _, body in refinements(parent)}
    return second | {body for parent in second for _, body in lifetime_refinements(parent)}


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in admitted_bodies():
        raise ValueError("review the current reset helper before generating")
    helpers().definition(source, "type_random_map::getMapItem", parameters="int x, int y")
    result = helpers().axis("group_reset", SOURCE, original, alternatives)
    result["options"] = result["options"][:60]
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct reset source states")
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
        raise ValueError("review the completed reset family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review reset parents against changed source: " + relative)
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {
            p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review reset parents against changed header population")
    for path in saved.rglob("*"):
        if path.is_file() and path.read_bytes() != (current / path.relative_to(saved)).read_bytes():
            raise ValueError("review reset parents against changed header: " + str(path.relative_to(saved)))
    if len(checkpoint["records"]) != 60 or any(not row.get("scores") for row in checkpoint["records"]) or len(checkpoint["elites"]) != 10:
        raise ValueError("expected all 60 scored states and ten reproduced elites")
    parents = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("reset parent did not reproduce: " + row["id"])
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("reset parent source hash changed: " + row["id"])
        parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents, *, lifetime=False):
    if len(parents) != 10:
        raise ValueError("expected ten reproduced reset parents")
    original = helpers().definition(source, FUNCTION)
    retained = [(label, helpers().definition(text, FUNCTION)) for label, text in parents]
    if len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct reset parent sources")
    options = helpers().axis("group_reset_frontier", SOURCE, original, retained)["options"]
    seen = {option["replace"] for option in options}
    inputs = [("baseline", original), *retained]
    successors = []
    for index, (_, text) in enumerate(inputs):
        mutations = list((lifetime_refinements if lifetime else refinements)(text))
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
                return [dict(name="group_reset_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("reset frontier does not supply 60 distinct states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    parser.add_argument("--lifetime-parents-from", type=Path)
    args = parser.parse_args()
    if args.parents_from and args.lifetime_parents_from:
        parser.error("select only one parent stage")
    source = (HOMM3_DIR / SOURCE).read_text()
    checkpoint = args.lifetime_parents_from or args.parents_from
    lifetime = bool(args.lifetime_parents_from)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=make_parent_axes(source, load_parents(checkpoint, source, frontier=lifetime), lifetime=lifetime)
                        if checkpoint else make_axes(source),
                   evidence=LIFETIME_EVIDENCE if lifetime else FRONTIER_EVIDENCE if checkpoint else __doc__)
    if checkpoint:
        payload["parent_checkpoint"] = str(checkpoint.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure-group reset states ->", args.output)


if __name__ == "__main__":
    main()
