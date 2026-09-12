#!/usr/bin/env python3
"""Restore the position constructor to its evidenced RMG source location.

Retail 0x5355c0 lies immediately before canFitObject at 0x5355e0, and RMG
callers both retain and expand this ordinary constructor. The previous
support-file placement prevents every rmg.cpp expansion. A documented older
probe rejected the move because some callers' scores fell; that is not source
evidence against ownership. Compare the current split as a negative control
with the owning TU at the retail position, using the current member
initializers and ordinary X/Y/Z body stores. Preserve the by-value ABI,
implicit copy operations, all callers, source order and canonical body.
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
    support = (HOMM3_DIR / "src/rmg_support.cpp").read_text()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    marker = "// Retail retains this tiny value constructor"
    owner = "src/rmg_support.cpp" if marker in support else "src/rmg.cpp"
    current = support if owner.endswith("rmg_support.cpp") else source
    start = current.index(marker)
    end = current.index("\n}\n", start) + 3
    block = current[start:end]
    anchor = "// Complete-only group fit predicate, recovered on decomp-complete-4.0 in"
    assigned = block.replace("\n    : m_x(newX), m_y(newY), m_z(newZ)\n{\n}",
                             "\n{\n    m_x = newX;\n    m_y = newY;\n    m_z = newZ;\n}")
    assert assigned != block
    options = [{"name": "source_control", "replace": block}]
    if owner == "src/rmg_support.cpp":
        for label, owned in (("owning_tu_initializers", block), ("owning_tu_body_stores", assigned)):
            options.append({"name": label, "replace": "", "extra_edits": [
                {"source": "src/rmg.cpp", "insert_before": anchor, "text": owned + "\n"}]})
    else:
        options.append({"name": "owning_tu_body_stores", "replace": assigned})
        options.append({"name": "split_source_negative_control", "replace": "", "extra_edits": [
            {"source": "src/rmg_support.cpp", "insert_before": "// The river painter deliberately inherits the generic line walker as its",
             "text": block + "\n"}]})
    payload = {"schema": 1, "source": owner, "units": ["rmg", "rmg_support"],
               "evidence": __doc__, "axes": [{"name": "position_constructor_owner", "find": block,
                                              "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("3 position-constructor ownership/store controls")


if __name__ == "__main__":
    main()
