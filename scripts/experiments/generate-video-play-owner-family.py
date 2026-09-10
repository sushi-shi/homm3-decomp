#!/usr/bin/env python3
"""Retail videoPlay active-track ownership through the initial Smacker draw."""
import argparse
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
start = source.index("int videoPlay(int id, int x, int y, int w, int h)\n{")
body = source[start:source.index("\n}\n", start) + 2]

hide = "            g_mouseManager->hidePointer();"
declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
uses = (
    "g_smackVideo->m_width",
    "g_smackVideo->m_height",
)
call = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
assert body.count(hide) == 1
assert body.count(declarations) == 1
assert body.count(uses[0]) == 2
assert body.count(uses[1]) == 2
assert body.count(call) == 1


def pointer_uses(changed, expression):
    for use in uses:
        changed = changed.replace(use, expression + use[len("g_smackVideo"):])
    return changed.replace(call, "_SmackToBuffer(" + expression + ", pos.x, pos.y,")


options = [{"name": "control"}]

changed = body.replace(hide, hide + "\n            Smack* smk = g_smackVideo;")
options.append({"name": "branch-pointer-after-hide", "replace": pointer_uses(changed, "smk")})

changed = body.replace(hide, "            Smack* smk = g_smackVideo;\n" + hide)
options.append({"name": "branch-pointer-before-hide", "replace": pointer_uses(changed, "smk")})

changed = body.replace(declarations, declarations + "\n    Smack* smk;")
changed = changed.replace(hide, hide + "\n            smk = g_smackVideo;")
options.append({"name": "function-pointer-assigned-after-hide", "replace": pointer_uses(changed, "smk")})

changed = body.replace(declarations, declarations + "\n    Smack* smk;")
changed = changed.replace(hide, "            smk = g_smackVideo;\n" + hide)
options.append({"name": "function-pointer-assigned-before-hide", "replace": pointer_uses(changed, "smk")})

changed = body.replace(hide, hide + "\n            Smack& smk = *g_smackVideo;")
changed = pointer_uses(changed, "&smk")
changed = changed.replace("&smk->", "smk.")
options.append({"name": "branch-reference-after-hide", "replace": changed})

changed = body.replace(hide, hide + "\n            Smack* const smk = g_smackVideo;")
options.append({"name": "branch-const-pointer-after-hide", "replace": pointer_uses(changed, "smk")})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "active-smacker-owner-lifetime",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a four-byte platform stub, so retail 0x5972d0 is authoritative for this Complete-only body.",
        "After hidePointer, retail tests vw and loads g_smackVideo into ESI before the first signed branch; ESI then supplies both dimension fields, both center calculations, and the SmackToBuffer argument.",
        "The current repeated global expressions instead use ECX and allocate the entry parameters in edi,esi,ebx order. Test one real owner binding across exactly the retail-observed use interval.",
        "The wait loop deliberately continues to read g_smackVideo because close/update code can change it; these variants do not cache the track across that loop.",
        "Seven finite source states preserve all calls, branches, SDK interfaces, POINT storage, and the proven unrotated loop.",
    ],
}, indent=2) + "\n")
