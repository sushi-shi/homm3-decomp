"""Test actual input/output lifetimes around the canonical primary-skill call.

Complete's skill quest has no matching Dreamcast class; retail 0x56dad0 is
the source of this bounded hypothesis. Its first loop reads one signed byte
from requiredSkills, compares the widened byte, and stores the selected low
byte through a separately advanced missing-skills address. The present
const int& binds a converted temporary, not the signed-byte member itself.
Keep hero::getPrimarySkill and all six existing fences. Vary only the actual
required-value binding, declaration lifetime, output address and the first
dialog loop's unsigned/signed index (retail jl proves the signed comparison).
No pasted clamp body, dummy mass, new pragma, false inline or raw storage.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/seerhut.cpp").read_text()
    signature = "void type_skill_quest::doProposalDialog(hero* currentHero)\n{\n"
    start = source.index(signature) + len(signature)
    end = source.index("\n\n    if (m_progressText.length() > 0)", start)
    original = source[start:end]
    if original.count("currentHero->getPrimarySkill(i)") != 1 or original.count("const int& required") != 1:
        raise ValueError("Review the actual accessor/required-skill input parent")
    options = [dict(name="unchanged-input-loop")]
    for required_type, destination, required_first in itertools.product(
            ("const int&", "const signed char&", "signed char"),
            ("indexed", "reference", "cursor"), (False, True)):
        statements = ["int have = currentHero->getPrimarySkill(i);",
                      required_type + " required = m_requiredSkills[i];"]
        if required_first:
            statements.reverse()
        if destination == "reference":
            statements.insert(0, "signed char& output = missing[i];")
            target = "output"
        elif destination == "cursor":
            target = "*output"
        else:
            target = "missing[i]"
        statements.append(target + " = required > have ? required : 0;")
        if destination == "cursor":
            statements.append("++output;")
        candidate = "    signed char missing[4];\n"
        if destination == "cursor":
            candidate += "    signed char* output = missing;\n"
        candidate += "    for (int i = 0; i < 4; ++i) {\n"
        candidate += "".join("        " + statement + "\n" for statement in statements)
        candidate += "    }"
        if required_type == "const int&" and destination == "indexed" and not required_first:
            # This differs only in a stale explanatory comment, not C++.
            continue
        options.append(dict(name=required_type.replace(" ", "-").replace("&", "-ref")
                            + "-" + destination + ("-required-first" if required_first else "-have-first"),
                            replace=candidate))
    unsigned = "for (unsigned int i = 0; i < 4; ++i) {"
    function_end = source.index("\n}\n", end)
    if source[start:function_end].count(unsigned) != 1:
        raise ValueError("Review the first dialog's guard index")
    # Restrict the anchor to the unique first resource loop, not other functions.
    index_anchor = "        type_dialog_resource resource;\n        " + unsigned
    if source.count(index_anchor) != 1:
        raise ValueError("Guard index anchor is no longer unique")
    return dict(schema=1, source="src/seerhut.cpp", units=["seerhut"], evidence=__doc__, axes=[
        dict(name="required-and-output-lifetimes", find=original, options=options),
        dict(name="dialog-index-signedness", find=index_anchor, options=[
            dict(name="unsigned-control"), dict(name="retail-signed", replace=index_anchor.replace("unsigned int", "int"))]),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
