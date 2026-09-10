#!/usr/bin/env python3
"""Retail videoPlay translation-unit include-order family."""
import argparse
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
includes = [
    "#include <va.h>",
    "#include <windows.h>",
    "#include <ddraw.h>",
    "#include <string.h>",
    "#include <string>",
    '#include "smackmgr.h"',
    '#include "binkmanager.h"',
    '#include "wingraph.h"',
    '#include "soundmgr.h"',
    '#include "winmgr.h"',
    '#include "mousemgr.h"',
    '#include "inputmgr.h"',
    '#include "kbwin.h"',
    '#include "bitmap16.h"',
    '#include "message.h"',
    '#include "prefs.h"',
    '#include "textresource.h"',
]
block = "\n".join(includes)
assert source.count(block) == 1


def option(name, rows):
    replacement = "\n".join(rows)
    value = {"name": name}
    if replacement != block:
        value["replace"] = replacement
    return value


options = [option("control", includes)]
for i, include in enumerate(includes):
    options.append(option("remove-%02d-%s" % (i, Path(include.split()[-1].strip('<>\"')).stem),
                          includes[:i] + includes[i + 1:]))
for i in range(len(includes) - 1):
    rows = list(includes)
    rows[i], rows[i + 1] = rows[i + 1], rows[i]
    options.append(option("swap-%02d-%02d" % (i, i + 1), rows))
for i, include in enumerate(includes):
    rows = includes[:i] + includes[i + 1:]
    options.append(option("move-%02d-first" % i, [include] + rows))
    options.append(option("move-%02d-last" % i, rows + [include]))

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "post-pch-include-order",
        "find": block,
        "options": options,
    }],
    "evidence": [
        "Dreamcast identifies smackmgr.cpp and exposes a module type/declaration stream that differs from the Complete reconstruction, while its VideoPlay body is a four-byte platform stub.",
        "Retail and candidate VideoPlay agree on size, CFG, calls, stack frame, and definition slots; why-reg isolates the residual to C1 front-end pseudo-processing state. Existing includes are the declarations actually parsed after the terrain PCH and can change that state by relative creation order.",
        "Test every redundant single-include removal, adjacent swap, and one-include move to each boundary. A state is admissible only if it compiles with the pinned profile, VideoPlay closes, and all currently exact smackmgr siblings stay exact.",
        "%d finite states; no synthetic declaration, function-body operation, compiler directive, or header edit." % len(options),
    ],
}, indent=2) + "\n")
