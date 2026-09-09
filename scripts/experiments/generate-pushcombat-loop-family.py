"""Separate equivalent midpoint/termination expressions in the DC loop scope.

The complete PushCombatPoint source recovery removes the insert fence and
scores 99.7819%; the earlier while-loop form is exact. Keep the positive
DC midpoint-before-break scope, varying commutative addition and the signed
termination relation rather than deleting a proven source operation.
No helper, artificial work, cast or inline annotation is introduced.

Historical pre-adoption control: frozen context 34bab0471386d66e7211.
"""

import argparse
import importlib.util
import itertools
import json
from pathlib import Path


def make_manifest():
    path = Path(__file__).with_name("generate-pushcombat-boundary-family.py")
    spec = importlib.util.spec_from_file_location("pushcombat", path)
    parent = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parent)
    manifest = parent.make_manifest()
    axis = manifest["axes"][0]
    recovered = axis["options"][63]["replace"]
    options = [dict(name="unchanged"),
               dict(name="recovered-earlier-loop", replace=axis["options"][47]["replace"])]
    for addition, guard in itertools.product(
            ("first + last", "last + first"),
            ("first >= last", "last <= first", "!(last > first)", "!(first < last)")):
        candidate = parent.replace(recovered, "middle = (first + last) / 2;",
                                   "middle = (" + addition + ") / 2;")
        candidate = parent.replace(candidate, "if (first >= last)", "if (" + guard + ")")
        options.append(dict(name=addition + "; " + guard, replace=candidate))
    manifest["evidence"] = __doc__
    axis["name"] = "midpoint-and-termination"
    axis["options"] = options
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
