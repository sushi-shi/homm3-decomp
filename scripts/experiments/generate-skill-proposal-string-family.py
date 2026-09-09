"""Cross real returned-string lifetimes with paired skill-dialog fence removal.

The custom proposal string and formatted branch's requirement string are
never mutated. Both may own returned values or bind const references that
extend those values' lifetimes to the same scope exit. Retail calls _Tidy
at those exits, where the fully fenced parent calls string destructor
wrappers. Preserve the mutable formatted text, canonical accessor, exact
scope endpoints and every existing call. No new helper, allocator object,
assertion, fake inline or replacement override is introduced.

Use all eight paired ctor/insert/cleanup removals plus each isolated insert
and cleanup, crossed with the four actual string bindings: 48 states.
The preceding exhaustive 64-subset family bounds deletion alone; it is not
repeated as purported new evidence. Snapshot and reproduced control identity
are checked before authoring the follow-up.
"""

import argparse
import itertools
import json
from pathlib import Path

from homm3.vc6 import source_families as families


ROOT = Path(__file__).resolve().parents[2]


def make_manifest(parent):
    input_path = parent / "input.json"
    _, originals, axes = families.load_manifest(input_path, parent / "snapshot")
    current = (ROOT / "src/seerhut.cpp").read_text()
    if originals["src/seerhut.cpp"] != current:
        raise ValueError("Parent source changed; refresh the full-build controls")
    control = families.render(originals, axes, [0])
    control_id = families.digest(json.dumps(control, sort_keys=True).encode())[:24]
    records = json.loads((parent / "checkpoint.json").read_text())["records"]
    record = next(row for row in records if row["id"] == control_id)
    if not record.get("scores") or not (parent / "candidates" / control_id / "repeat/seerhut/candidate.obj").is_file():
        raise ValueError("Missing scored, reproduced parent control")
    start = current.index("void type_skill_quest::doProposalDialog(hero* currentHero)\n{")
    end = current.index("\n}\n", start) + 2
    original = current[start:end]
    lines = original.splitlines(keepends=True)
    regions, opened = [], None
    for index, line in enumerate(lines):
        if line.strip() == "#pragma inline_depth(0)":
            if opened is not None:
                raise ValueError("Nested fence")
            opened = index
        elif line.strip() == "#pragma inline_depth()":
            if opened is None:
                raise ValueError("Unpaired fence")
            regions.append((opened, index))
            opened = None
    if len(regions) != 6 or opened is not None:
        raise ValueError("Review the six-region parent")
    masks = [(mask & 1) * 9 + ((mask >> 1) & 1) * 18 + ((mask >> 2) & 1) * 36 for mask in range(8)]
    masks += [2, 16, 4, 32]
    options = []
    for custom_ref, requirement_ref, mask in itertools.product(range(2), range(2), masks):
        removed = {line for bit, pair in enumerate(regions) if mask & (1 << bit) for line in pair}
        candidate = "".join(line for index, line in enumerate(lines) if index not in removed)
        if custom_ref:
            candidate = candidate.replace("std::string text = getProposalDialogText();",
                                          "const std::string& text = getProposalDialogText();")
        if requirement_ref:
            candidate = candidate.replace("std::string requirement = skillRequirementText(missing);",
                                          "const std::string& requirement = skillRequirementText(missing);")
        option = dict(name=("custom-ref" if custom_ref else "custom-value")
                      + ("-requirement-ref" if requirement_ref else "-requirement-value")
                      + "-remove-mask-" + str(mask))
        if candidate != original:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source="src/seerhut.cpp", units=["seerhut"],
                evidence=__doc__ + "\nParent " + parent.name + ": " + control_id,
                axes=[dict(name="string-ownership-and-fences", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(args.parent.resolve()), indent=2) + "\n")
