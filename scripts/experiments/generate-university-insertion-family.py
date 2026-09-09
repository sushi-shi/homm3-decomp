"""Recheck the university record through the complete public insertion chain.

DC randomize_university (0xac048, game.cpp:4770/4797) declares the native
record and calls push_back. Pinned Dinkumware implements push_back through
single-element insert, then count-insert; retail retains the count child.
The older eight-state family omitted single-element insert and the native
member/reference receiver forms. Test those boundaries after typed vector
helper recovery, keeping the current class/constructor declaration fixed.
The extra Complete constructor call for a native local remains a separate
type-ownership question, not a license for a fake no-initialization overload.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/game.cpp"


def replace(source, old, new):
    if source.count(old) != 1:
        raise ValueError("Review university source anchor: " + old)
    return source.replace(old, new)


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    start = source.index("VA(0x004c06f0,")
    original = source[start:source.index("// E:\\gamedcs\\game.cpp:4816", start)]
    start = source.index("union TUniversitySkillsPointerAlias {\n")
    alias = source[start:source.index("// E:\\gamedcs\\game.cpp:4770\n", start)]
    start = original.index("    std::vector<type_university>* universityList = &m_universities;")
    tail = original[start:original.index("\n}\n", start)]
    options = []
    for api, unpin, typed, receiver in itertools.product(
            ("count", "single", "push"), range(2), range(2),
            ("pointer", "reference", "member")):
        candidate = original
        if typed:
            candidate = replace(candidate, "    int university[4];",
                                "    type_university university;")
            candidate = replace(candidate, "        university[i] = skill;",
                                "        university.m_skills[i] = skill;")
        if receiver == "pointer":
            lines = ["    std::vector<type_university>* universityList = &m_universities;"]
            vector = "universityList->"
        elif receiver == "reference":
            lines = ["    std::vector<type_university>& universityList = m_universities;"]
            vector = "universityList."
        else:
            lines = []
            vector = "m_universities."
        lines.extend([
            "    unsigned long universityIndex = " + vector + "size() & 0xfff;",
            "    cell->m_extraInfo = (cell->m_extraInfo & ~universityIndexBits)",
            "        | (universityIndex << 13);"])
        if api != "push":
            lines.append("    type_university* universityTail = " + vector + "end();")
        if not typed:
            lines.extend(["    type_university* universityRecord =",
                          "        universitySkillsRecord(university);"])
        record = "university" if typed else "*universityRecord"
        if not unpin:
            lines.append("#pragma inline_depth(0)")
        arguments = {"count": "insert(universityTail, 1, " + record + ");",
                     "single": "insert(universityTail, " + record + ");",
                     "push": "push_back(" + record + ");"}
        lines.append("    " + vector + arguments[api])
        if not unpin:
            lines.append("#pragma inline_depth()")
        candidate = replace(candidate, tail, "\n".join(lines))
        option = dict(name=f"{api}-unpin-{unpin}-typed-{typed}-{receiver}")
        if candidate != original:
            option["replace"] = candidate
        if typed:
            option["extra_edits"] = [dict(find=alias, replace="")]
        options.append(option)
    return dict(schema=1, source=SOURCE, units=["game"], evidence=__doc__,
                axes=[dict(name="university-insertion-chain", find=original,
                           options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
