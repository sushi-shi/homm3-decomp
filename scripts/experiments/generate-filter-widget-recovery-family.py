"""Recover the desktop filter builder using real pointer and loop lifetimes.

The retail-only 0x57d170 builder creates widgets 0x118..0x14f in order.
Its array loops set the highlight field directly and call the proven
button::setDisabledFrame helper. Preserve both operations and every
constructor argument. Compare real allocation-result lifetimes, loop-local
button bindings, public vector append APIs and index scopes. Never restore
the former unproven highlight setter or an inline fence.
"""
import argparse
import itertools
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
SIGNATURE = "void TSingleSelectionWindow::createFilterWidgets()"
GROUPS = (("CountA", 9), ("CountB", 9), ("CountC", 9), ("CountD", 8),
          ("Water", 4), ("Strength", 4))


def original_body():
    source = (ROOT / "src/singleselectionwindow.cpp").read_text()
    start = source.index(SIGNATURE + "\n{")
    return source[start:source.index("\n}", start) + 2]


def variants(body):
    allocations = list(re.finditer(r"    m_widgets\.push_back\((new (?:textWidget|button)\([\s\S]*?\))\);", body))
    if len(allocations) != 13:
        raise ValueError("Review standalone filter widget allocation sites")
    bodies = []
    for lifetime, binding, api, scoped in itertools.product(range(3), range(3), range(3), range(2)):
        candidate = body
        if lifetime:
            for allocation in reversed(allocations):
                expression = allocation.group(1)
                if lifetime == 1:
                    replacement = "    {\n        widget* created = " + expression + ";\n        m_widgets.push_back(created);\n    }"
                else:
                    replacement = "    created = " + expression + ";\n    m_widgets.push_back(created);"
                candidate = candidate[:allocation.start()] + replacement + candidate[allocation.end():]
            if lifetime == 2:
                candidate = candidate.replace(SIGNATURE + "\n{\n", SIGNATURE + "\n{\n    widget* created;\n", 1)
        for group, count in GROUPS:
            member = "m_filter" + group + "Buttons"
            old = f"    for (i = 0; i < {count}; ++i) {{\n        {member}[i]->m_highlightedFrame = 2;\n        {member}[i]->setDisabledFrame(1);\n        m_widgets.push_back({member}[i]);\n    }}"
            if candidate.count(old) != 1:
                raise ValueError("Review filter loop " + group)
            receiver, value, declaration = member + "[i]->", member + "[i]", ""
            if binding == 1:
                declaration = "        button* current = " + member + "[i];\n"
                receiver, value = "current->", "current"
            elif binding == 2:
                declaration = "        button& current = *" + member + "[i];\n"
                receiver, value = "current.", "&current"
            append = [f"m_widgets.push_back({value});", f"m_widgets.insert(m_widgets.end(), {value});",
                      f"m_widgets.insert(m_widgets.end(), 1, {value});"][api]
            new = f"    for (i = 0; i < {count}; ++i) {{\n" + declaration
            new += f"        {receiver}m_highlightedFrame = 2;\n        {receiver}setDisabledFrame(1);\n        {append}\n    }}"
            if scoped:
                new = "    {\n        int i;\n" + "\n".join("    " + line for line in new.splitlines()) + "\n    }"
            candidate = candidate.replace(old, new)
        if scoped:
            candidate = candidate.replace("    int i;\n    {", "    {", 1)
        bodies.append((f"allocation-{lifetime}-binding-{binding}-append-{api}-scope-{scoped}", candidate))
    return bodies


def make_manifest():
    body = original_body()
    choices = variants(body)
    if choices[0][1] != body:
        raise ValueError("The source no longer matches the original control")
    return {"schema": 1, "source": "src/singleselectionwindow.cpp", "units": ["singleselectionwindow"],
            "evidence": __doc__, "axes": [{"name": "filter-widget-lifetimes", "find": body,
            "options": [{"name": "unchanged"}] + [{"name": name, "replace": text} for name, text in choices[1:]]}]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
