"""Test natural ConvertVolume lifetimes while preserving its duplicated arms.

Retail 0x5996c0 matches the retained 151-byte ordinary body, but the existing
auto-inline fence controls its SetMusicVolume/ModifySample/MemorySample calls.
Dreamcast soundmgr.cpp:122 initializes the result, 125/136 gate the two
settings, 127/138 scale independently, and 146/151 clamp the shared result.
Its final 0..100 platform conversion is absent from Complete's 0..127 return.
No leading gap or positive inline clue supports an added verification.

Cross branch-local value snapshots, const references and direct setting reads
with short-circuit/nested range guards and the existing/removed fence. These
are source-lifetime hypotheses, not recovered DC locals (none are recorded).
There are no calls or writes between each setting's reads; all forms preserve
the two separately selected globals and the same arithmetic and clamp order.
Never factor away the duplicated source arms or add dummy compiler mass.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/soundmgr.cpp").read_text()
    start = source.index("#pragma auto_inline(off)\n")
    end = source.index("#pragma auto_inline(on)", start) + len("#pragma auto_inline(on)")
    original = source[start:end]
    if "int soundManager::convertVolume(" not in original:
        raise ValueError("Review the volume fence's owner")
    options = []
    for remove, lifetime, nested in itertools.product(range(2), range(3), range(2)):
        candidate = original
        for setting in ("g_unk698760", "g_unk698764"):
            old = """        int setting = @GLOBAL@;
        if (setting >= 1 && setting <= 10) {
            result = (setting + 1) * volumeValue / 10;
            if (result < 1)
                result = 1;
        }""".replace("@GLOBAL@", setting)
            new = old
            if nested:
                new = new.replace("        if (setting >= 1 && setting <= 10) {",
                                  "        if (setting >= 1) {\n            if (setting <= 10) {")
                new = new.replace("            result = (setting + 1) * volumeValue / 10;",
                                  "                result = (setting + 1) * volumeValue / 10;")
                new = new.replace("            if (result < 1)\n                result = 1;",
                                  "                if (result < 1)\n                    result = 1;")
                new = new[:-9] + "            }\n        }"
            if lifetime == 1:
                new = new.replace("int setting =", "const int& setting =")
            elif lifetime == 2:
                new = new.replace("        int setting = " + setting + ";\n", "")
                new = new.replace("setting", setting)
            if candidate.count(old) != 1:
                raise ValueError("Review the duplicated setting arm: " + setting)
            candidate = candidate.replace(old, new)
        if remove:
            candidate = candidate.replace("#pragma auto_inline(off)\n", "")
            candidate = candidate.replace("\n#pragma auto_inline(on)", "")
        option = dict(name=("unfenced" if remove else "existing-fence") + "-"
                      + ("value", "reference", "direct-read")[lifetime]
                      + ("-nested-bounds" if nested else "-and-bounds"))
        if candidate != original:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source="src/soundmgr.cpp", units=["soundmgr"], evidence=__doc__,
                axes=[dict(name="volume-value-boundary", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
