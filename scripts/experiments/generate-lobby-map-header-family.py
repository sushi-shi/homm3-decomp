"""Recover the lobby's ordinary map-header receive helper, then reprice its pins.

DC singleselectionwindow.cpp:6529 calls OnNewMapHeaderInfo (0x140d50),
whose ordinary definition starts at line 6968 and returns bool (QAA_N).
Complete's HandleNetMsg +0x310..+0x365 constructs t_complex_net_message,
constructs NewSMapHeader at +0x18, installs CNewMapHeaderInfoMsg's vtable,
receives through 0x512e00, calls SetupOrigData and destroys the header.
The existing flattened arm omits the receiver entirely. Its header-ctor
pin is not needed in the recovered helper hypothesis and is removed.

Cross that recovery with removal of the existing HeaderRequested auto-inline
override: its previous negative control predates this missing receive body.
All singleselectionwindow.h consumers are scored. Four finite states suffice.
Run after the proven bool-reference dispatcher interface has a full checkpoint.
"""

import argparse
import json
from pathlib import Path
import subprocess
import tomllib

from homm3.core.common import HOMM3_DIR


SOURCE = "src/singleselectionwindow.cpp"
ARM = """    case RS_NEW_MAP_HEADER_INFO: {
        // Residual: retail wraps this header in an 0x18-byte stream-reader
        // base (ctor 0x512c20, vtable 0x641d30) and fills it from the
        // message via 0x512e00; the reader class is not yet modeled, so
        // only the header local, its teardown and the SetupOrigData tail
        // survive here.
#pragma inline_depth(0)
        NewSMapHeader header;
#pragma inline_depth()
        g_game->setupOrigData();
        break;
    }
"""
CALL = """    case RS_NEW_MAP_HEADER_INFO:
        onNewMapHeaderInfo(netMsg);
        break;
"""
STUB = """// E:\\gamedcs\\singleselectionwindow.cpp:6968
DC_ONLY(0x140d50, 0x22)
unsigned char TSingleSelectionWindow::OnNewMapHeaderInfo(CNetMsg* pNetMsg)
{
    // @stub
}
"""
BODY = """#endif  // @carcass

// DC line 6529 calls this ordinary helper; line 6968 owns its definition.
// Complete adds the serialized receiver visible in HandleNetMsg +0x310:
// base ctor 0x512c20, header at +0x18, vtable 0x641d30, read 0x512e00,
// SetupOrigData, and header teardown. The receive result is not tested.
// Before normalization (function/parameter): OnNewMapHeaderInfo, pNetMsg.
// E:\\gamedcs\\singleselectionwindow.cpp:6968
DC_ONLY(0x140d50, 0x22)
bool TSingleSelectionWindow::onNewMapHeaderInfo(CNetMsg* netMsg)
{
    CNewMapHeaderInfoMsg msg;
    msg.remoteFn00512E00(netMsg);
    g_game->setupOrigData();
    return true;
}

#if 0  // @carcass
"""
DECL_ANCHOR = "    // Before normalization (function): TSingleSelectionWindow::OnGameHeaderInfoInitMsg."
DECL = """    // DC ordinary OnNewMapHeaderInfo, source line 6968; QAA_N return.
    bool onNewMapHeaderInfo(CNetMsg* netMsg);
"""


def consumers():
    configured = {row["unit"] for row in tomllib.loads(
        (HOMM3_DIR / "config/units.toml").read_text())["unit"]}
    result = set()
    output = subprocess.check_output(["ninja", "-t", "deps"], cwd=HOMM3_DIR, text=True)
    for block in output.split("\n\n"):
        lines = block.splitlines()
        if not lines or str(HOMM3_DIR / "include/singleselectionwindow.h") not in (
                line.strip() for line in lines[1:]):
            continue
        unit = Path(lines[0].split(":", 1)[0]).stem
        if unit not in configured or "(VALID)" not in lines[0]:
            raise ValueError("Refresh full-build dependencies: " + lines[0])
        result.add(unit)
    if "singleselectionwindow" not in result:
        raise ValueError("Missing lobby consumer")
    return sorted(result)


def make_manifest():
    source = (HOMM3_DIR / SOURCE).read_text()
    start = source.index("// auto_inline(off): retail CALLS this from HandleNetMsg's request arm;")
    end = source.index("#pragma auto_inline(on)", start) + len("#pragma auto_inline(on)")
    pinned = source[start:end]
    recovered = pinned[pinned.index("// E:\\gamedcs\\singleselectionwindow.cpp:1492"):]
    recovered = recovered.replace("\n#pragma auto_inline(on)", "")
    return {"schema": 1, "source": SOURCE, "units": consumers(),
            "evidence": __doc__, "axes": [
                {"name": "map-header-receive-owner", "find": ARM, "options": [
                    {"name": "incomplete-pinned-control"},
                    {"name": "ordinary-complete-receiver", "replace": CALL,
                     "extra_edits": [
                         {"source": SOURCE, "find": STUB, "replace": BODY},
                         {"source": "include/singleselectionwindow.h",
                          "insert_before": DECL_ANCHOR, "text": DECL},
                     ]},
                ]},
                {"name": "header-requested-override", "find": pinned, "options": [
                    {"name": "current-auto-inline-control"},
                    {"name": "natural-header-requested", "replace": recovered},
                ]},
            ]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest()
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    print("4 source states;", len(payload["units"]), "header consumers:", ", ".join(payload["units"]))
