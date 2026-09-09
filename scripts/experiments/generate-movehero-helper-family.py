"""Complete moveHero's ordinary static helper declarations and early exit.

DC RestoreMouse (0x10d57c, source 150) and check_for_town (0x10d5b4,
source 165) are the two other ordinary static helpers expanded by retail.
check_for_town's 170/171 negative-ID return closes before GetTown and
AI_enter_town. Carry four reviewed shipyard parents with all three continue
scopes, and separate these ordinary declarations from that early exit.
Forced declarations survive only as pre-existing negative controls.
"""

import argparse
import importlib.util
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    path = Path(__file__).with_name("generate-shipyard-scope-family.py")
    spec = importlib.util.spec_from_file_location("shipyard_scope_parent", path)
    parent = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parent)
    manifest = parent.make_manifest()
    axis = manifest["axes"][0]
    original = (ROOT / "src/philai.cpp").read_text()
    start = original.index("// E:\\gamedcs\\philai.cpp:150\n")
    end = original.index("#if 0  // @carcass\n", start)
    helper_source = original[start:end]
    options = [dict(name="unchanged")]
    for parent_index, ordinary, early in itertools.product((43, 47, 91, 95), range(4), range(2)):
        helpers = helper_source
        for bit, name in enumerate(("restoreMouse", "checkForTown")):
            if ordinary & (1 << bit):
                helpers = parent.replace(helpers,
                    "static __forceinline void " + name, "static void " + name)
        if early:
            helpers = parent.replace(helpers,
                "    if (townId >= 0)\n        aiEnterTown(currentHero, g_game->getTown(townId));",
                "    if (townId < 0)\n        return;\n    aiEnterTown(currentHero, g_game->getTown(townId));")
        options.append(dict(name=f"shipyard-{parent_index}-ordinary-{ordinary:02b}-early-{early}",
            replace=axis["options"][parent_index]["replace"],
            extra_edits=[dict(find=helper_source, replace=helpers)]))
    axis["name"] = "movehero-helper-boundaries"
    axis["options"] = options
    manifest["evidence"] = __doc__
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
