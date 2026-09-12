#!/usr/bin/env python3
"""Distinguish object-coordinate value copying from a borrowed accessor result.

Retail shipyard and treasure placement consume three-coordinate snapshots;
monolith copies into by-value border and guard arguments. The ordinary
getPosition accessor has no retained body or Dreamcast declaration, so those
copies could belong to the accessor or to its callers. Compare a const-reference
result with value/const-value results and real copy or construction lifetimes.
Preserve the existing accessor, every caller, canonical coordinate constructor,
implicit special members and all addition/guard parameter types. Check every
consumer, including exact object and treasure placement functions.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    start = source.rfind("\n", 0, source.index("type_object::getPosition() const")) + 1
    original = source[start:source.index("\n}", start) + 2]
    header = (HOMM3_DIR / "include/rmg.h").read_text()
    declarations = [line for line in header.splitlines() if "getPosition() const;" in line]
    assert len(declarations) == 1
    declaration = declarations[0]
    bodies = {
        "member": "    return m_position;",
        "copy_temporary": "    return TRmgMapPosition(m_position);",
        "coordinates": "    return TRmgMapPosition(m_position.m_x, m_position.m_y, m_position.m_z);",
        "named_copy": "    TRmgMapPosition result = m_position;\n    return result;",
    }
    options = [{"name": "source_control", "replace": original}]
    for result in ("TRmgMapPosition", "const TRmgMapPosition", "const TRmgMapPosition&"):
        for label, body in bodies.items():
            if result.endswith("&") and label != "member":
                continue
            replacement = result + " type_object::getPosition() const\n{\n" + body + "\n}"
            if replacement == original:
                continue
            options.append({"name": result + "+" + label, "replace": replacement, "extra_edits": [
                {"source": "include/rmg.h", "find": declaration,
                 "replace": "    " + result + " getPosition() const;"}]})
    payload = {"schema": 1, "source": "src/rmg.cpp", "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles", "singleselectionpopups",
                         "singleselectionwindow", "scenarioinfo"],
               "axes": [{"name": "position_accessor", "find": original, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(str(len(options)) + " object-position accessor ownership/lifetime states")


if __name__ == "__main__":
    main()
