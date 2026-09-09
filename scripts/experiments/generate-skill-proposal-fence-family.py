"""Exhaust the six existing skill-proposal fences jointly, without new code.

Retail 0x56dad0 calls both vector constructors, inserts and destructors but
expands the string destructors down to _Tidy. Individual deletion is not a
joint-inlining verdict. This finite 64-state family changes only the six
existing directive pairs; preserve the canonical primary-skill accessor and
all declarations, operations and lifetimes. No replacement override is added.
The class is Complete-specific; do not infer its scopes from the older DC
TSeerHut proposal body.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/seerhut.cpp").read_text()
    start = source.index("void type_skill_quest::doProposalDialog(hero* currentHero)\n{")
    end = source.index("\n}\n", start) + 2
    original = source[start:end]
    lines = original.splitlines(keepends=True)
    regions, opened = [], None
    for index, line in enumerate(lines):
        if line.strip() == "#pragma inline_depth(0)":
            if opened is not None:
                raise ValueError("Unexpected nested fence")
            opened = index
        elif line.strip() == "#pragma inline_depth()":
            if opened is None:
                raise ValueError("Unpaired fence")
            regions.append((opened, index))
            opened = None
    if len(regions) != 6 or opened is not None:
        raise ValueError("Review the six-region parent")
    names = ("custom-vector-ctor", "custom-insert", "custom-cleanup",
             "formatted-vector-ctor", "formatted-insert", "formatted-cleanup")
    options = [dict(name="unchanged-six-fences")]
    for mask in range(1, 64):
        removed = {line for bit, pair in enumerate(regions) if mask & (1 << bit) for line in pair}
        candidate = "".join(line for index, line in enumerate(lines) if index not in removed)
        options.append(dict(name="remove-" + "+".join(name for bit, name in enumerate(names)
                                                     if mask & (1 << bit)), replace=candidate))
    return dict(schema=1, source="src/seerhut.cpp", units=["seerhut"], evidence=__doc__,
                axes=[dict(name="joint-fence-subset", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
