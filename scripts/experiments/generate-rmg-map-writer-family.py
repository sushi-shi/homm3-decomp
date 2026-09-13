#!/usr/bin/env python3
"""Retail-only writeMap 0x54abf0: coordinate, vector and return lifetimes.

Retail uses a 0x14 frame, distinct terrain Y/Z homes, prototype-vector
first-pointer-biased receivers, and a final parameter-slot buffer plus SETE AL.
Preserve all iteration bounds, callbacks, wire writes and ordinary helpers.
These are source hypotheses, not recovered Dreamcast declarations.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(
        source, "type_random_map_generator::writeMap")


def replace(body, old, new):
    if body.count(old) != 1:
        raise ValueError("review map-writer anchor: " + old)
    return body.replace(old, new)


def variant(original, coordinates, vectors, result):
    body = original
    if coordinates:
        declarations = ("    int x;\n    int y;\n    int z;\n" if coordinates < 3
                        else "    TRmgMapPosition position;\n")
        for coordinate, bound in (("z", "numberLevels"), ("y", "mapHeight"), ("x", "mapWidth")):
            name = coordinate if coordinates < 3 else "position.m_" + coordinate
            body = replace(body,
                f"for (int {coordinate} = 0; {coordinate} < m_map.m_{bound}; ++{coordinate})",
                f"for ({name} = 0; {name} < m_map.m_{bound}; ++{name})")
        if coordinates in (1, 3):
            body = replace(body, "    TRmgMapItem* item =", declarations + "    TRmgMapItem* item =")
        else:
            body = body.replace("{\n", "{\n" + declarations, 1)
    if vectors:
        for counter, index in (("type", "index"), ("objectType", "prototype")):
            old = f"    for (int {counter} = 0; {counter} < 232; ++{counter})\n"
            binding = (f"std::vector<TRmgObjectPropertiesRef*>& prototypes = m_objectPrototypes[{counter}];"
                       if vectors == 1 else
                       f"std::vector<TRmgObjectPropertiesRef*>* prototypes = &m_objectPrototypes[{counter}];")
            body = replace(body, old, old.rstrip() + " {\n        " + binding + "\n")
            body = replace(body, f"m_objectPrototypes[{counter}].size()",
                "prototypes.size()" if vectors == 1 else "prototypes->size()")
            body = replace(body, f"m_objectPrototypes[{counter}][{index}]",
                f"prototypes[{index}]" if vectors == 1 else f"(*prototypes)[{index}]")
            end = ("                properties->m_prototypeIndex = prototypeCount++;\n        }" if counter == "type" else
                   "                writeRmgObjectPrototype(outfile, properties->m_prototype);\n        }")
            body = replace(body, end, end + "\n    }")
    if result:
        old = "    int reserved = 0;\n    return outfile->write(&reserved, sizeof(reserved)) == sizeof(reserved);"
        expression = "outfile->write(&reserved, sizeof(reserved)) == sizeof(reserved)"
        finish = (
            "",
            "        unsigned char result = " + expression + ";\n        return result;",
            "        return static_cast<unsigned char>(" + expression + ");",
            "        bool result = " + expression + ";\n        return result;",
        )[result]
        body = replace(body, old, "    {\n        int reserved = 0;\n" + finish + "\n    }")
    return body


def axes(source):
    original = definition(source)
    seed = original
    if "    TRmgMapPosition position;\n" in original:
        seed = replace(seed, "    TRmgMapPosition position;\n", "")
        for coordinate, bound in (("z", "numberLevels"), ("y", "mapHeight"), ("x", "mapWidth")):
            name = "position.m_" + coordinate
            seed = replace(seed, f"for ({name} = 0; {name} < m_map.m_{bound}; ++{name})",
                           f"for (int {coordinate} = 0; {coordinate} < m_map.m_{bound}; ++{coordinate})")
        seed = replace(seed,
            "    {\n        int reserved = 0;\n        unsigned char result = outfile->write(&reserved, sizeof(reserved)) == sizeof(reserved);\n        return result;\n    }",
            "    int reserved = 0;\n    return outfile->write(&reserved, sizeof(reserved)) == sizeof(reserved);")
        if variant(seed, 4, 0, 1) != original:
            raise ValueError("review adopted map-writer source")
    options = [(f"coordinates_{c}+vectors_{v}+result_{r}", variant(seed, c, v, r))
               for c, v, r in itertools.product(range(5), range(3), range(4))]
    axis = generator("generate-rmg-position-family.py").axis(
        "map_writer_lifetimes", SOURCE, original, options)
    if len(axis["options"]) != 60:
        raise ValueError("expected sixty distinct map-writer forms")
    return [axis]


def parents(path, source):
    directory = path.parent
    expected = dict(schema=1, units=["rmg"], axes=axes(source), evidence=__doc__)
    if json.loads((directory / "input.json").read_text()) != expected:
        raise ValueError("review map-writer parent manifest")
    if (directory / "snapshot" / SOURCE).read_bytes() != (HOMM3_DIR / SOURCE).read_bytes():
        raise ValueError("map-writer source snapshot changed")
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("map-writer header population changed")
    for item in saved.rglob("*"):
        if item.is_file() and item.read_bytes() != (current / item.relative_to(saved)).read_bytes():
            raise ValueError("map-writer header changed: " + str(item))
    checkpoint = json.loads(path.read_text())
    if len(checkpoint["records"]) != 60 or any(not row.get("scores") for row in checkpoint["records"]) or len(checkpoint["elites"]) != 10:
        raise ValueError("expected sixty scored states and ten reproduced parents")
    result = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        if any(repeated.get(key) != row[key] for key in ("choices", "scores", "object_hash", "source_hashes")):
            raise ValueError("map-writer parent did not reproduce")
        tree = (candidate / "first/tree" / SOURCE).read_text()
        expected_tree = source.replace(expected["axes"][0]["find"], expected["axes"][0]["options"][row["choices"][0]]["replace"])
        if tree != expected_tree or hashlib.sha256(tree.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("map-writer parent source changed")
        result.append((row["id"], definition(tree)))
    return result


def buffers(body):
    first = "    {\n        int reserved = 0;\n        outfile->write(&reserved, sizeof(reserved));\n    }"
    initial = "    int initialReserved = 0;\n    outfile->write(&initialReserved, sizeof(initialReserved));"
    yield "initial_function_scope", replace(body, first, initial)
    counts = body
    for expression in ("prototypeCount", "m_positions.size()"):
        old = "    {\n        int count = " + expression + ";\n        outfile->write(&count, sizeof(count));\n    }"
        name = "prototypeTotal" if expression == "prototypeCount" else "objectTotal"
        counts = replace(counts, old, "    int " + name + " = " + expression + ";\n    outfile->write(&" + name + ", sizeof(" + name + "));")
    yield "counts_function_scope", counts
    yield "all_function_scope", replace(counts, first, initial)
    for early in (False, True):
        shared = replace(body, first, "    wireCount = 0;\n    outfile->write(&wireCount, sizeof(wireCount));")
        for expression in ("prototypeCount", "m_positions.size()"):
            old = "    {\n        int count = " + expression + ";\n        outfile->write(&count, sizeof(count));\n    }"
            shared = replace(shared, old, "    wireCount = " + expression + ";\n    outfile->write(&wireCount, sizeof(wireCount));")
        if early:
            shared = shared.replace("{\n", "{\n    int wireCount;\n", 1)
        else:
            shared = replace(shared, "    wireCount = 0;", "    int wireCount;\n    wireCount = 0;")
        yield "shared_entry" if early else "shared_after_header", shared


def frontier(source, retained):
    options = list(retained)
    refined = [list(buffers(body)) for _, body in retained]
    for variant_index in range(5):
        for parent_index, (label, _) in enumerate(retained):
            name, body = refined[parent_index][(variant_index + parent_index) % 5]
            options.append((label + "+" + name, body))
    axis = generator("generate-rmg-position-family.py").axis(
        "map_writer_buffers", SOURCE, definition(source), options)
    axis["options"] = axis["options"][:60]
    if len(axis["options"]) != 60:
        raise ValueError("expected sixty buffer-frontier sources")
    return [axis]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--buffers-from", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg"], axes=axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    if args.buffers_from:
        source = (HOMM3_DIR / SOURCE).read_text()
        payload["axes"] = frontier(source, parents(args.buffers_from, source))
        payload["parent_checkpoint"] = str(args.buffers_from.resolve())
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated sixty map-writer forms ->", args.output)


if __name__ == "__main__":
    main()
