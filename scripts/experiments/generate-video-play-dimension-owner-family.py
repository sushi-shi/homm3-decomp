#!/usr/bin/env python3
"""Retail videoPlay width/height parameter versus local ownership."""
import argparse
import json
import re
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
start = source.index("int videoPlay(int id, int x, int y, int w, int h)\n{")
body = source[start:source.index("\n}\n", start) + 2]

declaration = "    int vw, vh;\n"
assignments = "        vh = h;\n        vw = w;\n"
assert body.count(declaration) == 1
assert body.count(assignments) == 1


def ownership(width, height):
    changed = body.replace(declaration, "")
    declarations = []
    initializers = []
    if width == "local":
        declarations.append("vw")
        initializers.append("        vw = w;")
    elif width == "reference":
        declarations.append("int& vw = w")
    else:
        changed = re.sub(r"\bvw\b", "w", changed)
    if height == "local":
        declarations.append("vh")
        initializers.insert(0, "        vh = h;")
    elif height == "reference":
        declarations.append("int& vh = h")
    else:
        changed = re.sub(r"\bvh\b", "h", changed)
    if "vw" in declarations or "vh" in declarations:
        locals_only = [name for name in declarations if name in ("vw", "vh")]
        refs = [name for name in declarations if "&" in name]
        text = ""
        if locals_only:
            text += "    int " + ", ".join(locals_only) + ";\n"
        for ref in refs:
            text += "    " + ref + ";\n"
        changed = changed.replace("    unsigned char result;\n", text + "    unsigned char result;\n")
    changed = changed.replace(assignments, "\n".join(initializers) + ("\n" if initializers else ""))
    return changed


options = []
for width in ("local", "parameter", "reference"):
    for height in ("local", "parameter", "reference"):
        changed = ownership(width, height)
        option = {"name": "width-%s-height-%s" % (width, height)}
        if changed != body:
            option["replace"] = changed
        options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "dimension-value-owners",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves int w/h parameters but has no Complete body or local records. Retail 0x5972d0 is authoritative for the dimension owners.",
        "Retail and candidate both assign vw to ESI first. Retail next assigns vh to EDI and x to EBX; candidate assigns x to EDI and vh to EBX. Both retail dimensions are homed in the incoming w/h slots across showVideo.",
        "Test each dimension as a copied signed local, the mutable parameter itself, or a source reference to that parameter. Partial ownership distinguishes the x/vh interference that the earlier both-parameter state could not isolate.",
        "Nine finite states preserve values, signed corrections, calls, branches, POINT, and the proven loop.",
    ],
}, indent=2) + "\n")
