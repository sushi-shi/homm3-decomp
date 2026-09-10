#!/usr/bin/env python3
"""Header-count ownership and natural calls after recovering all lobby handlers.

The preceding 64-state helper family must be exhausted and the fully recovered
corner adopted verbatim. Retail uses the selection vector's count twice in
the header-end arm, expanding the first size and retaining the second; DC
7104/7106 read the older scalar count. GetMapCount has no DC counterpart, so
test both existing wrapper and direct vector API at each of those two reads.
DC 7097 and retail stores put currentIndex before currentMap. Independently
remove existing constructor/network/manager overrides, never add a new pin.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

CPP = "src/singleselectionwindow.cpp"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    checkpoint = json.loads(args.parent.read_text())
    if len(checkpoint["records"]) != 64:
        raise ValueError("Exhaust the 64-state handler parent first")
    selected = next(row for row in checkpoint["elites"] if row["choices"] == [1]*6)
    tree = args.parent.parent / "candidates" / selected["id"] / "repeat/tree"
    for relative in (CPP, "include/singleselectionwindow.h"):
        if (HOMM3_DIR / relative).read_bytes() != (tree / relative).read_bytes():
            raise ValueError("Authored source differs from reproduced parent: " + relative)
    source = (HOMM3_DIR / CPP).read_text()
    count = """    m_fileSlider->setResolution(getMapCount() - g_unnamed69fdc8 + 1);
    if (m_selectionHeaders.size() > 0)"""
    count_options = [dict(name="wrapper_then_vector_control")]
    for name, first, second in (
        ("vector_then_vector", "m_selectionHeaders.size()", "m_selectionHeaders.size()"),
        ("wrapper_then_wrapper", "getMapCount()", "getMapCount()"),
        ("vector_then_wrapper", "m_selectionHeaders.size()", "getMapCount()"),
    ):
        count_options.append(dict(name=name, replace=(
            "    m_fileSlider->setResolution(" + first + " - g_unnamed69fdc8 + 1);\n"
            "    if (" + second + " > 0)")))

    start = source.index("// auto_inline(off): retail CALLS this from HandleNetMsg's request arm;")
    end = source.index("#pragma auto_inline(on)", start) + len("#pragma auto_inline(on)")
    override = source[start:end]
    ordinary = override[override.index("// E:\\gamedcs\\singleselectionwindow.cpp:1492"):]
    ordinary = ordinary.replace("\n#pragma auto_inline(on)", "")

    start = source.index("bool TSingleSelectionWindow::handleNetMsg(")
    end = source.index("\n}\n", start) + 2
    handler = source[start:end]
    constructor = """#pragma inline_depth(0)
                t_map_list_update* proc = new t_map_list_update(dpid);
#pragma inline_depth()"""
    no_constructor = handler.replace(constructor, "                t_map_list_update* proc = new t_map_list_update(dpid);")
    if no_constructor == handler or handler.count("#pragma inline_depth(0)") != 4:
        raise ValueError("Expected one constructor and three network-handler pins")
    unpinned = handler.replace("#pragma inline_depth(0)\n", "").replace("#pragma inline_depth()\n", "")
    network_only = unpinned.replace("                t_map_list_update* proc = new t_map_list_update(dpid);", constructor)
    manifest = dict(schema=1, source=CPP, units=["singleselectionwindow"],
        parent_context=args.parent.parent.name, parent_object=selected["object_hash"],
        evidence=__doc__, axes=[
            dict(name="header_count_owners", find=count, options=count_options),
            dict(name="header_zero_order", find="    m_currentIndex = m_currentMap = 0;", options=[
                dict(name="map_first_control"),
                dict(name="index_first", replace="    m_currentMap = m_currentIndex = 0;")]),
            dict(name="manager_override", find=override, options=[
                dict(name="existing_override_control"),
                dict(name="ordinary_manager", replace=ordinary)]),
            dict(name="dispatcher_overrides", find=handler, options=[
                dict(name="four_existing_overrides_control"),
                dict(name="natural_constructor", replace=no_constructor),
                dict(name="natural_network_handlers", replace=network_only),
                dict(name="all_natural", replace=unpinned)]),
        ])
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"{args.output}: 64 source states")


if __name__ == "__main__":
    main()
