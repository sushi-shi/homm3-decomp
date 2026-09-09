"""Test real global-handle bindings in the ordinary StopMouseThread helper.

Retail 0x577810 proves the thread guard, signal/wait/close order, each global
reload and both zero stores before the final mouse-pointer restore. The older
DC 0x12fdd4 retains only the final pointer restore; its PC line gap is not
assertion evidence. Retail setupScenarioOptions 0x580a70 calls this helper,
while retail-only generateRandomMap 0x5860e0 expands it after the request,
progress and path objects end their shared scope.

All options preserve the ordinary helper and both callers. A reference binds
the actual global handle storage, not a copied handle across opaque API calls:
every later read observes the same storage as the direct spelling. Mutable
references carry the zero store too; const references leave that store on the
global. Cross the thread/event bindings and equivalent guard exit with removal
of the existing fence. No assertion, fake helper or additional work is added.
These are lifetime hypotheses, not recovered DC local declarations.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/singleselectionwindow.cpp").read_text()
    begin = source.rfind("#pragma auto_inline(off)\n", 0, source.index("void stopMouseThread()\n{"))
    end = source.index("#pragma auto_inline(on)", begin) + len("#pragma auto_inline(on)")
    original = source[begin:end]
    start_body = original.index("void stopMouseThread()\n{")
    body = original[start_body:original.index("\n}", start_body) + 2]
    statements = """SetEvent(g_endMouseThreadEvent);
WaitForSingleObject(g_mouseThread, INFINITE);
CloseHandle(g_mouseThread);
g_mouseThread = 0;
CloseHandle(g_endMouseThreadEvent);
g_endMouseThreadEvent = 0;
g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);"""
    options = []
    for thread, event, early, unfenced in itertools.product(range(3), range(3), range(2), range(2)):
        guard = "g_mouseThread"
        before_guard = []
        block = statements
        if thread:
            guard = "thread"
            before_guard.append(("HANDLE&" if thread == 1 else "HANDLE const&") + " thread = g_mouseThread;")
            block = block.replace("g_mouseThread", "thread")
            if thread == 2:
                block = block.replace("thread = 0;", "g_mouseThread = 0;")
        if event:
            block = block.replace("g_endMouseThreadEvent", "event")
            if event == 2:
                block = block.replace("event = 0;", "g_endMouseThreadEvent = 0;")
            block = (("HANDLE&" if event == 1 else "HANDLE const&")
                     + " event = g_endMouseThreadEvent;\n" + block)
        lines = ["void stopMouseThread()", "{"]
        lines.extend("    " + line for line in before_guard)
        if early:
            lines.extend(["    if (!" + guard + ")", "        return;"])
            lines.extend("    " + line for line in block.splitlines())
        else:
            lines.append("    if (" + guard + ") {")
            lines.extend("        " + line for line in block.splitlines())
            lines.append("    }")
        lines.append("}")
        candidate = original.replace(body, "\n".join(lines))
        if unfenced:
            candidate = candidate.replace("#pragma auto_inline(off)\n", "").replace("\n#pragma auto_inline(on)", "")
        label = ("direct", "mutable-reference", "const-reference")
        option = dict(name="-".join(("thread-" + label[thread], "event-" + label[event],
                                    "early-return" if early else "guarded-block",
                                    "unfenced" if unfenced else "existing-fence")))
        if candidate != original:
            option["replace"] = candidate
        if not options and candidate != original:
            raise ValueError("Review the unchanged-source teardown control")
        options.append(option)
    return dict(schema=1, source="src/singleselectionwindow.cpp", units=["singleselectionwindow"],
                evidence=__doc__, axes=[dict(name="stop-mouse-lifetimes", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
