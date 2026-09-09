"""Test save-local types and lifetimes without changing canonical helpers.

DC game::Save (0xa8cd0) names char_buffer, ushort_buffer, int MapExtraSize
and int i. Retail has narrow stream writes and reused small stack slots;
the retained vector writer returns native bool. Test genuine local reuse,
types and declaration lifetimes while preserving all calls and existing
pin locations. No family changes the SavedGameHeader ownership or bodies.
"""
import argparse
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/game.cpp").read_text()
    start = source.index("int game::save(TAbstractFile* outfile)\n{")
    body = source[start:source.index("\n#if 0  // @carcass", start)]
    anchors = ("    char byteValue;\n", "    char charBuffer;\n",
               "    short extraShortValue;", "    unsigned int mapExtraBytes =",
               "    int i;\n", "    SavedGameHeader saved;", "        unsigned char lithSaved;")
    if any(body.count(anchor) != 1 for anchor in anchors):
        raise ValueError("Review the save-local family against the new baseline")
    options = [{"name": "unchanged"}]
    for chars, word, size, counter, result in itertools.product(range(3), *([range(2)] * 4)):
        if (chars, word, size, counter, result) == (0, 0, 0, 0, 0):
            continue
        candidate = body
        if chars == 1:
            candidate = candidate.replace("    char byteValue;\n", "").replace("byteValue", "charBuffer")
        elif chars == 2:
            candidate = candidate.replace("    char charBuffer;\n", "").replace(
                "        charBuffer = g_unnamed69950c;", "        char charBuffer = g_unnamed69950c;")
        if word:
            candidate = candidate.replace("    short extraShortValue;", "    unsigned short extraShortValue;")
        if size:
            candidate = candidate.replace("    unsigned int mapExtraBytes =", "    int mapExtraBytes =")
        if counter:
            candidate = candidate.replace("    int i;\n", "").replace(
                "    SavedGameHeader saved;", "    int i;\n    SavedGameHeader saved;")
        if result:
            candidate = candidate.replace("        unsigned char lithSaved;", "        bool lithSaved;")
        options.append({"name": f"char-{chars}-uword-{word}-int-size-{size}-early-i-{counter}-bool-{result}",
                        "replace": candidate})
    return {"schema": 1, "source": "src/game.cpp", "units": ["game"], "evidence": __doc__,
            "axes": [{"name": "save-local-types-and-lifetimes", "find": body, "options": options}]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
