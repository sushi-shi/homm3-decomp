#!/usr/bin/env python3
"""Check dimension-value consumers without losing the retained list allocator.

The dimension-member parent preserves both exact owned-map and base-generator
constructors, but expands every list<TPoint>::_Buynode call in path carving.
Retail retains that helper. Test real dimension snapshots/references at the
function or per-level lifetime, preserving the vector/list ownership scopes,
all queue operations, random draws and map helpers. Parent snapshots and their
reproduced hashes are checked before constructing this follow-up. Keep the
current independent-field source and unchanged dimension parent as controls.
"""
import argparse
import hashlib
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    directory = args.checkpoint.parent
    checkpoint = json.loads(args.checkpoint.read_text())
    parents = [row for row in checkpoint["elites"]
               if row["labels"].get("map_dimensions") == "dimension_value+owned_fields+view_fields"]
    assert len(parents) == 1
    parent = parents[0]
    candidate = directory / "candidates" / parent["id"]
    repeated = json.loads((candidate / "repeat/result.json").read_text())
    for key in ("object_hash", "scores", "source_hashes", "choices"):
        assert parent[key] == repeated[key], key
    originals, adopted = {}, {}
    for name in ("src/rmg.cpp", "include/rmg.h"):
        originals[name] = (HOMM3_DIR / name).read_text()
        assert originals[name] == (directory / "snapshot" / name).read_text(), name
        adopted[name] = (candidate / "first/tree" / name).read_text()
        assert hashlib.sha256(adopted[name].encode()).hexdigest() == parent["source_hashes"][name], name
    source = adopted["src/rmg.cpp"]
    signature = "void type_random_map_generator::carveBranchingPaths()"
    start = source.index(signature)
    original = source[start:source.index("\n}", start) + 2]
    variants = [("parent_direct", original)]
    for scoped in (False, True):
        for binding in ("reference", "value", "scalars"):
            marker = ("    for (int level = 0; level < m_map.m_size.m_z; ++level) {\n"
                      if scoped else signature + "\n{\n")
            first = original.index(marker) + len(marker)
            last = original.index("    item = m_map.m_mapItems;\n", first) if scoped else len(original)
            region = original[first:last]
            indent = "        " if scoped else "    "
            if binding == "scalars":
                declarations = []
                for component, name in (("x", "width"), ("y", "height"), ("z", "levels")):
                    old = "m_map.m_size.m_" + component
                    if old in region:
                        declarations.append(indent + "const int " + name + " = " + old + ";\n")
                        region = region.replace(old, name)
                declaration = "".join(declarations)
            else:
                declaration = indent + "const TRmgMapPosition" + ("&" if binding == "reference" else "") + " dimensions = m_map.m_size;\n"
                region = region.replace("m_map.m_size", "dimensions")
            variants.append((("level_" if scoped else "function_") + binding,
                             original[:first] + declaration + region + original[last:]))
    options = [{"name": "direct_dimensions_control", "replace": originals["src/rmg.cpp"]}]
    for label, body in variants:
        options.append({"name": label, "replace": source.replace(original, body), "extra_edits": [
            {"source": "include/rmg.h", "find": originals["include/rmg.h"], "replace": adopted["include/rmg.h"]}]})
    payload = {"schema": 1, "source": "src/rmg.cpp", "evidence": __doc__,
               "parent_checkpoint": str(args.checkpoint.resolve()),
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles", "singleselectionpopups",
                         "singleselectionwindow", "scenarioinfo"],
               "axes": [{"name": "dimension_uses", "find": originals["src/rmg.cpp"], "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("8 dimension ownership/use controls from reproduced parent " + parent["id"])


if __name__ == "__main__":
    main()
