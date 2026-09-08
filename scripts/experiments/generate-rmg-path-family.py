#!/usr/bin/env python3
"""Generate connection-path scopes/bounds and prototype-selector forms.

Retail 0x5408e0 requires one initial retained map lookup, a predecessor
snapshot and a same-zone bounds walk. Its virtual placement can change the
decoration flag, so preserve the post-call query. Retail 0x546040 retains
the subtype/category/mask checks and chooses from a real candidate vector.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import textwrap

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source
from homm3.vc6.source_families import load_manifest


def definition(source, name):
    found = _source.find_definitions(source, name)
    if len(found) != 1:
        raise ValueError(f"expected one definition of {name}")
    item = found[0]
    return source[source.rfind("\n", 0, item.head) + 1:item.body_close + 1]


def axis(name, original, alternatives):
    options = [dict(name="baseline", replace=original)]
    seen = {original}
    for label, replacement in alternatives:
        if replacement not in seen:
            options.append(dict(name=label, replace=replacement))
            seen.add(replacement)
    return dict(name=name, find=original, options=options)


def path_axes(original):
    start = original.index("        if (item->m_connection.m_present) {")
    end = original.index("        TRmgMapPosition previous =", start)
    connection = original[start:end]
    split = connection.index("        }\n        if (!item->m_connection.m_present)")
    placement = connection[len("        if (item->m_connection.m_present) {\n"):split]
    cleanup = connection[split + len("        }\n"):]
    clear = ("            item->m_tileData.m_borderObject = 0;\n"
             "            item->m_tileData.m_subterraneanGate = 1;\n")
    nested_true = ("        if (item->m_connection.m_present) {\n" + placement
                   + textwrap.indent(cleanup, "    ") + "        } else {\n" + clear + "        }\n")
    nested_false = ("        if (!item->m_connection.m_present) {\n" + clear
                    + "        } else {\n" + placement + textwrap.indent(cleanup, "    ") + "        }\n")
    bounds_start = original.index("            TRmgZoneBounds bounds;")
    bounds_end = original.index("        }\n        position = previous;", bounds_start)
    bounds = original[bounds_start:bounds_end]
    path = Path(__file__).with_name("generate-rmg-border-flood-family.py")
    spec = importlib.util.spec_from_file_location("rmg_border_bounds", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    unindented = textwrap.indent(textwrap.dedent(bounds), "    ")
    bounds_options = []
    for storage, order in itertools.product(("scalars", "corners", "rectangle"), itertools.permutations(range(4))):
        replacement = module.border(unindented, storage, order)
        replacement = textwrap.indent(textwrap.dedent(replacement), "            ")
        bounds_options.append((storage + "+" + "".join(map(str, order)), replacement))
    previous = "        TRmgMapPosition previous = item->m_previousTile;"
    return [
        axis("path_connection_scope", connection, [("nested_present", nested_true), ("nested_absent", nested_false)]),
        axis("path_bounds", bounds, bounds_options),
        axis("path_predecessor", previous, [
            ("direct_copy", "        TRmgMapPosition previous(item->m_previousTile);"),
            ("assigned", "        TRmgMapPosition previous;\n        previous = item->m_previousTile;"),
        ]),
    ]


def selector_body(original, binding, predicate, insertion):
    start = original.index("    for (unsigned int index =")
    end = original.index("    if (!candidates.size())")
    head = original[:start]
    if binding == "member":
        vector = "m_objectPrototypes[objectType]"
    else:
        const = "const " if binding == "const_reference" else ""
        head += f"    {const}std::vector<TRmgObjectPropertiesRef*>& prototypes = m_objectPrototypes[objectType];\n"
        vector = "prototypes"
    lines = [f"    for (unsigned int index = 0; index < {vector}.size(); ++index) {{",
             f"        TRmgObjectPropertiesRef* properties = {vector}[index];",
             "        TObjectType* prototype = properties->m_prototype;"]
    checks = [
        "        if (prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_4",
        "            || prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_5) {",
        "            if (terrain == eTerrainWater)", "                continue;",
        "        } else if (!prototype->m_recommendedTerrainMask.test(terrain)) {",
        "            continue;", "        }",
    ]
    calls = {"push_back": "candidates.push_back(properties)",
             "insert_value": "candidates.insert(candidates.end(), properties)",
             "insert_count": "candidates.insert(candidates.end(), 1, properties)"}
    checks.append("        " + calls[insertion] + ";")
    if predicate == "nested":
        lines.append("        if (prototype->m_subtype == subtype) {")
        lines += ["    " + line for line in checks]
        lines.append("        }")
    else:
        lines += ["        if (prototype->m_subtype != subtype)", "            continue;"] + checks
    lines.append("    }")
    return head + "\n".join(lines) + "\n" + original[end:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    axes = path_axes(definition(source, "type_random_map_generator::openConnectionPath"))
    selector = definition(source, "type_random_map_generator::selectObjectPrototype")
    axes.append(axis("prototype_selector", selector,
        [("+".join(form), selector_body(selector, *form)) for form in itertools.product(
            ("member", "reference", "const_reference"), ("continue", "nested"),
            ("push_back", "insert_value", "insert_count"))]))
    payload = dict(schema=1, source="src/rmg.cpp", units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence="Retail 0x5408e0 queries connection presence again after virtual placement, but a path with no initial decoration goes directly to clearing the route flags. Preserve this distinction, the ordinary selector/map helpers, predecessor snapshot and same-zone clipped bounds. Retail 0x546040 differs only in range/terrain register allocation; compare a named prototype range, structured subtype admission and real public candidate-vector insertion calls. No new pragmas, helpers or synthetic operations.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
