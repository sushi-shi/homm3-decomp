#!/usr/bin/env python3
"""Test the shared counted subtype range, coupled to rule construction.

Retail derives the reverse subtype cursor from its begin pointer and the
Rule* group's count after the nonempty guard. The rbegin family instead loads
the subtype end pointer eagerly. Use the same canonical count and consumed
iterator range as retail, preserving short-circuited empty handling and the
final Rule*[match] lookup. No vector implementation or wrapper is copied.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

parent = generator("generate-rmg-rule-ranges-family.py")


def variant(original, mode):
    if mode == 0:
        return original
    group = "subtypesByType[mappedType][terrain]"
    declaration = ""
    if mode in (2, 4):
        declaration = f"                const std::vector<int>& subtypeGroup = {group};\n"
        group = "subtypeGroup"
    iterator = {1: "reverse_iterator", 2: "const_reverse_iterator", 3: "iterator",
                4: "const_iterator", 5: "iterator"}[mode]
    declaration += f"                std::vector<int>::{iterator} matchingSubtype({group}.begin() + match);"
    anchor = "                int match = rulesByType[mappedType][terrain].size();"
    if mode in (1, 2):
        loop = ("                while (match-- && *matchingSubtype != subtype)\n"
                "                    ++matchingSubtype;")
    elif mode in (3, 4):
        loop = ("                while (match-- && *--matchingSubtype != subtype)\n"
                "                    ;")
    else:
        loop = ("                while (match--) {\n"
                "                    if (*--matchingSubtype == subtype)\n"
                "                        break;\n"
                "                }")
    old = anchor + "\n                while (match-- && subtypesByType[mappedType][terrain][match] != subtype)\n                    ;"
    # VC6's iterator is a raw pointer. Do not form begin()+0 when the
    # empty default vector's begin pointer is null. Retail guards its
    # begin load and cursor calculation with the same nonempty condition.
    inner = "\n".join("    " + line for line in (declaration + "\n" + loop).splitlines())
    return parent.replace(original, old,
        anchor + "\n                if (match != 0) {\n" + inner +
        "\n                } else {\n                    --match;\n                }")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    retained = parent.parents(args.checkpoint)
    options = [dict(name=f"{row['id']}+counted_range_{mode}", replace=variant(body, mode))
               for row, body in retained for mode in range(6)]
    if len({row["replace"] for row in options}) != 60:
        raise ValueError("expected sixty distinct source states")
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
        parent_checkpoint=str(args.checkpoint.resolve()),
        reproduced_parents=[dict(id=row["id"], choices=row["choices"], object_hash=row["object_hash"])
                            for row, _ in retained], axes=[dict(name="rule_counted_range", source=parent.SOURCE,
        find=parent.definition((HOMM3_DIR / parent.SOURCE).read_text()), options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("60 coupled counted subtype-range and rule-lifetime states")


if __name__ == "__main__":
    main()
