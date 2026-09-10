"""Recheck every existing dispatcher pin after restoring its receive helper.

The parent must be the completed lobby-map-header family, and the authored
source/header must exactly equal its reproduced ordinary-receiver/pinned-
HeaderRequested candidate. Preserve that evidenced helper in every child.
Try each remaining HandleNetMsg depth-zero region separately, plus all at
once, crossed with the existing HeaderRequested auto-inline override.
This retains both complete-receiver parent choices while excluding the
semantically incomplete old receive arm. No new override is introduced.

These bounded controls test the changed inliner context; an unsuccessful
single-region removal does not prove the region necessary in other source
models or arbitrary combinations. All functions in the edited TU are scored.
"""

import argparse
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR


SOURCE = "src/singleselectionwindow.cpp"


def make_manifest(parent):
    checkpoint = json.loads(parent.read_text())
    input_payload = json.loads((parent.parent / "input.json").read_text())
    axes = [axis["name"] for axis in input_payload["axes"]]
    if axes != ["map-header-receive-owner", "header-requested-override"]:
        raise ValueError("Expected the lobby-map-header parent family")
    retained = {tuple(row["choices"]): row for row in checkpoint["elites"]}
    if not {(1, 0), (1, 1)} <= set(retained):
        raise ValueError("Both complete-receiver parents must have reproduced")
    selected = retained[(1, 0)]
    tree = parent.parent / "candidates" / selected["id"] / "first/tree"
    for relative in (SOURCE, "include/singleselectionwindow.h"):
        if (HOMM3_DIR / relative).read_bytes() != (tree / relative).read_bytes():
            raise ValueError("Authored source is not the reproduced parent: " + relative)
    source = (HOMM3_DIR / SOURCE).read_text()
    begin = source.index("bool TSingleSelectionWindow::handleNetMsg(")
    end = source.index("\n}\n", begin) + 3
    function = source[begin:end]
    pairs = list(re.finditer(r"#pragma inline_depth\(0\)\n.*?#pragma inline_depth\(\)\n",
                            function, re.S))
    if len(pairs) != function.count("#pragma inline_depth(0)"):
        raise ValueError("Unpaired or nested dispatcher overrides")
    options = [{"name": "current-dispatcher-control"}]
    for index, pair in enumerate(pairs):
        block = pair.group()
        code = block.replace("#pragma inline_depth(0)\n", "").replace("#pragma inline_depth()\n", "")
        options.append({"name": "remove-region-%02d" % (index + 1),
                        "replace": function[:pair.start()] + code + function[pair.end():]})
    options.append({"name": "remove-all-dispatcher-regions", "replace":
                    function.replace("#pragma inline_depth(0)\n", "").replace("#pragma inline_depth()\n", "")})
    start = source.index("// auto_inline(off): retail CALLS this from HandleNetMsg's request arm;")
    stop = source.index("#pragma auto_inline(on)", start) + len("#pragma auto_inline(on)")
    pinned = source[start:stop]
    ordinary = pinned[pinned.index("// E:\\gamedcs\\singleselectionwindow.cpp:1492"):]
    ordinary = ordinary.replace("\n#pragma auto_inline(on)", "")
    return {"schema": 1, "source": SOURCE, "units": ["singleselectionwindow"],
            "evidence": __doc__, "parent_context": parent.parent.name,
            "parent_objects": {str(key): retained[key]["object_hash"] for key in ((1, 0), (1, 1))},
            "region_statements": [p.group() for p in pairs], "axes": [
                {"name": "dispatcher-existing-depth-regions", "find": function, "options": options},
                {"name": "header-requested-override", "find": pinned, "options": [
                    {"name": "current-auto-inline-control"},
                    {"name": "natural-header-requested", "replace": ordinary},
                ]},
            ]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path, help="Completed parent checkpoint.json")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest(args.parent)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    count = len(payload["axes"][0]["options"]) * 2
    print(count, "source states;", len(payload["region_statements"]), "existing depth-zero regions")
