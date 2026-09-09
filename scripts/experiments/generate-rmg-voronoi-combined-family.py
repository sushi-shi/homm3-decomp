#!/usr/bin/env python3
"""Recombine reproduced Voronoi factory and connector parents.

Retail's four expanded perimeter factories and retained diagonal factory
share the same canonical createEdge definition. Changes to that definition
alter nested vector-insertion decisions in the constructor. Combine the
ten factory and ten connector frontiers atomically, scoring all consumers of
the connector's real member declaration; do not confuse separate peaks with
a jointly reproduced call sequence.
"""
import argparse
import copy
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg_support.cpp"


def parents(path):
    context = path.parent
    checkpoint = json.loads(path.read_text())
    if not checkpoint.get("generation") or len(checkpoint["elites"]) != 10:
        raise ValueError("requires a completed ten-parent frontier")
    payload, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
    for relative in set(originals) | {"include/rmg.h", SOURCE}:
        if (context / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("stale parent snapshot: " + relative)
    result = []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        for relative, text in rendered.items():
            reproduced = context / "candidates" / elite["id"] / "repeat/tree" / relative
            if not reproduced.is_file() or reproduced.read_text() != text:
                raise ValueError("parent reproduction does not match input")
        result.append(copy.deepcopy(payload["axes"][0]["options"][elite["choices"][0]]))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("factory", type=Path)
    parser.add_argument("connector", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    source = (HOMM3_DIR / SOURCE).read_text()
    ctor = helper.definition(source, "TRmgVoronoi::TRmgVoronoi")
    factory = helper.definition(source, "TRmgVoronoi::createEdge")
    factories = parents(args.factory)
    connectors = parents(args.connector)
    # Include independent controls as well as all 100 parent combinations.
    options = [dict(name="original", replace=ctor)]
    for option in factories:
        options.append(dict(name="factory_only+" + option["name"], replace=ctor,
                            extra_edits=[dict(source=SOURCE, find=factory, replace=option["replace"])]))
    for connector in connectors:
        options.append(dict(copy.deepcopy(connector), name="connector_only+" + connector["name"]))
        for variant in factories:
            option = copy.deepcopy(connector)
            option["name"] += "+factory+" + variant["name"]
            # A connector immediately after createEdge shares its anchor;
            # replace the factory inside that atomic edit, never overlap edits.
            edited = False
            for edit in option.get("extra_edits", []):
                if edit.get("find") == factory:
                    if edit["replace"].count(factory) != 1:
                        raise ValueError("review changed factory/connector span")
                    edit["replace"] = edit["replace"].replace(factory, variant["replace"])
                    edited = True
            if not edited:
                option.setdefault("extra_edits", []).append(dict(source=SOURCE, find=factory, replace=variant["replace"]))
            options.append(option)
    payload = dict(schema=1, units=generator("generate-rmg-map-accessor-family.py").UNITS,
                   evidence=__doc__, parent_checkpoints=[str(args.factory.resolve()), str(args.connector.resolve())],
                   axes=[dict(name="voronoi_combined_boundaries", source=SOURCE, find=ctor, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "combined Voronoi states")


if __name__ == "__main__":
    main()
