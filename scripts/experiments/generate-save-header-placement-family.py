"""Compare save-header class placement within its canonical Game.h owner.

DC Game.h:1301/1312/1325/1344 supplies the four inline method boundaries.
Their class-defined form must follow the game layout used by reset/load.
Compare that with separate inline definitions at the same header position,
preserving signatures, field order, method bodies and all source calls.
Every current compiler dependency on game.h is included in the score vector.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "include/game.h").read_text()
    start = source.index("class SavedGameHeader {")
    end = source.index("SIZE(SavedGameHeader, 0x5a4);", start) + len("SIZE(SavedGameHeader, 0x5a4);")
    declaration = source[start:end]
    start = source.index("// Original: SavedGameHeader::SavedGameHeader;")
    definitions = source[start:source.index("// Original: game::is_human_ally;", start)]
    embedded = declaration
    for name, prototype in [("SavedGameHeader", "    SavedGameHeader();"),
                            ("reset", "    void reset();"),
                            ("save", "    int save(TAbstractFile* outfile);"),
                            ("load", "    int load(TAbstractFile* infile);")]:
        signature = re.search(r"inline (?:void |int )?SavedGameHeader::" + name + r"\([^\n]*\)\n\{", definitions)
        if signature is None or embedded.count(prototype) != 1:
            raise ValueError(f"Review the current {name} definition/declaration")
        comment = definitions.rfind("// Original:", 0, signature.start())
        end = definitions.index("\n}", signature.end()) + 2
        method = definitions[comment:end].replace("SavedGameHeader::" + name + "(", name + "(")
        embedded = embedded.replace(prototype, "\n".join(
            "    " + line if line else "" for line in method.splitlines()))
    units = []
    dependencies = subprocess.check_output(["ninja", "-t", "deps"], cwd=ROOT, text=True)
    for block in dependencies.split("\n\n"):
        if str(ROOT / "include/game.h") in block:
            match = re.match(r"build/objdiff/base/(.+)\.obj:", block)
            if match:
                units.append(match[1])
    if "game" not in units or "singleselectionwindow" not in units:
        raise ValueError("Run the full build to refresh compiler dependency records")
    options = [{"name": "unchanged"}]
    for name, replacement in [("class-after-game-out-of-class-methods", declaration + "\n\n" + definitions),
                              ("class-after-game-in-class-methods", embedded + "\n\n")]:
        options.append({"name": name, "replace": "", "extra_edits": [
            {"find": definitions, "replace": replacement}]})
    return {"schema": 1, "source": "include/game.h", "units": sorted(units), "evidence": __doc__,
            "axes": [{"name": "saved-header-class-definition", "find": declaration, "options": options}]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
