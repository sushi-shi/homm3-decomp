#!/usr/bin/env python3
"""Retail videoPlay source-local semantic-name family."""
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


def rename(changed, point="pos", width="vw", height="vh",
           result="result", aborted="aborted"):
    # Token boundaries are explicit because each replacement is a C++ identifier.
    import re
    for old, new in (("pos", point), ("vw", width), ("vh", height),
                     ("result", result), ("aborted", aborted)):
        changed = re.sub(r"\b" + old + r"\b", new, changed)
    return changed


forms = [("current", {})]
for name in ("pt", "point", "position", "videoPos", "updatePos", "drawPos",
             "origin", "screenPos", "updatePoint"):
    forms.append(("point-" + name, {"point": name}))
for label, width, height in (
        ("width-height", "width", "height"),
        ("video-width-height", "videoWidth", "videoHeight"),
        ("draw-width-height", "drawWidth", "drawHeight"),
        ("smack-width-height", "smackWidth", "smackHeight"),
        ("sw-sh", "sw", "sh"), ("ww-hh", "ww", "hh"),
        ("cx-cy", "cx", "cy"), ("dx-dy", "dx", "dy"),
        ("w2-h2", "w2", "h2")):
    forms.append(("dimensions-" + label, {"width": width, "height": height}))
for label, result, aborted in (
        ("retval-abort", "retVal", "abort"),
        ("rval-abort", "rval", "abort"),
        ("return-abort", "returnValue", "abort"),
        ("success-abort", "success", "abort"),
        ("result-abort", "result", "abort"),
        ("retval-aborted", "retVal", "aborted"),
        ("result-cancelled", "result", "cancelled"),
        ("result-skipped", "result", "skipped"),
        ("rc-abortflag", "rc", "abortFlag")):
    forms.append(("flags-" + label, {"result": result, "aborted": aborted}))

# Curated combinations use likely original-era short names and descriptive names.
forms.extend([
    ("pt-width-height-retval-abort",
     {"point": "pt", "width": "width", "height": "height",
      "result": "retVal", "aborted": "abort"}),
    ("point-width-height-result-abort",
     {"point": "point", "width": "width", "height": "height",
      "result": "result", "aborted": "abort"}),
    ("pt-sw-sh-rval-abort",
     {"point": "pt", "width": "sw", "height": "sh",
      "result": "rval", "aborted": "abort"}),
    ("position-video-dims-success-cancelled",
     {"point": "position", "width": "videoWidth", "height": "videoHeight",
      "result": "success", "aborted": "cancelled"}),
    ("updatepos-draw-dims-result-skipped",
     {"point": "updatePos", "width": "drawWidth", "height": "drawHeight",
      "result": "result", "aborted": "skipped"}),
])

options = []
seen = set()
for label, names in forms:
    changed = rename(body, **names)
    if changed in seen:
        continue
    seen.add(changed)
    option = {"name": label}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "semantic-local-names",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the parameter names but its platform stub records no Complete-only locals. The current POINT/dimension/flag names are therefore reconstructed names rather than source facts.",
        "Retail and candidate have identical pseudo definition slots, CFG, calls, and frame, but C2 receives those pseudos in a different processing order. Test whether C1's local-symbol state depends on the unknown semantic names before changing any operation or lifetime.",
        "Names are drawn from the surrounding video code, Win32 POINT idioms, and ordinary late-1990s C++ spellings. Each state renames declarations and all uses together, preserving types, scopes, statements, calls, branches, and external ABI.",
        "Thirty-two finite name sets; retain only a retail-exact result with every sibling unchanged.",
    ],
}, indent=2) + "\n")
