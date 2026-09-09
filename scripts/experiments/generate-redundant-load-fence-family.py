"""Recheck the two byte-neutral Load fences jointly after main integration.

The full 226-region audit at d7f7be28 found each of these deletions emitted
the identical game object. Check their combination independently: deleting
two individually inert overrides can still change later inline decisions.
Keep the canonical creature-bank loader, early exits, final cleanup, and
the intervening isLocalHuman fence untouched. No C++ statement is rewritten.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/game.cpp").read_text()
    anchors = [
        ("creature-bank-load", "#pragma inline_depth(0)\n"
         "    loadObjectVector(infile, &m_creatureBanks);\n#pragma inline_depth()"),
        ("normal-exit", "    setupAdjacentMons();\n    aiExamineMap();\n\n"
         "#pragma inline_depth(0)\n    return 0;\n#pragma inline_depth()"),
    ]
    axes = []
    for name, old in anchors:
        if source.count(old) != 1:
            raise ValueError("Review the Load fence: " + name)
        new = old.replace("#pragma inline_depth(0)\n", "")
        new = new.replace("\n#pragma inline_depth()", "")
        axes.append(dict(name=name, find=old, options=[dict(name="existing-fence"),
                                                     dict(name="removed", replace=new)]))
    return dict(schema=1, source="src/game.cpp", units=["game"], evidence=__doc__, axes=axes)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
