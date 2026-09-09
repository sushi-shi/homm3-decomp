"""Separate native-bool writer results and the two existing Save fences.

The typed vector-helper family removes the gate-pair pointer union and all
six Load resize fences while improving both callers. Its direct-expression
native-bool writer uses SBB/INC where retail retains SETAE, so test the result lifetime
before treating this as an ABI contradiction. Keep four load-helper parents
and every subset of the two old Save fences; no new override is introduced.
The canonical helper/template ownership and reference parameter stay fixed.
"""

import argparse
import importlib.util
import itertools
import json
from pathlib import Path


def make_manifest():
    path = Path(__file__).with_name("generate-game-vector-helper-family.py")
    spec = importlib.util.spec_from_file_location("vector_helper_parent", path)
    parent = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parent)
    manifest = parent.make_manifest()
    axis = manifest["axes"][0]
    options = [dict(name="unchanged")]
    guard = """    if (outfile->write(&srcVector[0], static_cast<short>(count) * sizeof(T))
        < static_cast<short>(count) * sizeof(T))
        return false;
    return true;
"""
    comparison = ("outfile->write(&srcVector[0], static_cast<short>(count) * sizeof(T))\n"
                  "        >= static_cast<short>(count) * sizeof(T)")
    returns = {
        "guard": guard,
        "expression": "    return " + comparison + ";\n",
        "bool-local": "    bool written = " + comparison + ";\n    return written;\n",
        "const-bool-local": "    const bool written = " + comparison + ";\n    return written;\n",
        "byte-local": "    unsigned char written = " + comparison + ";\n    return written;\n",
        "success-guard": "    if (" + comparison + ")\n        return true;\n    return false;\n",
    }
    for index, (form, result), mask in itertools.product((1, 9, 17, 25), returns.items(), range(4)):
        option = json.loads(json.dumps(axis["options"][index]))
        option["name"] = f"parent-{index}-{form}-remove-{mask:02b}"
        for edit in option["extra_edits"]:
            if "insert_before" in edit:
                if edit["text"].count(guard) != 1:
                    raise ValueError("Review writer result anchor")
                edit["text"] = edit["text"].replace(guard, result)
            elif edit["find"].startswith("    // PINNED for the same reason"):
                lines = edit["replace"].splitlines(keepends=True)
                bit = 0
                new = []
                for line in lines:
                    if line.strip() == "#pragma inline_depth(0)":
                        if not mask & (1 << bit):
                            new.append(line)
                    elif line.strip() == "#pragma inline_depth()":
                        if not mask & (1 << bit):
                            new.append(line)
                        bit += 1
                    else:
                        new.append(line)
                if bit != 2:
                    raise ValueError("Review Save fence inventory")
                edit["replace"] = "".join(new)
        options.append(option)
    axis["name"] = "vector-result-lifetimes"
    axis["options"] = options
    manifest["evidence"] = __doc__
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
