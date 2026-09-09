"""Test whether the Conflux initializer was misbound as a generic constructor.

Retail 0x5d2d80 receives a sixteen-byte record in ECX and fills the four
elemental schools. Its three retained callers are Conflux-only AI branches;
townManager::doUniversity expands the same operation. Generic map university
records are different: RandomizeUniversity fills four selected skills with
no initializer call, and Load passes an uninitialized fill to opaque resize.
Those generic paths contradict automatic elemental-school initialization.

Keep one ordinary, explicitly called record initializer at the existing
townmgr boundary, restore the native map local, and compare void/pointer
return hypotheses with the public insertion APIs and the old fence control.
initializeMagicSkills is a PROVISIONAL behavioral name, not a recovered DC
symbol. The ECX input and EAX copy do not uniquely establish a historical
method name or constructor kind; scores alone cannot settle that ambiguity.
The old constructor label may score zero after this deliberate ABI-identity
change: inspect the retained thirty-byte body and all callers separately.
"""

import argparse
import importlib.util
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def replace(source, old, new, count=1):
    if source.count(old) != count:
        raise ValueError("Review initializer anchor: " + old)
    return source.replace(old, new)


def make_manifest():
    path = Path(__file__).with_name("generate-university-insertion-family.py")
    spec = importlib.util.spec_from_file_location("university_insertion_parent", path)
    parent = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parent)
    manifest = parent.make_manifest()
    axis = manifest["axes"][0]
    options = [dict(name="unchanged")]
    header = (ROOT / "include/game.h").read_text()
    ai = (ROOT / "src/philai.cpp").read_text()
    town = (ROOT / "src/townmgr.cpp").read_text()
    start = town.index("type_university::type_university()\n{")
    constructor = town[start:town.index("\n}\n", start) + 3]
    body = constructor[constructor.index("{\n"):]
    ai_calls = replace(ai, "type_university university;",
                       "type_university university;\n        university.initializeMagicSkills();", 3)
    town_calls = replace(town, "        type_university townUniversity;",
                         "        type_university townUniversity;\n        townUniversity.initializeMagicSkills();")
    for result in ("void", "pointer"):
        declaration = ("void" if result == "void" else "type_university*") + " initializeMagicSkills();"
        new_header = replace(header, "type_university();", declaration)
        definition = ("void" if result == "void" else "type_university*") + " type_university::initializeMagicSkills()\n" + body
        if result == "pointer":
            definition = replace(definition, "\n}", "\n    return this;\n}")
        new_town = replace(town_calls, constructor, definition)
        for index, candidate in enumerate(axis["options"]):
            # Native local, original pointer receiver, each API/fence pair.
            if not candidate["name"].endswith("typed-1-pointer"):
                continue
            option = json.loads(json.dumps(candidate))
            option["name"] = result + "-" + candidate["name"]
            option["extra_edits"].extend([
                dict(source="include/game.h", find=header, replace=new_header),
                dict(source="src/philai.cpp", find=ai, replace=ai_calls),
                dict(source="src/townmgr.cpp", find=town, replace=new_town)])
            options.append(option)
    axis["name"] = "university-initializer-ownership"
    axis["options"] = options
    manifest["units"] = ["game", "philai", "townmgr", "mapcell", "initialize"]
    manifest["evidence"] = __doc__
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
