"""Recover RandomizeUniversity's record and public vector insertion boundary.

DC game.cpp:4770 names a type_university local, and 4797 calls push_back.
Complete still writes four skill enums and calls the vector insertion copy.
Its separate default constructor belongs to townmgr.cpp, so a typed local
must use that existing declaration; do not fabricate an inline/no-init ctor.

Cross the record local, push_back versus the current flattened insert, and
deletion of the existing inline-depth fence. Score every game function.
The local change is allowed to expose the absent retail constructor call
as a documented residual, not an excuse to duplicate constructor bodies.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/game.cpp"
START = "union TUniversitySkillsPointerAlias {\n"
END = "// E:\\gamedcs\\game.cpp:4770\n"
RECORD = """    type_university* universityRecord =
        universitySkillsRecord(university);
"""
TAIL = "    type_university* universityTail = universityList->end();\n"
INSERT = "    universityList->insert(universityTail, 1, *universityRecord);"
BLOCK = TAIL + RECORD + "#pragma inline_depth(0)\n" + INSERT + "\n#pragma inline_depth()"


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    start = source.index(START)
    helpers = source[start:source.index(END, start)]
    options = []
    for api in range(2):
        for unpin in range(2):
            for typed in range(2):
                lines = ""
                if not api:
                    lines += TAIL
                if not typed:
                    lines += RECORD
                record = "university" if typed else "*universityRecord"
                if not unpin:
                    lines += "#pragma inline_depth(0)\n"
                if api:
                    lines += "    universityList->push_back(" + record + ");"
                else:
                    lines += "    universityList->insert(universityTail, 1, " + record + ");"
                if not unpin:
                    lines += "\n#pragma inline_depth()"
                option = dict(name=f"push-back-{api}-unpin-{unpin}-typed-{typed}")
                if lines != BLOCK:
                    option["replace"] = lines
                if typed:
                    option["extra_edits"] = [
                        dict(find="    int university[4];", replace="    type_university university;"),
                        dict(find="        university[i] = skill;",
                             replace="        university.m_skills[i] = skill;"),
                        dict(find=helpers, replace="")]
                options.append(option)
    return dict(schema=1, source=SOURCE, units=["game"], evidence=__doc__,
                axes=[dict(name="record-and-insertion-boundary", find=BLOCK, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
