"""Recheck the task-destructor fence and its derived COMDAT fold.

The complete deletion audit found all tracked scores unchanged but an untracked
CNewPlayerUpdateProc destructor changing from a five-byte jump to a 38-byte
body. Raw review identifies the new body as instruction/relocation-identical
to the exact retained CNewPlayerUpdateTask destructor at retail 0x583ef0.
Its manager/drop/window callers keep calling the derived symbol; retail calls
the retained teardown directly, suggesting that the fence prevents the natural
COMDAT fold rather than protecting a real distinct target.

Reproduce both states on the fresh integrated parent before judging that fold.
The genuine VC6 linker control subsequently REFUTED the fold: the ordinary
Task body is non-COMDAT, and even /OPT:ICF keeps both symbols at distinct
addresses. Removal is justified by the complete score/body review, not a
claimed linker repair; the generated duplicate adds 32 padded code bytes.
Keep the ordinary explicit task destructor, proven Complete inheritance and
novtable interface unchanged. DC's older CNewPlayerUpdateProc has only a
forward type record; it does not prove an implicit Complete task destructor.
The missing dead scalar-deleting-wrapper row remains a separate question.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/singleselectionwindow.cpp").read_text()
    begin = source.rfind("#pragma auto_inline(off)\n", 0,
                        source.index("CNewPlayerUpdateTask::~CNewPlayerUpdateTask()"))
    end = source.index("#pragma auto_inline(on)", begin) + len("#pragma auto_inline(on)")
    original = source[begin:end]
    if original.count("#pragma auto_inline(off)") != 1 or original.count("VA(0x00583ef0, 0x26)") != 1:
        raise ValueError("Review the task-destructor fence anchor")
    return dict(schema=1, source="src/singleselectionwindow.cpp", units=["singleselectionwindow"],
                evidence=__doc__, axes=[dict(name="task-destructor-fence", find=original, options=[
                    dict(name="existing-fence"),
                    dict(name="unfenced", replace=original.replace("#pragma auto_inline(off)\n", "")
                         .replace("\n#pragma auto_inline(on)", "")),
                ])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
