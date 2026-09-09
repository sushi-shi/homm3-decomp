#!/usr/bin/env python3
"""Prototype filtering induction and version-value lifetimes.

Retail 0x536200 agrees on all 34 blocks and named call positions. The first
loop's index/derived-byte-offset homes are exchanged; its version and record
address occupy different registers. Allocation deliberately re-reads the
table's storage, so never carry the object reference into the constructor.
Sorting, filtering constants, trait remapping and public insertion stay fixed.
No Dreamcast counterpart was found. Cross actual index scopes, version
bindings and record/type declarations, without changing shared helpers.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "TRmgGeneratorBase::loadObjectPrototypes"


def variant(original, lifetime, version, record):
    body = original
    old = "        TObjectType& object = m_objectsTxt.m_objectTypes[index];\n        int type = object.m_objectType;"
    lines = []
    if version in (1, 3):
        lines.append("        " + ("int" if version == 1 else "const int&") + " version = m_mapVersion;")
    if record == 2:
        lines.append("        int type;")
    lines.append("        " + ("const " if record == 1 else "") + "TObjectType& object = m_objectsTxt.m_objectTypes[index];")
    lines.append("        " + ("" if record == 2 else "int ") + "type = object.m_objectType;")
    if version == 2:
        lines.append("        int version = m_mapVersion;")
    if body.count(old) != 1:
        raise ValueError("review changed prototype filtering locals")
    body = body.replace(old, "\n".join(lines))
    if version:
        body = body.replace("if (m_mapVersion <", "if (version <")
    loop = "    for (unsigned int index = 0; index < m_objectsTxt.m_objectTypes.size(); ++index) {"
    if lifetime in (1, 2, 3):
        body = body.replace(loop, "    unsigned int index = 0;\n    for (; index < m_objectsTxt.m_objectTypes.size(); ++index) {")
    elif lifetime == 4:
        body = body.replace('    m_objectsTxt.load("objects.txt");', '    unsigned int index;\n    m_objectsTxt.load("objects.txt");')
        body = body.replace(loop, "    for (index = 0; index < m_objectsTxt.m_objectTypes.size(); ++index) {")
    sort = "    for (unsigned int first = 0; first < m_objectPrototypes[54].size() - 1; ++first) {"
    if lifetime == 2:
        start = body.index("    unsigned int index = 0;")
        end = body.index(sort)
        body = body[:start] + "    {\n" + "\n".join("    " + line for line in body[start:end].splitlines()) + "\n    }\n" + body[end:]
    elif lifetime == 3:
        start = body.index(sort)
        end = body.index("    readObjectPlacementRules();", start)
        block = body[start:end].replace("unsigned int first = 0", "index = 0").replace("first", "index")
        body = body[:start] + block + body[end:]
    return body


def historical_definition(original):
    current = original
    direct = "        int type = m_objectsTxt.m_objectTypes[index].m_objectType;"
    if direct in original:
        original = original.replace(direct, "        const TObjectType& object = m_objectsTxt.m_objectTypes[index];\n        int type = object.m_objectType;")
        original = original.replace("m_objectsTxt.m_objectTypes[index].m_subtype", "object.m_subtype")
    if "for (unsigned int first = 0;" in original:
        if current != original:
            raise ValueError("unreviewed direct-read parent")
        return original
    body = original.replace("    unsigned int index = 0;\n    for (; index <", "    for (unsigned int index = 0; index <")
    body = body.replace("const TObjectType& object", "TObjectType& object")
    start = body.index("    for (index = 0; index < m_objectPrototypes[54]")
    end = body.index("    readObjectPlacementRules();", start)
    sorting = body[start:end].replace("index", "first").replace("for (first = 0;", "for (unsigned int first = 0;")
    body = body[:start] + sorting + body[end:]
    if variant(body, 3, 0, 1) != original:
        raise ValueError("review changed shared-counter child")
    return body


def variants(source):
    current = generator("generate-rmg-position-family.py").definition(source, NAME)
    original = historical_definition(current)
    if current != original:
        yield dict(name="adopted_shared_counter", replace=current)
    for choice in itertools.product(range(5), range(4), range(3)):
        body = variant(original, *choice)
        if current != original and body == current:
            continue
        yield dict(name="index_%d+version_%d+record_%d" % choice,
                   replace=body)


def signedness_frontier(source, checkpoint_path):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or not checkpoint["elites"]:
        raise ValueError("unfinished prototype loader population")
    payload, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
    for relative in (SOURCE, "include/rmg.h", "include/advmgr_objects.h", "include/mapcell.h"):
        if (context / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("changed parent snapshot: " + relative)
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, NAME)
    options = [dict(name="original", replace=original)]
    seen = {original}
    parents = []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        if result["scores"] != elite["scores"] or result["object_hash"] != elite["object_hash"]:
            raise ValueError("parent score/code identity did not reproduce")
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text:
                raise ValueError("changed reproduced source")
        parents.append((elite["id"], helper.definition(rendered[SOURCE], NAME)))
    # Equal comparison signedness does not establish the counter type when
    # the other operand is vector::size_type. Never change a loop bound or
    # cast the size merely to manufacture a different branch instruction.
    for choice in itertools.product(range(2), repeat=3):
        for identity, parent in parents:
            body = parent
            for variable, signed in zip(("index", "first", "second"), choice):
                if signed:
                    body = body.replace("unsigned int " + variable, "int " + variable)
            if body in seen:
                continue
            seen.add(body)
            options.append(dict(name=identity + "+signed_%d%d%d" % choice, replace=body))
    return options


def receiver_frontier(source):
    current = generator("generate-rmg-position-family.py").definition(source, NAME)
    if "for (index = 0; index < m_objectPrototypes[54]" not in current:
        raise ValueError("receiver frontier requires the reviewed shared-counter child")
    original = variant(historical_definition(current), 3, 0, 1)
    if current != original:
        yield dict(name="adopted_direct_reads", replace=current)
    old = "        const TObjectType& object = m_objectsTxt.m_objectTypes[index];\n        int type = object.m_objectType;"
    receivers = [old,
        "        int type = m_objectsTxt.m_objectTypes[index].m_objectType;",
        "        const TObjectType* object = &m_objectsTxt.m_objectTypes[index];\n        int type = object->m_objectType;",
        "        std::vector<TObjectType>::const_iterator object = m_objectsTxt.m_objectTypes.begin() + index;\n        int type = object->m_objectType;",
        "        std::vector<TObjectType>& prototypes = m_objectsTxt.m_objectTypes;\n" + old.replace("m_objectsTxt.m_objectTypes[index]", "prototypes[index]"),
        "        TObjectTypeTable& table = m_objectsTxt;\n" + old.replace("m_objectsTxt.m_objectTypes[index]", "table.m_objectTypes[index]")]
    for receiver, semantic_type, scope in itertools.product(range(6), range(2), range(3)):
        body = original.replace(old, receivers[receiver])
        if receiver == 1:
            body = body.replace("object.m_subtype", "m_objectsTxt.m_objectTypes[index].m_subtype")
        elif receiver in (2, 3):
            body = body.replace("object.m_subtype", "object->m_subtype")
        if semantic_type:
            body = body.replace("        int type =", "        TAdventureObjectType type =")
        if scope == 1:
            body = body.replace('    m_objectsTxt.load("objects.txt");\n    unsigned int index = 0;',
                                '    unsigned int index;\n    m_objectsTxt.load("objects.txt");\n    index = 0;')
        elif scope == 2:
            body = body.replace("    unsigned int index = 0;\n    for (; index <", "    for (unsigned int index = 0; index <")
        if current != original and body == current:
            continue
        yield dict(name=f"receiver_{receiver}+type_{semantic_type}+scope_{scope}", replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path,
                        help="counter-type frontier from every reproduced distinct parent")
    parser.add_argument("--receivers", action="store_true",
                        help="record/container receivers and the original object's enum type")
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, NAME)
    if args.parents_from and args.receivers:
        parser.error("choose only one frontier")
    options = (signedness_frontier(source, args.parents_from) if args.parents_from else
               list(receiver_frontier(source)) if args.receivers else list(variants(source)))
    if options[0]["replace"] != original:
        raise ValueError("unchanged control differs")
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="prototype_loader_lifetimes", source=SOURCE, find=original, options=options)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "prototype loader states")


if __name__ == "__main__":
    main()
