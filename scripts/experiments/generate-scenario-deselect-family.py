"""Recover the ordinary CHeroWindowEx deselect handler and remove its caller pin.

DC window.cpp:1122/1123 (0x197f48) owns the base's return-zero body;
scenarioinfo.cpp:503 (0x12ab00) calls it. The protected virtual signature
uses bool&, proven by ?OnWidgetDeselect@CHeroWindowEx@@MAAHHAA_N@Z.
Retail 0x5698a0 retains a call to the five-byte body at 0x559140, whose
existing canonical claim is t_stdio_file_adapter::write. ICF folding
does not prove a header-inline definition. Move the body into window.cpp,
after WindowHandler and before GetRolloverWidget in recovered source order.

Cross the current header definition / recovered ordinary body with the
existing caller pin / natural call. Four finite states are sufficient;
all configured window.h consumers are scored, not just scenarioinfo.
Run after the atomic bool-reference interface migration has a full-build
checkpoint, so renamed symbols are present in the scoring inventory.
"""

import argparse
import json
from pathlib import Path
import subprocess
import tomllib

from homm3.core.common import HOMM3_DIR


DECL = "    virtual int onWidgetDeselect(int id, bool& exitFlag) { return 0; }  // slot 12"
CALL = """#pragma inline_depth(0)
    return CHeroWindowEx::onWidgetDeselect(id, exitFlag);
#pragma inline_depth()"""
BODY = """// E:\\gamedcs\\window.cpp:1122/1123, dc 0x197f48. Ordinary source-owned
// body, not a header inline. Retail vtable slot 12 and CScenarioInfoDlg's
// qualified call resolve to 0x559140 (xor eax,eax; ret 8), ICF-folded with
// t_stdio_file_adapter::write; that existing claim remains the sole owner.
// Before normalization (function/parameter): OnWidgetDeselect, bExitFlag.
DC_ONLY(0x197f48, 0x4)
int CHeroWindowEx::onWidgetDeselect(int id, bool& exitFlag)
{
    return 0;
}

"""
BODY_ANCHOR = "// E:\\gamedcs\\window.cpp:1128 - vtable 0x243ce8 slot 13."


def consumers():
    configured = {row["unit"] for row in tomllib.loads(
        (HOMM3_DIR / "config/units.toml").read_text())["unit"]}
    result = set()
    output = subprocess.check_output(["ninja", "-t", "deps"], cwd=HOMM3_DIR, text=True)
    for block in output.split("\n\n"):
        lines = block.splitlines()
        if not lines or str(HOMM3_DIR / "include/window.h") not in (
                line.strip() for line in lines[1:]):
            continue
        unit = Path(lines[0].split(":", 1)[0]).stem
        if unit not in configured or "(VALID)" not in lines[0]:
            raise ValueError("Refresh full-build dependencies: " + lines[0])
        result.add(unit)
    if not {"window", "scenarioinfo", "multiplayerwindow", "hiscore",
            "customcampaignwindow"}.issubset(result):
        raise ValueError("Missing callback-family consumers")
    return sorted(result)


def make_manifest():
    source = (HOMM3_DIR / "src/window.cpp").read_text()
    comment = source.index("// E:\\gamedcs\\window.cpp:1122 - OnWidgetDeselect has NO window.obj row.")
    start = source.rindex("#if 0  // @carcass", 0, comment)
    end = source.index("#endif  // @carcass", comment) + len("#endif  // @carcass\n\n")
    carcass = source[start:end]
    return {"schema": 1, "source": "src/scenarioinfo.cpp", "units": consumers(),
            "evidence": __doc__, "axes": [
                {"name": "base-handler-owner", "source": "include/window.h",
                 "find": DECL, "options": [
                     {"name": "current-header-control"},
                     {"name": "ordinary-window-body",
                      "replace": "    virtual int onWidgetDeselect(int id, bool& exitFlag);  // slot 12",
                      "extra_edits": [
                          {"source": "src/window.cpp", "find": carcass, "replace": ""},
                          {"source": "src/window.cpp", "insert_before": BODY_ANCHOR, "text": BODY},
                      ]},
                 ]},
                {"name": "scenario-call", "find": CALL, "options": [
                    {"name": "current-pin-control"},
                    {"name": "natural-call",
                     "replace": "    return CHeroWindowEx::onWidgetDeselect(id, exitFlag);"},
                ]},
            ]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest()
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    print("4 source states;", len(payload["units"]), "header consumers")
