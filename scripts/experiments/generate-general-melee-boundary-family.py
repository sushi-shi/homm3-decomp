"""Recover chooseMelee's canonical ordinary general-melee call.

DC ai_combat.cpp:1346 calls do_general_melee (0x2b948) on local_data with
local_enemy; both objects are live through the later value comparison and
reverse-order cleanup. Retail 0x4267c0 expands the helper but keeps its
getFinalMeleeValue/kill/inflictDamage calls. Its current fenced pasted body
is not a separate source helper. Keep the ordinary declaration, retained
0x4264d0 body and source order. Do not add inline, assertions or dummy mass.

Cross pasted-fenced, pasted-unfenced and canonical-unfenced boundaries with
four real helper scopes: separate/joint zero guard and function/branch ratio
lifetimes. Both float inputs are still read first, each ratio still rounds
to float, and no damage multiplication is reassociated. The other pasted
negative control is deliberately retained only in the experiment.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/ai_combat.cpp").read_text()
    start = source.index("void type_AI_combat_data::doGeneralMelee(type_AI_combat_data& defender)\n{")
    end = source.index("\n}\n", start) + 2
    original = source[start:end]
    guard = "    if (attacker == 0.0)\n        return;\n    if (target == 0.0)\n        return;"
    if original.count(guard) != 1 or original.count("    float ratio;\n") != 1:
        raise ValueError("Review the actual retained general-melee helper")
    helpers = []
    for joint_guard, branch_ratio in itertools.product(range(2), range(2)):
        candidate = original
        if joint_guard:
            candidate = candidate.replace(guard, "    if (attacker == 0.0 || target == 0.0)\n        return;")
        if branch_ratio:
            candidate = candidate.replace("    float ratio;\n", "").replace("        ratio = ", "        float ratio = ")
        option = dict(name=("joint-zero-guard" if joint_guard else "separate-zero-guards")
                      + ("-branch-ratios" if branch_ratio else "-shared-ratio"))
        if candidate != original:
            option["replace"] = candidate
        helpers.append(option)
    caller = source.index("bool type_AI_combat_data::chooseMelee(")
    pin_start = source.index("#pragma inline_depth(0)\n", caller)
    pin_end = source.index("#pragma inline_depth()", pin_start) + len("#pragma inline_depth()")
    pasted = source[pin_start:pin_end]
    if pasted.count("localData.getFinalMeleeValue()") != 1 or pasted.count("localEnemy.inflictDamage(") != 1:
        raise ValueError("Review the fenced copied general-melee call site")
    unpasted = "        localData.doGeneralMelee(localEnemy);"
    return dict(schema=1, source="src/ai_combat.cpp", units=["ai_combat"], evidence=__doc__, axes=[
        dict(name="retained-helper-scopes", find=original, options=helpers),
        dict(name="choose-melee-boundary", find=pasted, options=[
            dict(name="pasted-fenced-control"),
            dict(name="pasted-unfenced-negative", replace=pasted.replace("#pragma inline_depth(0)\n", "")
                 .replace("\n#pragma inline_depth()", "")),
            dict(name="canonical-ordinary-call", replace=unpasted)]),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
