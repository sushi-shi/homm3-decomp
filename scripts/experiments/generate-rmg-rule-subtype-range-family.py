#!/usr/bin/env python3
"""Combine rule lifetimes with ownership of the retail reverse subtype cursor.

The Rule* reverse-cursor family recovers single-insert and the first vector
constructor decisions together, but adds a pointer load/induction absent from
retail. Retail instead decrements a subtype pointer and loads Rule*[match]
only after success. Put canonical reverse traversal on that consumed range.
Keep the signed match sentinel, last-duplicate precedence, and canonical
vector size/rbegin/end and reverse_iterator interfaces.
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
    if mode in (2, 3, 5):
        qualifier = "const " if mode in (3, 5) else ""
        declaration = f"                {qualifier}std::vector<int>& subtypeGroup = {group};\n"
        group = "subtypeGroup"
    iterator = "const_reverse_iterator" if mode in (3, 5) else "reverse_iterator"
    if mode in (4, 5):
        declaration += f"                std::vector<int>::{iterator} matchingSubtype({group}.end());"
    else:
        declaration += f"                std::vector<int>::{iterator} matchingSubtype = {group}.rbegin();"
    anchor = "                int match = rulesByType[mappedType][terrain].size();"
    body = parent.replace(original, anchor, anchor + "\n" + declaration)
    return parent.replace(body,
        "                while (match-- && subtypesByType[mappedType][terrain][match] != subtype)\n                    ;",
        "                while (match-- && *matchingSubtype != subtype)\n                    ++matchingSubtype;")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    retained = parent.parents(args.checkpoint)
    options = [dict(name=f"{row['id']}+subtype_range_{mode}", replace=variant(body, mode))
               for row, body in retained for mode in range(6)]
    if len({row["replace"] for row in options}) != 60:
        raise ValueError("expected sixty distinct source states")
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
        parent_checkpoint=str(args.checkpoint.resolve()),
        reproduced_parents=[dict(id=row["id"], choices=row["choices"], object_hash=row["object_hash"])
                            for row, _ in retained], axes=[dict(name="rule_subtype_range", source=parent.SOURCE,
        find=parent.definition((HOMM3_DIR / parent.SOURCE).read_text()), options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("60 coupled subtype-cursor and rule-lifetime states")


if __name__ == "__main__":
    main()
