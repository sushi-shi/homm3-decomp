"""Refine ordinary shipyard helpers through source-real scopes and values.

The 256-case boundary family leaves two useful ordinary-helper parents:
five of seven fences removed (89.6272%) and all seven removed (88.8817%).
Both expand naturally into moveHero. DC mark_shipyards 851/852 and 882/883,
and clear_shipyards 920/921, positively identify three continue scopes.
Keep the canonical helpers and resource early return, and cross those scopes
with actual point-value lifetimes and the boolean can-build result's type.
No code is pasted into a caller and no inline directive is introduced.
"""

import argparse
import importlib.util
import itertools
import json
from pathlib import Path


def replace(source, old, new):
    if source.count(old) != 1:
        raise ValueError("Review source anchor: " + old)
    return source.replace(old, new)


def continue_guard(source, guard, inverse):
    start = source.index(guard)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    indent = source[source.rfind("\n", 0, start) + 1:start]
    body = source[opening + 2:end - 1]
    if not body.endswith(indent):
        raise ValueError("Review guard indentation")
    body = body[:-len(indent)]
    body = "".join(line[4:] if line.startswith("    ") else line
                   for line in body.splitlines(keepends=True))
    return (source[:start] + inverse + "\n" + indent + "    continue;\n\n"
            + body.rstrip("\n") + source[end:])


def make_manifest():
    path = Path(__file__).with_name("generate-shipyard-boundary-family.py")
    spec = importlib.util.spec_from_file_location("shipyard_parent", path)
    parent = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parent)
    manifest = parent.make_manifest()
    axis = manifest["axes"][0]
    options = [dict(name="unchanged")]
    for parent_index, scopes, point, flag in itertools.product(
            (250, 255), range(8), ("copy", "reference", "temporary"),
            ("unsigned char", "bool")):
        source = axis["options"][parent_index]["replace"]
        if scopes & 1:
            source = continue_guard(source,
                "if (currentTown->m_dockSite != town::TOWN_DOCK_SITE_NONE) {",
                "if (currentTown->m_dockSite == town::TOWN_DOCK_SITE_NONE)")
        mark, clear = source.split("// E:\\gamedcs\\philai.cpp:896", 1)
        parts = [mark, clear]
        for i in range(2):
            if scopes & (2 << i):
                parts[i] = continue_guard(parts[i],
                    "if (info->m_boatX != ShipyardInfo::NO_BOAT) {",
                    "if (info->m_boatX == ShipyardInfo::NO_BOAT)")
            if point == "reference":
                parts[i] = replace(parts[i], "type_point shipyardPoint =",
                                   "const type_point& shipyardPoint =")
            elif point == "temporary":
                parts[i] = replace(parts[i],
                    "        type_point shipyardPoint = player->m_shipyards[shipyardIndex];\n", "")
                parts[i] = replace(parts[i], "g_game->getCell(shipyardPoint)",
                                   "g_game->getCell(player->m_shipyards[shipyardIndex])")
        source = parts[0] + "// E:\\gamedcs\\philai.cpp:896" + parts[1]
        source = replace(source, "unsigned char canBuildShip = 0;",
                         flag + " canBuildShip = 0;")
        options.append(dict(name=f"parent-{parent_index}-continues-{scopes:03b}-{point}-{flag}",
                            replace=source))
    axis["name"] = "ordinary-helper-scopes"
    axis["options"] = options
    manifest["evidence"] = __doc__
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
