"""Cross canonical adventure cell calls with the recovered validity signature.

Dreamcast's is_valid S_PUB32 is ?is_valid@type_point@@QBA_NXZ: a const
bool member, defined in findpath.cpp:36, not an inline header body. Its
retail 0x4b1330 checks and byte return agree. The two corrected signatures
separate const qualification from the return type; no body is duplicated.
The old mangled claim can score as missing until a normal build regenerates
source-owned labels, so inspect the renamed function's actual bytes too.

The parent drawing family preserves the proven GetCell calls and ordinary
helper definition. This smaller follow-up keeps either the complete old
drawing boundary or all canonical cell/boat/location calls. It does not
move is_valid into a header just to make callers inline it.

Historical pre-adoption control: use e4650642 or the frozen input snapshot
2675b9280570a49850e1, not the subsequently corrected signatures/helpers.
"""

import argparse
import importlib.util
import json
from pathlib import Path


def make_manifest():
    parent = Path(__file__).with_name("generate-adventure-cell-call-family.py")
    spec = importlib.util.spec_from_file_location("cell_calls", parent)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    result = module.make_manifest()
    result["units"] = ["advmgr", "findpath"]
    result["evidence"] = __doc__
    drawing = result["axes"][1]
    drawing["options"] = [drawing["options"][0], drawing["options"][-1]]
    result["axes"].append(dict(
        name="validity-signature", source="include/struct.h",
        find="    unsigned char isValid();", options=[
            dict(name="unchanged"),
            dict(name="const-byte", replace="    unsigned char isValid() const;",
                 extra_edits=[dict(source="src/findpath.cpp",
                     find="unsigned char type_point::isValid()",
                     replace="unsigned char type_point::isValid() const")]),
            dict(name="const-bool", replace="    bool isValid() const;",
                 extra_edits=[dict(source="src/findpath.cpp",
                     find="unsigned char type_point::isValid()",
                     replace="bool type_point::isValid() const")])]))
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
