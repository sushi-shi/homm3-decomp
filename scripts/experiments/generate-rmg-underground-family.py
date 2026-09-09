#!/usr/bin/env python3
"""Underground scan-coordinate and borrowed-view lifetimes (0x5439e0).

No Dreamcast counterpart is mapped. Retail and candidate agree on all 36
CFG blocks and ten calls. The first scan row lives at -0x28 in retail,
reused by the later level-position value; ours reuses the zone-index home
at -0x14. Initial borrowed-view scheduling and bounds-copy registers also
differ. Test real coordinate ownership, dimension values and zone/bounds
reads without changing the borrowed constructor, predicates, brush or RAII.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::decorateUnderground"


def baseline_definition(source):
    current = generator("generate-rmg-position-family.py").definition(source, NAME)
    if "    TRmgMapPosition scan;\n" not in current:
        return current
    body = current.replace("    TRmgMapPosition scan;\n    scan.m_z = 1;\n", "")
    body = body.replace("m_map.getMapItem(0, 0, scan.m_z)", "m_map.getMapItem(0, 0, 1)")
    body = body.replace("    TPoint dimensions(m_map.m_mapWidth, m_map.m_mapHeight);\n", "")
    body = body.replace("    type_random_map map(item, dimensions.m_x, dimensions.m_y);",
                        "    type_random_map map(item,\n        m_map.m_mapWidth, m_map.m_mapHeight);")
    body = body.replace("for (scan.m_y = 0; scan.m_y < m_map.m_mapHeight; ++scan.m_y)",
                        "for (int y = 0; y < m_map.m_mapHeight; ++y)")
    body = body.replace("for (scan.m_x = 0; scan.m_x < m_map.m_mapWidth; ++scan.m_x, ++item)",
                        "for (int x = 0; x < m_map.m_mapWidth; ++x, ++item)")
    body = body.replace("paintRectangle(scan.m_x, scan.m_y,", "paintRectangle(x, y,")
    body = body.replace("        scan = m_zones[zone]->getLevelPosition();",
                        "        TRmgMapPosition position = m_zones[zone]->getLevelPosition();")
    body = body.replace("if (scan.m_z != 1)", "if (position.m_z != 1)")
    body = body.replace("        TRmgZoneBounds bounds = m_zones[zone]->m_bounds;\n        int terrain = m_zones[zone]->m_terrain;",
                        "        int terrain = m_zones[zone]->m_terrain;\n        TRmgZoneBounds bounds = m_zones[zone]->m_bounds;")
    if zone_reads(dimensions(coordinates(body, 4), 3), 1) != current:
        raise ValueError("review changed underground implementation before rebasing")
    return body


def coordinates(original, form):
    if not form:
        return original
    first = original.index("    for (int y = 0;")
    end = original.index("    if (m_progress)", first)
    scan = original[first:end]
    scan = scan.replace("int y =", "scan.m_y =").replace("int x =", "scan.m_x =")
    scan = scan.replace("y <", "scan.m_y <").replace("x <", "scan.m_x <")
    scan = scan.replace("++y", "++scan.m_y").replace("++x", "++scan.m_x")
    scan = scan.replace("paintRectangle(x, y,", "paintRectangle(scan.m_x, scan.m_y,")
    if form in (1, 2, 3):
        declaration = "    TPoint scan;\n"
    else:
        declaration = "    TRmgMapPosition scan;\n"
    body = original[:first] + declaration + scan + original[end:]
    if form in (2, 3):
        start = body.index("        for (int y = bounds.m_minimumY;")
        end = body.index("\n    }\n    if (m_progress)", start)
        scan = body[start:end]
        scan = scan.replace("int y =", "scan.m_y =").replace("int x =", "scan.m_x =")
        scan = scan.replace("y <", "scan.m_y <").replace("x <", "scan.m_x <")
        scan = scan.replace("++y", "++scan.m_y").replace("++x", "++scan.m_x")
        scan = scan.replace("getMapItem(x, y,", "getMapItem(scan.m_x, scan.m_y,")
        scan = scan.replace("paintRectangle(x, y,", "paintRectangle(scan.m_x, scan.m_y,")
        if form == 3:
            scan = "        TPoint scan;\n" + scan
        body = body[:start] + scan + body[end:]
    if form == 4:
        # Carry the real level in the same coordinate used for the first
        # scan. The returned zone position overwrites it after that lifetime.
        body = body.replace("    TRmgMapPosition scan;\n", "    TRmgMapPosition scan;\n    scan.m_z = 1;\n")
        body = body.replace("        TRmgMapPosition position = m_zones[zone]->getLevelPosition();",
                            "        scan = m_zones[zone]->getLevelPosition();")
        body = body.replace("position.m_z != 1", "scan.m_z != 1")
        # The first scan consumes x/y only; initialization of z is meaningful
        # only when it supplies the initial plane query, not as dead padding.
        body = body.replace("    TRmgMapItem* item = m_map.getMapItem(0, 0, 1);",
                            "    TRmgMapPosition scan;\n    scan.m_z = 1;\n    TRmgMapItem* item = m_map.getMapItem(0, 0, scan.m_z);")
        duplicate = "    TRmgMapPosition scan;\n    scan.m_z = 1;\n"
        second = body.index(duplicate, body.index(duplicate) + len(duplicate))
        body = body[:second] + body[second + len(duplicate):]
    return body


def dimensions(body, form):
    if not form:
        return body
    old = "    type_random_map map(item,\n        m_map.m_mapWidth, m_map.m_mapHeight);"
    if form == 1:
        replacement = "    int width = m_map.m_mapWidth;\n    int height = m_map.m_mapHeight;\n    type_random_map map(item, width, height);"
    elif form == 2:
        replacement = "    int height = m_map.m_mapHeight;\n    int width = m_map.m_mapWidth;\n    type_random_map map(item, width, height);"
    else:
        replacement = "    TPoint dimensions(m_map.m_mapWidth, m_map.m_mapHeight);\n    type_random_map map(item, dimensions.m_x, dimensions.m_y);"
    if body.count(old) != 1:
        raise ValueError("changed underground borrowed-map constructor")
    return body.replace(old, replacement)


def zone_reads(body, form):
    if not form:
        return body
    old = "        int terrain = m_zones[zone]->m_terrain;\n        TRmgZoneBounds bounds = m_zones[zone]->m_bounds;"
    if form == 1:
        replacement = "        TRmgZoneBounds bounds = m_zones[zone]->m_bounds;\n        int terrain = m_zones[zone]->m_terrain;"
        return body.replace(old, replacement)
    body = body.replace("    for (unsigned int zone = 0; zone < m_zones.size(); ++zone) {",
                        "    for (unsigned int zone = 0; zone < m_zones.size(); ++zone) {\n        TRmgZone* current = m_zones[zone];")
    return body.replace("m_zones[zone]->", "current->")


def variants(original):
    for scan, size, zone in itertools.product(range(5), range(4), range(3)):
        yield "scan_%d+dimensions_%d+zone_%d" % (scan, size, zone), zone_reads(dimensions(coordinates(original, scan), size), zone)


def origin_variant(parent, form):
    split = parent.index("    TRmgTerrainBrush brush(")
    head, tail = parent[:split], parent[split:]
    line = next(line for line in head.splitlines() if "TRmgMapItem* item =" in line)
    query = line.split(" = ", 1)[1].removesuffix(";")
    if form == 0:
        opening = head.index("{\n") + 2
        head = head[:opening] + "    type_random_map& sourceMap = m_map;\n" + head[opening:].replace("m_map.", "sourceMap.")
    elif form == 1:
        level = "scan.m_z" if "scan.m_z" in query else "1"
        head = head.replace(line, "    TRmgMapPosition origin;\n    origin.m_x = 0;\n    origin.m_y = 0;\n"
                            + "    origin.m_z = " + level + ";\n    TRmgMapItem* item = m_map.getMapItem(origin);")
    elif form == 2:
        head = head.replace(line, "    TRmgMapItem* const& first = " + query + ";\n    TRmgMapItem* item = first;")
        head = head.replace("type_random_map map(item,", "type_random_map map(first,")
    elif form == 3:
        head = head.replace(line, "    TRmgMapItem* first = " + query + ";\n    TRmgMapItem* item = first;")
        head = head.replace("type_random_map map(item,", "type_random_map map(first,")
    elif form == 4:
        declarations = [line for line in head.splitlines() if line.startswith(("    int width =", "    int height =", "    TPoint dimensions("))]
        if not declarations:
            declarations = ["    int width = m_map.m_mapWidth;", "    int height = m_map.m_mapHeight;"]
            head = head.replace("type_random_map map(item,\n        m_map.m_mapWidth, m_map.m_mapHeight);", "type_random_map map(item, width, height);")
        else:
            for declaration in declarations:
                head = head.replace(declaration + "\n", "")
        head = head.replace(line, "\n".join(declarations) + "\n" + line)
    else:
        level = "scan.m_z" if "scan.m_z" in query else "1"
        head = head.replace(line, "    TRmgMapPosition origin(0, 0, " + level + ");\n    TRmgMapItem* item = m_map.getMapItem(origin);")
    return head + tail


def origin_frontier(source, checkpoint_path):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) != 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished ten-parent underground population")
    payload, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
    if len(checkpoint["records"]) != 60 or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("underground population is not fully scored")
    for folder in ("src", "include"):
        frozen, live = context / "snapshot" / folder, HOMM3_DIR / folder
        paths = [p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()]
        live_paths = [p.relative_to(live) for p in live.rglob("*")
                      if p.is_file() and "build" not in p.relative_to(live).parts]
        if set(paths) != set(live_paths) or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed underground snapshot: " + folder)
    definition = generator("generate-rmg-position-family.py").definition
    current = definition(source, NAME)
    forms, seen, parents = [("unchanged", current)], {current}, []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("underground parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced underground source")
        parent = definition(rendered["src/rmg.cpp"], NAME)
        parents.append((elite["id"], parent))
        if parent not in seen:
            seen.add(parent)
            forms.append((elite["id"] + "+parent", parent))
    for form in range(6):
        for identity, parent in parents:
            body = origin_variant(parent, form)
            if body in seen:
                continue
            seen.add(body)
            forms.append((identity + "+origin_%d" % form, body))
            if len(forms) == 60:
                return forms
    return forms


def constructor_family(source, checkpoint_path, controls=False):
    # Reuse the same complete parent-snapshot and repetition audit, but keep
    # only its ten actual reproduced parents, not uncompiled origin children.
    parents = [(name, body) for name, body in origin_frontier(source, checkpoint_path)
               if name.endswith("+parent")]
    if len(parents) != 10:
        raise ValueError("constructor family needs every reproduced parent")
    helper = generator("generate-rmg-position-family.py")
    header = (HOMM3_DIR / "include/rmg.h").read_text()
    old = helper.definition(header, "type_random_map", parameters="TRmgMapItem* items, int width, int height")
    statements = ("        m_mapWidth = width;", "        m_mapHeight = height;", "        m_mapItems = items;")
    anchors = ["\n".join(statements[i] for i in order) for order in itertools.permutations(range(3))]
    matched = [anchor for anchor in anchors if old.count(anchor) == 1]
    if len(matched) != 1:
        raise ValueError("review changed canonical borrowed constructor")
    anchor = matched[0]
    original = helper.definition(source, NAME)
    options = [dict(name="unchanged", replace=original)]
    for order in itertools.permutations(range(3)):
        constructor = old.replace(anchor, "\n".join(statements[i] for i in order))
        for name, body in parents:
            option = dict(name=name + "+constructor_" + "".join(map(str, order)), replace=body)
            if constructor != old:
                option["extra_edits"] = [dict(source="include/rmg.h", find=old, replace=constructor)]
            options.append(option)
    if controls:
        selected = next(option for option in options if option["name"] == parents[0][0] + "+constructor_201")
        if "extra_edits" not in selected:
            raise ValueError("constructor already adopted; these historical causal controls no longer apply")
        caller = dict(name="caller_only", replace=selected["replace"])
        header = dict(name="constructor_only", replace=original, extra_edits=selected["extra_edits"])
        combined = dict(selected, name="caller_and_constructor")
        options = [options[0], caller, header, combined]
    return dict(schema=1, units=generator("generate-rmg-map-accessor-family.py").UNITS,
                evidence=__doc__ + "\nBorrowed-constructor field stores are compared across all seven header consumers; the exact water-border caller is a required control.",
                axes=[dict(name="underground_constructor", source="src/rmg.cpp", find=original, options=options)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--origins-from", type=Path)
    parser.add_argument("--constructor-from", type=Path)
    parser.add_argument("--constructor-controls", action="store_true")
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = helper.definition(source, NAME)
    forms = origin_frontier(source, args.origins_from) if args.origins_from else variants(baseline_definition(source))
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("underground_lifetimes", "src/rmg.cpp", original, forms)])
    if args.constructor_from:
        payload = constructor_family(source, args.constructor_from, args.constructor_controls)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "underground lifetime states")


if __name__ == "__main__":
    main()
