#!/usr/bin/env python3
"""Prototype wire-buffer initialization and lifetimes at retail 0x54ae30.

Retail's two six-byte buffers use a dword plus a word zero-store; the current
partial aggregate initializer emits two bytes around an unaligned dword.
The frame is four bytes short and the terrain/scalar buffer homes differ.
Preserve the two opaque image-name queries, all eleven ordered stream writes,
the canonical bit-position helper and every field read after its prior write.
No mapped Dreamcast counterpart, helper changes, raw bitset views or pins.
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
NAME = "writeRmgObjectPrototype"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def seed_body(source):
    actual = definition(source)
    if "        unsigned char mask[6] = {0};" in actual:
        return actual
    seed = replace(actual, "    unsigned char terrainMask[2];\n", "")
    seed = replace(seed, "    int x;\n    int y;\n", "")
    seed = replace(seed, "for (y = 6; y--;)", "for (int y = 6; y--;)", 2)
    seed = replace(seed, "for (x = 7; x >= 0; --x)", "for (int x = 7; x >= 0; --x)", 2)
    seed = replace(seed, "        unsigned char mask[6];\n        memset(mask, 0, sizeof(mask));",
        "        unsigned char mask[6] = {0};", 2)
    seed = replace(seed, "        terrainMask[0] = 0;\n        terrainMask[1] = 0;",
        "        unsigned char mask[2] = {0, 0};", 2)
    seed = replace(seed, "terrainMask[terrain / 8]", "mask[terrain / 8]", 2)
    seed = replace(seed, "write(terrainMask, sizeof(terrainMask))", "write(mask, sizeof(mask))", 2)
    seed = replace(seed, "    int reserved[4];\n    memset(reserved, 0, sizeof(reserved));",
        "    int reserved[4] = {0, 0, 0, 0};")
    recognized = dict(reserved_refinements(dict(loop_refinements(variant(seed, 2, 0, 2)))["shared_xy"]))["int_memset"]
    if actual != recognized:
        raise ValueError("review the adopted prototype-writer form")
    return seed


def replace(body, old, new, count=1):
    if body.count(old) != count:
        raise ValueError("review prototype-writer anchor: " + old)
    return body.replace(old, new)


def variant(original, initialization, scalars, terrain):
    body = original
    old = "        unsigned char mask[6] = {0};"
    new = (
        old,
        "        unsigned char mask[6] = {0, 0, 0, 0, 0, 0};",
        "        unsigned char mask[6];\n        memset(mask, 0, sizeof(mask));",
        "        unsigned char mask[6];\n        std::fill(mask, mask + sizeof(mask), 0);",
        "        unsigned char mask[6];\n        for (int byte = 0; byte < sizeof(mask); ++byte)\n            mask[byte] = 0;",
    )[initialization]
    body = replace(body, old, new, 2)
    declarations = []
    if scalars in (1, 3):
        declarations.append("    int intBuffer;\n")
        body = replace(body, "        int length = nameLength;\n        outfile->write(&length, sizeof(length));",
            "        intBuffer = nameLength;\n        outfile->write(&intBuffer, sizeof(intBuffer));")
        for field in ("m_objectType", "m_subtype"):
            body = replace(body, "        int value = prototype->" + field + ";\n        outfile->write(&value, sizeof(value));",
                "        intBuffer = prototype->" + field + ";\n        outfile->write(&intBuffer, sizeof(intBuffer));")
    if scalars in (2, 3):
        declarations.append("    char byteBuffer;\n")
        for field in ("m_slotCategory", "m_isUnderlay"):
            body = replace(body, "        char value = prototype->" + field + ";\n        outfile->write(&value, sizeof(value));",
                "        byteBuffer = prototype->" + field + ";\n        outfile->write(&byteBuffer, sizeof(byteBuffer));")
    if terrain:
        for field in ("m_terrainMask", "m_recommendedTerrainMask"):
            old = """    {
        unsigned char mask[2] = {0, 0};
        for (int terrain = 0; terrain < 10; ++terrain)
            if (prototype->FIELD.test(terrain))
                mask[terrain / 8] |= 1 << (terrain % 8);
        outfile->write(mask, sizeof(mask));
    }""".replace("FIELD", field)
            new = old.replace("unsigned char mask[2] = {0, 0};", "terrainMask[0] = 0;\n        terrainMask[1] = 0;")
            new = new.replace("mask[terrain / 8]", "terrainMask[terrain / 8]").replace("write(mask, sizeof(mask))", "write(terrainMask, sizeof(terrainMask))")
            body = replace(body, old, new)
        if terrain == 1:
            anchor = "    {\n        terrainMask[0] = 0;"
            body = body.replace(anchor, "    unsigned char terrainMask[2];\n" + anchor, 1)
        else:
            declarations.append("    unsigned char terrainMask[2];\n")
    return body.replace("{\n", "{\n" + "".join(declarations), 1)


def axes(source):
    original = definition(source)
    seed = seed_body(source)
    options = [(f"initialization_{i}+scalars_{s}+terrain_{t}", variant(seed, i, s, t))
        for i, s, t in itertools.product(range(5), range(4), range(3))]
    if original != seed:
        options = options[:59]
    result = generator("generate-rmg-position-family.py").axis("prototype_wire_buffers", SOURCE, original, options)
    if len(result["options"]) != 60:
        raise ValueError("expected sixty distinct writer sources")
    return [result]


def parents(checkpoint_path, source, *, loops=False):
    directory = checkpoint_path.parent
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg"], axes=axes(source), evidence=__doc__)
    if loops:
        ancestor = Path(payload["parent_checkpoint"])
        expected["axes"] = loop_axes(source, parents(ancestor, source))
        expected["parent_checkpoint"] = str(ancestor.resolve())
    if payload != expected:
        raise ValueError("review prototype-writer parent manifest")
    if (directory / "snapshot" / SOURCE).read_bytes() != (HOMM3_DIR / SOURCE).read_bytes():
        raise ValueError("review changed prototype-writer source snapshot")
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review changed prototype-writer header population")
    for path in saved.rglob("*"):
        if path.is_file() and path.read_bytes() != (current / path.relative_to(saved)).read_bytes():
            raise ValueError("review changed prototype-writer header " + str(path))
    checkpoint = json.loads(checkpoint_path.read_text())
    if len(checkpoint["records"]) != 60 or any(not row.get("scores") for row in checkpoint["records"]) or len(checkpoint["elites"]) != 10:
        raise ValueError("expected sixty scored states and ten reproduced parents")
    result = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        if any(repeated.get(key) != row[key] for key in ("choices", "scores", "object_hash", "source_hashes")):
            raise ValueError("prototype-writer parent did not reproduce")
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("prototype-writer parent source changed")
        result.append((row["id"], definition(text)))
    return result


def loop_refinements(body):
    for label, variables, early in (
        ("shared_xy", ("x", "y"), False),
        ("entry_xy", ("x", "y"), True),
        ("shared_y", ("y",), False),
        ("shared_x", ("x",), False),
        ("shared_bit", ("bit",), False),
        ("shared_all", ("x", "y", "bit"), False),
    ):
        changed = body
        declaration = "".join("    int " + variable + ";\n" for variable in variables)
        for variable in variables:
            before = "int " + variable + " = "
            if changed.count(before) != 2:
                raise ValueError("review prototype-writer loop variable " + variable)
            changed = changed.replace(before, variable + " = ")
        if early:
            changed = changed.replace("{\n", "{\n" + declaration, 1)
        else:
            anchor = "    {\n        unsigned char mask[6]"
            if changed.count(anchor) != 2:
                raise ValueError("review prototype-writer mask scope")
            changed = changed.replace(anchor, declaration + anchor, 1)
        yield label, changed


def reserved_refinements(body):
    original = "    int reserved[4] = {0, 0, 0, 0};"
    for label, initialization in (
        ("int_memset", "    int reserved[4];\n    memset(reserved, 0, sizeof(reserved));"),
        ("unsigned_array", "    unsigned int reserved[4] = {0, 0, 0, 0};"),
        ("bytes_memset", "    unsigned char reserved[16];\n    memset(reserved, 0, sizeof(reserved));"),
        ("bytes_full", "    unsigned char reserved[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};"),
        ("int_assignments", "    int reserved[4];\n    reserved[0] = 0;\n    reserved[1] = 0;\n    reserved[2] = 0;\n    reserved[3] = 0;"),
        ("int_fill", "    int reserved[4];\n    std::fill(reserved, reserved + 4, 0);"),
    ):
        yield label, replace(body, original, initialization)


def loop_axes(source, retained, *, reserved=False):
    original = definition(source)
    name = "prototype_reserved_storage" if reserved else "prototype_loop_lifetimes"
    options = generator("generate-rmg-position-family.py").axis(name, SOURCE, original, retained)["options"]
    seen = {option["replace"] for option in options}
    if len(retained) != 10 or len({body for _, body in retained}) != 10:
        raise ValueError("expected ten distinct writer parent sources")
    # Rotate whole lifetime alternatives so the bounded frontier covers each
    # intended axis, rather than spending its width on the earliest spellings.
    mutations = []
    for index, (_, body) in enumerate(retained):
        rows = list(reserved_refinements(body) if reserved else loop_refinements(body))
        offset = index % len(rows)
        mutations.append(rows[offset:] + rows[:offset])
    for entries in itertools.zip_longest(*mutations):
        for (parent, _), entry in zip(retained, entries):
            if entry is None:
                continue
            label, body = entry
            if body not in seen:
                seen.add(body)
                options.append(dict(name=parent + "+" + label, replace=body))
            if len(options) == 60:
                return [dict(name=name, source=SOURCE, find=original, options=options)]
    return [dict(name=name, source=SOURCE, find=original, options=options)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--loops-from", type=Path)
    parser.add_argument("--reserved-from", type=Path)
    args = parser.parse_args()
    if args.loops_from and args.reserved_from:
        parser.error("choose one parent frontier")
    payload = dict(schema=1, units=["rmg"], axes=axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    if args.loops_from:
        source = (HOMM3_DIR / SOURCE).read_text()
        payload["axes"] = loop_axes(source, parents(args.loops_from, source))
        payload["parent_checkpoint"] = str(args.loops_from.resolve())
    if args.reserved_from:
        source = (HOMM3_DIR / SOURCE).read_text()
        payload["axes"] = loop_axes(source, parents(args.reserved_from, source, loops=True), reserved=True)
        payload["parent_checkpoint"] = str(args.reserved_from.resolve())
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated sixty prototype-writer sources ->", args.output)


if __name__ == "__main__":
    main()
