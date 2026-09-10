#!/usr/bin/env python3
"""Retail smackmgr SDK constants as macros versus TU-local const objects."""
import argparse
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()

constants = [
    ("buffer555",
     "static const unsigned int g_smackBuffer555 = 0x80000000;",
     "#define SMACKBUFFER555 0x80000000UL",
     "g_smackBuffer555", "SMACKBUFFER555"),
    ("buffer565",
     "static const unsigned int g_smackBuffer565 = 0xC0000000;",
     "#define SMACKBUFFER565 0xC0000000UL",
     "g_smackBuffer565", "SMACKBUFFER565"),
    ("from-archive",
     "static const int g_smackOpenFromArchive = 0x1140;",
     "#define SMACKOPEN_FROM_ARCHIVE 0x1140",
     "g_smackOpenFromArchive", "SMACKOPEN_FROM_ARCHIVE"),
    ("track-mask",
     "static const int g_smackTrackMask = 0xfe000;",
     "#define SMACK_TRACK_MASK 0xfe000",
     "g_smackTrackMask", "SMACK_TRACK_MASK"),
    ("no-frame-skip",
     "static const int g_smackOpenNoFrameSkip = 0x200;",
     "#define SMACKOPEN_NO_FRAME_SKIP 0x200",
     "g_smackOpenNoFrameSkip", "SMACKOPEN_NO_FRAME_SKIP"),
]
for _, declaration, _, name, _ in constants:
    assert source.count(declaration) == 1, declaration
    assert source.count(name) >= 2, name

options = [{"name": "const-objects"}]
for mask in range(1, 1 << len(constants)):
    changed = source
    labels = []
    for bit, (label, declaration, macro, name, macro_name) in enumerate(constants):
        if mask & (1 << bit):
            labels.append(label)
            changed = changed.replace(declaration, macro)
            # Replace uses after replacing the declaration, whose new macro
            # spelling must remain intact.
            changed = changed.replace(name, macro_name)
    options.append({"name": "macros-" + "-".join(labels), "replace": changed})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "sdk-constant-declaration-state",
        "find": source,
        "options": options,
    }],
    "evidence": [
        "Dreamcast identifies the same smackmgr.cpp module, while its CodeView symbol stream demonstrates a different TU declaration state and the Complete-only videoPlay body supplies no local evidence.",
        "SMACKBUFFER555, SMACKBUFFER565 and SmackOpen flag values are SDK preprocessor constants. The reconstruction currently models all five values as TU-local static const objects, which preserve expressions but create C1XX symbols absent from the original header interface.",
        "why-reg isolates videoPlay's residual to a front-end pseudo processing-order permutation. Test every subset of these five real SDK/project constant representations and require the other exact smackmgr functions to remain exact.",
        "Thirty-two finite states preserve every runtime value, operation, call, branch, and ABI; no dummy declaration or compiler directive.",
    ],
}, indent=2) + "\n")
