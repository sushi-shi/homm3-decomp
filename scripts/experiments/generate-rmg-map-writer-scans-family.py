#!/usr/bin/env python3
"""Public vector access and loop-counter lifetimes for writeMap 0x54abf0.

The adopted coordinate/result locals already reproduce every frame home.
Only the two prototype-vector receiver biases differ. Preserve size queries
on every iteration, ordinary helper calls and the two serialization passes.
No raw vector internals, cached sizes across callbacks or inline controls.
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
    return generator("generate-rmg-map-writer-family.py").definition(source)


def replace(body, old, new, count=1):
    if body.count(old) != count:
        raise ValueError("review map-writer scan anchor: " + old)
    return body.replace(old, new)


def variant(original, lifetime, access, types):
    body = original
    outer_type = "unsigned int" if types == 1 else "int"
    inner_type = "int" if types == 2 else "unsigned int"
    for outer, inner in (("type", "index"), ("objectType", "prototype")):
        body = replace(body, "for (int " + outer + " =", "for (" + outer_type + " " + outer + " =")
        body = replace(body, "for (unsigned int " + inner + " =", "for (" + inner_type + " " + inner + " =")
        old = f"            TRmgObjectPropertiesRef* properties = m_objectPrototypes[{outer}][{inner}];"
        container = f"m_objectPrototypes[{outer}]"
        value = (f"{container}[{inner}]", f"{container}.begin()[{inner}]",
                 f"*({container}.begin() + {inner})", "*entry")[access]
        before = (f"            std::vector<TRmgObjectPropertiesRef*>::iterator entry = {container}.begin() + {inner};\n"
                  if access == 3 else "")
        body = replace(body, old, before + "            TRmgObjectPropertiesRef* properties = " + value + ";")
    if lifetime:
        share_inner = lifetime in (2, 4)
        for old in ("type", "objectType"):
            body = replace(body, f"for ({outer_type} {old} = 0; {old} < 232; ++{old})",
                           "for (type = 0; type < 232; ++type)")
        body = replace(body, "m_objectPrototypes[objectType]", "m_objectPrototypes[type]", 2)
        if share_inner:
            for old in ("index", "prototype"):
                body = replace(body, f"for ({inner_type} {old} = 0; {old} < m_objectPrototypes[type].size(); ++{old})",
                               "for (index = 0; index < m_objectPrototypes[type].size(); ++index)")
            if access == 0:
                body = replace(body, "m_objectPrototypes[type][prototype]", "m_objectPrototypes[type][index]")
            elif access == 1:
                body = replace(body, "m_objectPrototypes[type].begin()[prototype]", "m_objectPrototypes[type].begin()[index]")
            else:
                body = replace(body, "m_objectPrototypes[type].begin() + prototype", "m_objectPrototypes[type].begin() + index")
        declarations = "    " + outer_type + " type;\n"
        if share_inner:
            declarations += "    " + inner_type + " index;\n"
        if lifetime in (3, 4):
            body = body.replace("{\n", "{\n" + declarations, 1)
        else:
            body = replace(body, "    int prototypeCount = 2;", declarations + "    int prototypeCount = 2;")
    return body


def axes(source):
    original = definition(source)
    options = [(f"lifetime_{l}+access_{a}+types_{t}", variant(original, l, a, t))
               for l, a, t in itertools.product(range(5), range(4), range(3))]
    axis = generator("generate-rmg-position-family.py").axis("prototype_scans", SOURCE, original, options)
    if len(axis["options"]) != 60:
        raise ValueError("expected sixty distinct scan forms")
    return [axis]


def parents(path, source):
    directory = path.parent
    payload = dict(schema=1, units=["rmg"], axes=axes(source), evidence=__doc__)
    if json.loads((directory / "input.json").read_text()) != payload:
        raise ValueError("review scan parent manifest")
    if (directory / "snapshot" / SOURCE).read_bytes() != (HOMM3_DIR / SOURCE).read_bytes():
        raise ValueError("scan source snapshot changed")
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("scan header population changed")
    for item in saved.rglob("*"):
        if item.is_file() and item.read_bytes() != (current / item.relative_to(saved)).read_bytes():
            raise ValueError("scan header snapshot changed: " + str(item))
    checkpoint = json.loads(path.read_text())
    rows = checkpoint["records"]
    if len(rows) != 60 or any(not row.get("scores") for row in rows):
        raise ValueError("expected sixty successfully scored scan parents")
    if len(checkpoint["elites"]) != min(10, len({row["object_hash"] for row in rows})):
        raise ValueError("expected the full distinct reproduced frontier")
    result = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        if any(repeated.get(key) != row[key] for key in ("choices", "scores", "object_hash", "source_hashes")):
            raise ValueError("scan parent did not reproduce")
        rendered = source.replace(payload["axes"][0]["find"], payload["axes"][0]["options"][row["choices"][0]]["replace"])
        actual = (candidate / "first/tree" / SOURCE).read_text()
        if actual != rendered or hashlib.sha256(actual.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("scan parent source changed")
        result.append((row["id"], definition(actual)))
    return result


def owners(body):
    vector = "std::vector<TRmgObjectPropertiesRef*>"
    changed = body
    for name in ("type", "objectType"):
        receiver = "m_objectPrototypes[" + name + "]"
        changed = changed.replace(receiver, "static_cast<const " + vector + "&>(" + receiver + ")")
    yield "const_receivers", changed.replace("::iterator entry", "::const_iterator entry")
    for label, declaration in (
        ("array_pointer", vector + "* prototypeLists = m_objectPrototypes;"),
        ("const_array_pointer", "const " + vector + "* prototypeLists = m_objectPrototypes;"),
        ("const_array_reference", "const " + vector + " (&prototypeLists)[232] = m_objectPrototypes;"),
    ):
        changed = body.replace("{\n", "{\n    " + declaration + "\n", 1)
        for name in ("type", "objectType"):
            changed = changed.replace("m_objectPrototypes[" + name + "]", "prototypeLists[" + name + "]")
        if label != "array_pointer":
            changed = changed.replace("::iterator entry", "::const_iterator entry")
        yield label, changed
    yield "property_pointer_reference", replace(body,
        "TRmgObjectPropertiesRef* properties =", "TRmgObjectPropertiesRef* const& properties =", 2)
    changed = replace(body, "TRmgObjectPropertiesRef* properties = ", "TRmgObjectPropertiesRef& properties = *", 2)
    # A leading dereference must cover the complete indexed/iterator expression.
    lines = changed.splitlines()
    for index, line in enumerate(lines):
        marker = "TRmgObjectPropertiesRef& properties = *"
        if marker in line:
            prefix, expression = line.split(marker)
            lines[index] = prefix + marker + "(" + expression[:-1] + ");"
    changed = "\n".join(lines)
    for member in ("m_refCount", "m_prototypeIndex", "m_prototype"):
        changed = changed.replace("(properties->" + member, "(properties." + member)
        changed = changed.replace("                properties->" + member, "                properties." + member)
        changed = changed.replace(", properties->" + member, ", properties." + member)
    yield "property_object_reference", changed


def frontier(source, retained):
    options = list(retained)
    refinements = [list(owners(body)) for _, body in retained]
    for generation in range(6):
        for index, (label, _) in enumerate(retained):
            name, body = refinements[index][(generation + index) % 6]
            options.append((label + "+" + name, body))
    axis = generator("generate-rmg-position-family.py").axis("prototype_owners", SOURCE, definition(source), options)
    axis["options"] = axis["options"][:60]
    if len(axis["options"]) != 60:
        raise ValueError("expected sixty distinct owner-frontier forms")
    return [axis]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--owners-from", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg"], axes=axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    if args.owners_from:
        source = (HOMM3_DIR / SOURCE).read_text()
        payload["axes"] = frontier(source, parents(args.owners_from, source))
        payload["parent_checkpoint"] = str(args.owners_from.resolve())
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated sixty map-writer scan forms ->", args.output)


if __name__ == "__main__":
    main()
