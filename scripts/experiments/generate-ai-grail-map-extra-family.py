"""Restore the shared map-extra point overload before judging Grail inlining.

DC AdvMgr.h:1254, dc 0x1f084, proves an int-returning header inline taking
type_point by value and forwarding x/y/z to the unsigned-short scalar
overload. find_all_destinations calls it at lines 3300 and 3359; retail
0x42edd0 retains the scalar calls at +0x161 and +0x380. Its current first
site models the copy as a caller-local probe and both sites paste coordinate
extraction. The canonical body currently exists only inside advmgr.cpp.
Move, do not duplicate, that definition into its original owning header.
Remove findpath.cpp's second, falsely static copy as part of the same move.
The header's get_map_center member is DC line 1245, before this helper at
1254; place the free overload after the advManager class, not before it.
Retain the real scalar declaration for header visibility; it agrees exactly
with kb.h's declaration and the retained 0x4f79b0 definition.

Cross original/scoped point lifetimes and the two call sites with the
artifact constructor controls. Header collateral includes every live
advmgr.h consumer. No new pins or inline qualifiers are introduced.
"""

import argparse
import json
from pathlib import Path
import subprocess
import tomllib

from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator


LOCAL = """// The DC header overload survives here only through /Ob2 expansion. The
// retail instruction stream independently proves its by-value forwarding
// shape.
// Before normalization (function): GetMapExtra.
inline int getMapExtra(type_point point)
{
    return ::getMapExtra(point.m_x, point.m_y, point.m_z);
}

"""
SHARED = """// AdvMgr.h:1254, dc 0x1f084. The by-value point overload forwards all
// three coordinates to kb.cpp's retained scalar accessor (0x4f79b0).
// The scalar declaration also remains in kb.h for its scalar-only users.
// Before normalization (function): GetMapExtra.
unsigned short getMapExtra(int x, int y, int z);
inline int getMapExtra(type_point point)
{
    return ::getMapExtra(point.m_x, point.m_y, point.m_z);
}

"""
FIRST = """            type_point probe = cell->m_point;
            if ((getMapExtra(probe.m_x, probe.m_y, probe.m_z) & g_unnamed69ccc4)
"""
SECOND = """        if (!(getMapExtra(point.m_point.m_x, point.m_point.m_y, point.m_point.m_z)
              & g_unnamed69ccc4)
"""


def consumers():
    configured = {row["unit"] for row in tomllib.loads(
        (HOMM3_DIR / "config/units.toml").read_text())["unit"]}
    output = subprocess.check_output(["ninja", "-t", "deps"], cwd=HOMM3_DIR, text=True)
    result = set()
    for block in output.split("\n\n"):
        lines = block.splitlines()
        if not lines or str(HOMM3_DIR / "include/advmgr.h") not in (
                line.strip() for line in lines[1:]):
            continue
        unit = Path(lines[0].split(":", 1)[0]).stem
        if unit not in configured or "(VALID)" not in lines[0]:
            raise ValueError("Refresh the full-build dependency closure: " + lines[0])
        result.add(unit)
    if not {"ai_player", "advmgr", "kb"}.issubset(result):
        raise ValueError("Missing expected accessor consumers")
    return sorted(result)


def make_manifest():
    module = generator("generate-ai-grail-lifetime-family.py")
    source = (HOMM3_DIR / "src/ai_player.cpp").read_text()
    helper = module.function(source, "static void checkHolyGrail(")
    caller = module.function(source, "long findAllDestinations(hero* currentHero,")
    path_source = (HOMM3_DIR / "src/findpath.cpp").read_text()
    path_start = path_source.index("// AdvMgr.h:1254 in the Dreamcast roster")
    path_end = path_source.index("// E:\\gamedcs\\findpath.cpp:461", path_start)
    path_copy = path_source[path_start:path_end]
    control = {"name": "flattened-calls-control"}
    options = [control]
    for name, first, second, copied in (
            ("header-only", False, False, False),
            ("first-site", True, False, False),
            ("second-site", False, True, False),
            ("both-sites-copy-control", True, True, True),
            ("both-sites", True, True, False)):
        body = caller
        if first:
            if copied:
                replacement = """            type_point probe = cell->m_point;
            if ((getMapExtra(probe) & g_unnamed69ccc4)
"""
            else:
                replacement = """            if ((getMapExtra(cell->m_point) & g_unnamed69ccc4)
"""
            body = body.replace(FIRST, replacement)
        if second:
            body = body.replace(SECOND, """        if (!(getMapExtra(point.m_point) & g_unnamed69ccc4)
""")
        options.append({"name": name, "replace": body, "extra_edits": [
            {"source": "src/advmgr.cpp", "find": LOCAL, "replace": ""},
            {"source": "src/findpath.cpp", "find": path_copy, "replace": ""},
            {"source": "include/advmgr.h", "insert_before": "// Retail .bss 0x699268 (DC ?gpAdvManager@@3PAVadvManager@@A).", "text": SHARED},
        ]})
    artifact_options = []
    for index, (name, body) in enumerate(module.helper_forms(helper)):
        if index not in (0, 4, 12, 16):
            continue
        option = {"name": name}
        if body != helper:
            option["replace"] = body
        artifact_options.append(option)
    return {"schema": 1, "source": "src/ai_player.cpp", "units": consumers(),
            "evidence": __doc__, "axes": [
                {"name": "shared-map-extra-boundary", "find": caller, "options": options},
                {"name": "artifact-construction", "find": helper, "options": artifact_options},
            ]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest()
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    print("24 source states; header consumers:", ", ".join(payload["units"]))
