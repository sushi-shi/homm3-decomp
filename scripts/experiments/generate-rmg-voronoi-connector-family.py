#!/usr/bin/env python3
"""Shared, ordinary diagonal connector in two Voronoi callers.

The constructor's fifth edge and addSite's fan iterations have the same
createEdge(first.twin.site, first.twin.zone, second.site, second.zone),
splice(first.twin.previous), twin.splice(second) sequence. Both retail sites
retain createEdge; the constructor expands the first splice, while the fan
retains both. Test a provisional shared member boundary, not a false inline
declaration, and preserve createEdge/splice themselves. No DC counterpart.
"""
import argparse
import copy
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

DECLARATION = "    TRmgBoundaryVertex* connectEdges(TRmgBoundaryVertex* first, TRmgBoundaryVertex* second);\n"
SOURCE = "src/rmg_support.cpp"
HEADER = "include/rmg.h"


def connector(binding, result=0, previous=0, twin_result=0):
    lines = []
    if binding == 0:
        lines += ["TRmgBoundaryVertex* result = createEdge(first->m_twin->m_sitePosition,",
                  "    first->m_twin->m_zone, second->m_sitePosition, second->m_zone);"]
    elif binding == 1:
        lines += ["TRmgBoundaryVertex* twin = first->m_twin;",
                  "TRmgBoundaryVertex* result = createEdge(twin->m_sitePosition,",
                  "    twin->m_zone, second->m_sitePosition, second->m_zone);"]
    else:
        kind = "TPoint" if binding == 2 else "const TPoint&"
        lines += [f"{kind} firstSite = first->m_twin->m_sitePosition;",
                  f"{kind} secondSite = second->m_sitePosition;",
                  "TRmgBoundaryVertex* result = createEdge(firstSite, first->m_twin->m_zone,",
                  "    secondSite, second->m_zone);"]
    if result == 1:
        at = next(index for index, line in enumerate(lines) if "TRmgBoundaryVertex* result =" in line)
        lines[at] = lines[at].replace("TRmgBoundaryVertex* result =", "result =")
        lines.insert(at, "TRmgBoundaryVertex* result;")
    elif result == 2:
        lines = [line.replace("TRmgBoundaryVertex* result = createEdge", "TRmgBoundaryVertex& result = *createEdge") for line in lines]
    access = "." if result == 2 else "->"
    predecessor = "first->m_twin->m_previous"
    if previous:
        lines += [f"TRmgBoundaryVertex* previous = {predecessor};"]
        predecessor = "previous"
    lines += [f"result{access}splice({predecessor});"]
    if twin_result:
        lines += [f"TRmgBoundaryVertex* resultTwin = result{access}m_twin;",
                  "resultTwin->splice(second);"]
    else:
        lines += [f"result{access}m_twin->splice(second);"]
    lines += ["return &result;" if result == 2 else "return result;"]
    return ("TRmgBoundaryVertex* TRmgVoronoi::connectEdges(\n"
            "    TRmgBoundaryVertex* first, TRmgBoundaryVertex* second)\n{\n"
            + "\n".join("    " + line for line in lines) + "\n}")


def helper_options(source):
    helper = generator("generate-rmg-position-family.py")
    constructor = helper.definition(source, "TRmgVoronoi::TRmgVoronoi")
    insertion = helper.definition(source, "TRmgVoronoi::addSite")
    factory = helper.definition(source, "TRmgVoronoi::createEdge")
    start = constructor.index("    TRmgBoundaryVertex* diagonal =")
    end = constructor.index("    m_root = firstEdge;", start)
    new_constructor = constructor[:start] + "    connectEdges(fourthEdge, thirdEdge);\n" + constructor[end:]
    start = insertion.index("        base = createEdge(")
    end = insertion.index("        edge = base->m_previous;", start)
    new_insertion = insertion[:start] + "        base = connectEdges(edge, oppositeBase);\n" + insertion[end:]
    for binding, placement, loop, opposite in itertools.product(range(4), range(3), range(2), range(2)):
        body = connector(binding)
        ctor, site = new_constructor, new_insertion
        if loop:
            site = site.replace("    do {", "    for (;;) {").replace(
                "    } while (edge->m_twin->m_previous != m_root);",
                "        if (edge->m_twin->m_previous == m_root)\n            break;\n    }")
        if opposite:
            site = site.replace("TRmgBoundaryVertex* oppositeBase = base->m_twin;",
                                "TRmgBoundaryVertex* const& oppositeBase = base->m_twin;")
        edits = [dict(source=HEADER, insert_before="    void removeEdge(TRmgBoundaryVertex* edge);", text=DECLARATION)]
        if placement == 0:
            ctor = body + "\n\n" + ctor
        elif placement == 1:
            edits.append(dict(source=SOURCE, find=factory, replace=factory + "\n\n" + body))
        else:
            site = body + "\n\n" + site
        edits.append(dict(source=SOURCE, find=insertion, replace=site))
        yield dict(name=f"connector_{binding}+placement_{placement}+loop_{loop}+opposite_{opposite}",
                   replace=ctor, extra_edits=edits)


def make_manifest(source, checkpoint_path):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, "TRmgVoronoi::TRmgVoronoi")
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("requires ten completed constructor parents")
    for relative in (SOURCE, HEADER):
        if (context / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("stale constructor parent snapshot: " + relative)
    parent = json.loads((context / "input.json").read_text())["axes"][0]
    if parent["find"] != original:
        raise ValueError("changed constructor parent anchor")
    options = [dict(name="original", replace=original)]
    for elite in checkpoint["elites"]:
        tree = context / "candidates" / elite["id"] / "repeat/tree" / SOURCE
        option = parent["options"][elite["choices"][0]]
        if not tree.is_file() or helper.definition(tree.read_text(), "TRmgVoronoi::TRmgVoronoi") != option["replace"]:
            raise ValueError("missing or changed reproduced parent")
        if option["replace"] != original:
            options.append(dict(name="parent_" + option["name"], replace=option["replace"]))
    options += list(helper_options(source))
    return dict(schema=1, units=generator("generate-rmg-map-accessor-family.py").UNITS,
                evidence=__doc__, parent_checkpoint=str(checkpoint_path.resolve()),
                axes=[dict(name="voronoi_connector", source=SOURCE, find=original, options=options)])


def lifetime_manifest(source, checkpoint_path):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1:
        raise ValueError("unfinished connector search")
    for relative in (SOURCE, HEADER):
        if (context / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("stale connector source snapshot")
    parent = json.loads((context / "input.json").read_text())
    original = generator("generate-rmg-position-family.py").definition(source, "TRmgVoronoi::TRmgVoronoi")
    if parent["axes"][0]["find"] != original:
        raise ValueError("changed connector parent anchor")
    options = [dict(name="original", replace=original)]
    for elite in checkpoint["elites"]:
        option = parent["axes"][0]["options"][elite["choices"][0]]
        _, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        for relative, text in rendered.items():
            path = context / "candidates" / elite["id"] / "repeat/tree" / relative
            if not path.is_file() or path.read_text() != text:
                raise ValueError("connector parent reproduction/input differs")
        options.append(copy.deepcopy(option))
        binding = next((index for index in range(4) if connector(index) in option["replace"] or
                        any(connector(index) in edit.get("replace", "") for edit in option.get("extra_edits", []))), None)
        if binding is None:
            continue
        old = connector(binding)
        for result, previous, twin_result in itertools.product(range(3), range(2), range(2)):
            if (result, previous, twin_result) == (0, 0, 0):
                continue
            new = connector(binding, result, previous, twin_result)
            changed = copy.deepcopy(option)
            changed["name"] += f"+result_{result}+previous_{previous}+twin_{twin_result}"
            changed["replace"] = changed["replace"].replace(old, new)
            for edit in changed.get("extra_edits", []):
                if "replace" in edit:
                    edit["replace"] = edit["replace"].replace(old, new)
            options.append(changed)
    return dict(schema=1, units=parent["units"], evidence=__doc__,
                parent_checkpoint=str(checkpoint_path.resolve()),
                axes=[dict(name="connector_result_lifetimes", source=SOURCE, find=original, options=options)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--lifetimes", action="store_true")
    args = parser.parse_args()
    maker = lifetime_manifest if args.lifetimes else make_manifest
    payload = maker((HOMM3_DIR / SOURCE).read_text(), args.checkpoint)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "Voronoi connector states")


if __name__ == "__main__":
    main()
