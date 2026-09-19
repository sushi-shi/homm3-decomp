#!/usr/bin/env python3
"""Holista's coordinated rule-reader range/ownership frontier.

The byte-verified inliner trace puts the unwanted single-insert expansion
at budget65/cost64 while the earlier required vector ctor is budget53/cost51.
Cross ten reproduced rule-construction/parsed-lifetime parents with real
range/traversal ownership. Keep canonical size/constructor/insert boundaries:
no copied vector implementation, redundant checks or unused iterator calls.
The matrix passes and final reverse-precedence binding all consume the ranges.
"""
import argparse
import hashlib
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "TRmgGeneratorBase::readObjectPlacementRules"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def replace(body, old, new):
    if body.count(old) != 1:
        raise ValueError("review range anchor: " + old)
    return body.replace(old, new)


def variant(original, mode):
    body = original
    count = "    int ruleCount = m_placementRules.size();\n"
    loop = "    for (row = 3; row < ruleCount + 3; ++row) {"
    record = "        TRmgObjectPlacementRule& rule = m_placementRules[row - 3];"
    iterator = "std::vector<TRmgObjectPlacementRule>::iterator "
    if mode in (1, 2, 4):
        declaration = "    " + iterator + "matrixRule = m_placementRules.begin();\n"
        if mode == 2:
            declaration = ("    " + iterator + "firstRule = m_placementRules.begin();\n"
                           "    " + iterator + "matrixRule = firstRule;\n")
        elif mode == 4:
            declaration += "    " + iterator + "lastRule = m_placementRules.end();\n"
        body = replace(body, count, count + declaration)
        replacement = "    for (row = 3; row < ruleCount + 3; ++row, ++matrixRule) {"
        if mode == 4:
            replacement = "    for (row = 3; matrixRule != lastRule; ++row, ++matrixRule) {"
        body = replace(body, loop, replacement)
        body = replace(body, record, "        TRmgObjectPlacementRule& rule = *matrixRule;")
        if mode == 2:
            later = "    for (int index = 0; index < ruleCount; ++index) {\n        TRmgObjectPlacementRule* rule = &m_placementRules[index];"
            body = replace(body, later,
                "    " + iterator + "bindingRule = firstRule;\n"
                "    for (int index = 0; index < ruleCount; ++index, ++bindingRule) {\n"
                "        TRmgObjectPlacementRule* rule = &*bindingRule;")
    elif mode == 3:
        # The first row reader deliberately remains the parent form.
        before, after = body.split(count)
        after = replace(after, "        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);",
            "        TSpreadsheetResource::TStringVector::const_iterator values = sheet->getRow(row).begin();")
        body = before + count + after
    elif mode == 5:
        match = "                int match = rulesByType[mappedType][terrain].size();"
        body = replace(body, match,
            "                const std::vector<TRmgObjectPlacementRule*>& group = rulesByType[mappedType][terrain];\n"
            "                int match = group.size();\n"
            "                std::vector<TRmgObjectPlacementRule*>::const_reverse_iterator matchingRule = group.rbegin();")
        body = replace(body,
            "                while (match-- && subtypesByType[mappedType][terrain][match] != subtype)\n                    ;",
            "                while (match-- && subtypesByType[mappedType][terrain][match] != subtype)\n                    ++matchingRule;")
        body = replace(body, "properties->m_placementRule = rulesByType[mappedType][terrain][match];",
            "properties->m_placementRule = *matchingRule;")
    return body


def parents(checkpoint):
    directory = checkpoint.parent
    source = (HOMM3_DIR / SOURCE).read_text()
    if (directory / "snapshot" / SOURCE).read_text() != source:
        raise ValueError("parent source changed; reproduce against current source first")
    saved = directory / "snapshot/include"
    current = HOMM3_DIR / "include"
    expected = {p.relative_to(saved): p.read_bytes() for p in saved.rglob("*") if p.is_file()}
    actual = {p.relative_to(current): p.read_bytes() for p in current.rglob("*") if p.is_file()}
    if expected != actual:
        raise ValueError("parent header population/content changed")
    manifest = json.loads((directory / "input.json").read_text())
    if manifest["axes"][0]["name"] != "rule_construction_lifetime":
        raise ValueError("expected rule-construction/lifetime parent")
    result = json.loads(checkpoint.read_text())
    selected = sorted(result["elites"], key=lambda row: row["choices"] != [0])
    if len(selected) != 10 or selected[0]["choices"] != [0]:
        raise ValueError("ten reproduced parents including unchanged control required")
    retained = []
    for row in selected:
        trial = directory / "candidates" / row["id"]
        repeated = json.loads((trial / "repeat/result.json").read_text())
        if any(repeated[key] != row[key] for key in ("choices", "scores", "object_hash", "source_hashes")):
            raise ValueError("parent did not reproduce")
        text = (trial / "first/tree" / SOURCE).read_text()
        replacement = manifest["axes"][0]["options"][row["choices"][0]]["replace"]
        rendered = source.replace(manifest["axes"][0]["find"], replacement)
        if text != rendered or hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("parent source/manifest identity changed")
        retained.append((row, definition(text)))
    return retained


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    retained = parents(args.checkpoint)
    options = [dict(name=f"{row['id']}+range_{mode}", replace=variant(body, mode))
               for row, body in retained for mode in range(6)]
    if len({row["replace"] for row in options}) != 60:
        raise ValueError("expected sixty meaningful distinct source states")
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
        parent_checkpoint=str(args.checkpoint.resolve()),
        reproduced_parents=[dict(id=row["id"], choices=row["choices"], object_hash=row["object_hash"])
                            for row, _ in retained], axes=[dict(name="rule_range_ownership", source=SOURCE,
        find=definition((HOMM3_DIR / SOURCE).read_text()), options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("60 coupled rule-range and construction/lifetime states")


if __name__ == "__main__":
    main()
