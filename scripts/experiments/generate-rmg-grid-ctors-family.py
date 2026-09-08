#!/usr/bin/env python3
"""Generate 60 shared grid-constructor source states across all three RMG TUs.

Retail 0x4fa520 retains the explicit grid copy boundary; 0x5b76b0 reads
both coordinate arguments through references. Both canonical bodies are
exact. Brush destruction 0x5b72f0 and repairTerrainPoint 0x5b5440 still
disagree at nested expansions involving those same ordinary value operations.
The older grid family tested initializer-only versus body-only construction.
Extend it with mixed initializer/body forms and independent field-store
order, preserving the class layout, both signatures, assignment's value semantics,
and one canonical compound-add helper. Complete-only RMG has no DC counterpart.

Six copy implementations x five coordinate implementations x two assigned
translation returns = 60 real source combinations. No alternate declaration,
missing body, artificial destructor, dummy work, inline qualifier or pragma.
"""
import argparse
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, rank, select_elites

HEADER = "include/rmg.h"


def constructors(copy):
    signature = ("    TRmgGridPoint(const TRmgGridPoint& other)" if copy else
                 "    TRmgGridPoint(const unsigned int& newX, const unsigned int& newY)")
    x, y = ("other.m_x", "other.m_y") if copy else ("newX", "newY")
    yield "member_initializers", signature + "\n        : m_x(" + x + "), m_y(" + y + ") {}"
    for name, fields in (("field_stores", (("x", x), ("y", y))),
                         ("field_stores_yx", (("y", y), ("x", x)))):
        yield name, signature + "\n    {\n" + "\n".join(
            "        m_" + field + " = " + value + ";" for field, value in fields) + "\n    }"
    yield "x_initializer", signature + "\n        : m_x(" + x + ")\n    {\n        m_y = " + y + ";\n    }"
    yield "y_initializer", signature + "\n        : m_y(" + y + ")\n    {\n        m_x = " + x + ";\n    }"
    if copy:
        yield "assignment", signature + "\n    {\n        *this = other;\n    }"


def translations(expanded=False):
    prefix = "        TRmgGridPoint result;\n        result = *this;\n"
    yield "assigned_named", prefix + "        result += offset;\n        return result;"
    yield "assigned_compound", prefix + "        return result += offset;"
    if expanded:
        for construction, declaration in (
                ("copy", "TRmgGridPoint result(*this);"),
                ("copy_initialized", "TRmgGridPoint result = *this;"),
                ("coordinates", "TRmgGridPoint result(m_x, m_y);"),
                ("coordinates_initialized", "TRmgGridPoint result = TRmgGridPoint(m_x, m_y);")):
            for returned in ("named", "compound"):
                body = "        " + declaration + "\n"
                body += ("        result += offset;\n        return result;" if returned == "named" else
                         "        return result += offset;")
                yield construction + "_" + returned, body


def make_manifest(header, expanded=False):
    axes = []
    for name, forms in (("copy_constructor", constructors(True)),
                        ("coordinate_constructor", constructors(False)),
                        ("translation_return", translations(expanded))):
        options = list(forms)
        matches = [body for _, body in options if header.count(body) == 1]
        if len(matches) != 1:
            raise ValueError("review the current grid " + name + " before rebasing")
        original = matches[0]
        options.sort(key=lambda row: row[1] != original)
        axes.append(dict(name=name, find=original, options=[dict(name=label, replace=body)
                                                          for label, body in options]))
    return dict(schema=1, source=HEADER, units=["rmg", "rmg_support", "rmg_terrain"],
                evidence=__doc__, axes=axes)


def make_parent_manifest(header, parents):
    payload = make_manifest(header, expanded=True)
    copy_axis, coordinate_axis, translation_axis = payload["axes"]
    copies, coordinates = dict(constructors(True)), dict(constructors(False))
    current = (copy_axis["options"][0]["name"], coordinate_axis["options"][0]["name"])
    if (len(parents) != 6 or len(set(parents)) != 6 or current not in parents
            or any(copy not in copies or coordinate not in coordinates for copy, coordinate in parents)):
        raise ValueError("review six unique constructor parents including the current source")
    parents = sorted(parents, key=lambda pair: pair != current)
    options = [dict(name=copy + "+" + coordinate, replace=copies[copy], extra_edits=[dict(
        find=coordinate_axis["find"], replace=coordinates[coordinate])]) for copy, coordinate in parents]
    payload["axes"] = [dict(name="constructor_parent", find=copy_axis["find"], options=options), translation_axis]
    payload["evidence"] += ("\nFollow-up: the first 60 states yield six distinct code/relocation results. "
                            "Cross those reproduced constructor parents with ten real translation lifetimes. "
                            "Replace the byte-identical leading elite with the unchanged-source control, "
                            "and keep the canonical compound-add call in every return form.")
    return payload


def grid_definition(header):
    start = header.index("struct TRmgGridPoint {")
    return header[start:header.index("\n};", start) + 3]


def member_orders(header):
    """Reorder real in-class definitions; never reorder the data members."""
    point = grid_definition(header)
    prefix_end = point.index("    TRmgGridPoint() {}\n") + len("    TRmgGridPoint() {}\n")
    coordinate = point.index("    TRmgGridPoint(const unsigned int& newX, const unsigned int& newY)")
    translation = point.index("    TRmgGridPoint& operator+=(const TPoint& offset);")
    if not prefix_end < point.index("    TRmgGridPoint(const TRmgGridPoint& other)") < coordinate < translation:
        raise ValueError("review canonical member ordering in the constructor parents")
    blocks = {"copy": point[prefix_end:coordinate].rstrip("\n"),
              "coordinates": point[coordinate:translation].rstrip("\n"),
              "translation": point[translation:-3].rstrip("\n")}
    for order in itertools.permutations(blocks):
        yield "-".join(order), point[:prefix_end] + "\n\n".join(blocks[key] for key in order) + "\n};"


def make_member_order_manifest(header, parents):
    if len(parents) != 10 or len({text for _, text in parents}) != 10 or not any(text == header for _, text in parents):
        raise ValueError("review ten distinct grid parents including the unchanged source")
    original = grid_definition(header)
    surrounding = header.replace(original, "")
    options = []
    for label, text in sorted(parents, key=lambda row: row[1] != header):
        if text.replace(grid_definition(text), "") != surrounding:
            raise ValueError("review parent changes outside the grid class")
        options.extend(dict(name=label + "/" + name, replace=point) for name, point in member_orders(text))
    if options[0]["replace"] != original or len({row["replace"] for row in options}) != 60:
        raise ValueError("review 60 distinct member orders and the unchanged-source control")
    return dict(schema=1, source=HEADER, units=["rmg", "rmg_support", "rmg_terrain"],
                evidence=__doc__ + "\nFollow-up: ten reproduced constructor/translation parents crossed with "
                "six in-class member-definition orders. Keep data order, signatures, ordinary compound-add "
                "definition and implicit assignment unchanged. Complete-only retail retains the copy "
                "before compound-add (0x4fa520/0x4fa540), but coordinate construction later (0x5b76b0); "
                "this tests compiler source visibility, not a claimed original header order.",
                axes=[dict(name="grid_parent_member_order", find=original, options=options)])


def load_member_order_parents(checkpoint_path, header):
    checkpoint = json.loads(checkpoint_path.read_text())
    directory = checkpoint_path.parent
    parent_input = json.loads((directory / "input.json").read_text())
    if parent_input.get("units") != ["rmg", "rmg_support", "rmg_terrain"]:
        raise ValueError("review the three-TU constructor checkpoint")
    for relative in (HEADER, "src/rmg.cpp", "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review constructor parents against the current source: " + relative)
    baseline = next((row for row in checkpoint["records"] if not any(row["choices"])), None)
    elites = checkpoint["elites"]
    equivalent = [row for row in elites if baseline and row["object_hash"] == baseline["object_hash"]]
    if len(elites) != 10 or len(equivalent) != 1:
        raise ValueError("review ten reproduced parents and the baseline-equivalent member")
    # Preserve an exactly unchanged authored-source control, not just an
    # equivalent object under a different compound-return spelling.
    parents = [("baseline", header)]
    for row in elites:
        if row is equivalent[0]:
            continue
        text = (directory / "candidates" / row["id"] / "first/tree" / HEADER).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][HEADER]:
            raise ValueError("parent source hash changed: " + row["id"])
        parents.append((row["id"], text))
    return parents


def assignments():
    yield "implicit", ""
    signature = "    TRmgGridPoint& operator=(const TRmgGridPoint& other)\n    {\n"
    for fields in ("xy", "yx"):
        stores = ["m_" + field + " = other.m_" + field for field in fields]
        yield "stores_" + fields, signature + "".join("        " + store + ";\n" for store in stores) + "        return *this;\n    }\n\n"
        yield "returned_stores_" + fields, signature + "        return " + ", ".join(stores + ["*this"]) + ";\n    }\n\n"
    yield "receiver_reference", signature + """        TRmgGridPoint& result = *this;
        result.m_x = other.m_x;
        result.m_y = other.m_y;
        return result;
    }

"""


def make_assignment_manifest(header, parents):
    if len(parents) != 10 or len({text for _, text in parents}) != 10 or not any(text == header for _, text in parents):
        raise ValueError("review ten distinct grid parents including the unchanged source")
    original = grid_definition(header)
    surrounding = header.replace(original, "")
    anchor = "    TRmgGridPoint& operator+=(const TPoint& offset);"
    options = []
    for label, text in sorted(parents, key=lambda row: row[1] != header):
        point = grid_definition(text)
        if text.replace(point, "") != surrounding or "TRmgGridPoint& operator=(" in point or point.count(anchor) != 1:
            raise ValueError("review implicit assignment and the parent grid class")
        options.extend(dict(name=label + "/" + name, replace=point.replace(anchor, body + anchor))
                       for name, body in assignments())
    if options[0]["replace"] != original or len({row["replace"] for row in options}) != 60:
        raise ValueError("review 60 distinct assignment states and the unchanged-source control")
    return dict(schema=1, source=HEADER, units=["rmg", "rmg_support", "rmg_terrain"],
                evidence=__doc__ + "\nFollow-up: member order is byte-neutral for ten reproduced parents. "
                "The assignment-built copy parent drops the retained 0x4fa520 body in all three TUs, "
                "not just its claim label. Test that parent's actual copy-assignment helper boundary: "
                "implicit versus five explicit in-class implementations with the same const-reference "
                "argument, receiver-reference result and unsigned field values. Keep one canonical "
                "copy-assignment operation; no guard, additional constructor, dummy work or inline pin.",
                axes=[dict(name="grid_parent_assignment", find=original, options=options)])


def proxy_module():
    spec = importlib.util.spec_from_file_location(
        "rmg_grid_proxy", Path(__file__).with_name("generate-rmg-line-refresh-family.py"))
    proxy = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(proxy)
    return proxy


def make_proxy_manifest(header, source, parents):
    if len(parents) != 10 or len({text for _, text in parents}) != 10 or not any(text == header for _, text in parents):
        raise ValueError("review ten distinct grid parents including the unchanged source")
    original = grid_definition(header)
    surrounding = header.replace(original, "")
    options = []
    for label, text in sorted(parents, key=lambda row: row[1] != header):
        point = grid_definition(text)
        if text.replace(point, "") != surrounding:
            raise ValueError("review parent changes outside the grid class")
        options.append(dict(name=label, replace=point))
    proxy = proxy_module()
    constructor = proxy.constructor_definition(source)
    constructor_axis = proxy.helpers().axis("proxy_constructor", "src/rmg_terrain.cpp", constructor,
                                           proxy.constructor_forms(proxy.constructor_parameter(source)))
    return dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__ +
                "\nFollow-up: retain ten reproduced grid/assignment parents and cross them with six "
                "real proxy member constructions. Refresh 0x4f9f00 expands the original proxy but retains "
                "the neighbour factory at 0x4f9f86; current source instead retains an entry grid copy "
                "and expands the factory. The best coordinate-return parent restores compound-add at "
                "0x4f9f60, but still substitutes a grid-copy call for the retained neighbour factory. "
                "Keep the same proxy interface, copied-coordinate ownership and one ordinary constructor.",
                axes=[dict(name="grid_parent", source=HEADER, find=original, options=options), constructor_axis])


def temporary_translations():
    assigned = "        TRmgGridPoint result;\n        result = TRmgGridPoint(m_x, m_y);\n"
    yield "coordinate_assignment_named", assigned + "        result += offset;\n        return result;"
    yield "coordinate_assignment_compound", assigned + "        return result += offset;"
    yield "temporary_coordinates_compound", "        return TRmgGridPoint(m_x, m_y) += offset;"
    yield "temporary_copy_compound", "        return TRmgGridPoint(*this) += offset;"
    yield "explicit_copy_of_compound", ("        TRmgGridPoint result(m_x, m_y);\n"
                                       "        return TRmgGridPoint(result += offset);")


def load_temporary_parents(checkpoint_path, originals):
    checkpoint = json.loads(checkpoint_path.read_text())
    directory = checkpoint_path.parent
    parent_input = json.loads((directory / "input.json").read_text())
    if parent_input.get("units") != ["rmg", "rmg_support", "rmg_terrain"]:
        raise ValueError("review the three-TU proxy checkpoint")
    for relative in (HEADER, "src/rmg.cpp", "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review proxy parents against the current source: " + relative)
    baseline = next((row for row in checkpoint["records"] if not any(row["choices"])), None)
    elites = checkpoint["elites"]
    equivalent = [row for row in elites if baseline and row["object_hash"] == baseline["object_hash"]]
    if len(elites) != 10 or len(equivalent) != 1:
        raise ValueError("review ten reproduced proxy parents and the baseline-equivalent member")
    parents = [("baseline", originals)]
    for row in elites:
        if row is equivalent[0]:
            continue
        sources = {}
        for relative in originals:
            text = (directory / "candidates" / row["id"] / "first/tree" / relative).read_text()
            if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][relative]:
                raise ValueError("parent source hash changed: " + row["id"] + "/" + relative)
            sources[relative] = text
        parents.append((row["id"], sources))
    return parents


def make_temporary_manifest(originals, parents):
    source_name = "src/rmg_terrain.cpp"
    if len(parents) != 10 or not any(texts == originals for _, texts in parents):
        raise ValueError("review ten proxy/grid parents including the unchanged source")
    original = grid_definition(originals[HEADER])
    surrounding = originals[HEADER].replace(original, "")
    proxy = proxy_module()
    constructor = proxy.constructor_definition(originals[source_name])
    source_surrounding = originals[source_name].replace(constructor, "")
    options = []
    for label, texts in sorted(parents, key=lambda row: row[1] != originals):
        point = grid_definition(texts[HEADER])
        proxy_constructor = proxy.constructor_definition(texts[source_name])
        if (texts[HEADER].replace(point, "") != surrounding
                or texts[source_name].replace(proxy_constructor, "") != source_surrounding):
            raise ValueError("review parent changes outside the grid class and proxy constructor")
        matches = [body for _, body in translations(expanded=True) if point.count(body) == 1]
        if len(matches) != 1:
            raise ValueError("review the parent translation lifetime")
        original_translation = matches[0]
        for name, body in [("parent", original_translation), *temporary_translations()]:
            options.append(dict(name=label + "/" + name, replace=point.replace(original_translation, body),
                                extra_edits=[dict(source=source_name, find=constructor, replace=proxy_constructor)]))
    states = {(row["replace"], row["extra_edits"][0]["replace"]) for row in options}
    if len(states) != 60 or options[0]["replace"] != original:
        raise ValueError("review 60 distinct temporary states and the unchanged-source control")
    return dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__ +
                "\nFollow-up: carry each reproduced grid AND proxy-constructor parent together. "
                "The safe frontier restores += at retail 0x4f9f60, but passes its receiver result "
                "straight to a copy whereas retail first snapshots both returned fields and then "
                "copies at 0x4f9f77. Test real coordinate-assigned results, translated temporaries "
                "and an explicitly constructed return copy, retaining canonical += and at calls. "
                "No new interface, dummy work or inline qualifier is introduced.",
                axes=[dict(name="proxy_grid_parent_temporary", source=HEADER, find=original, options=options)])


def translation_method(point):
    signature = "    TRmgGridPoint operator+(const TPoint& offset) const"
    start = point.index(signature + "\n    {")
    end = point.index("\n    }", start) + len("\n    }")
    method = point[start:end]
    ordinary = "\n".join(line[4:] if line.startswith("    ") else line for line in method.splitlines())
    ordinary = ordinary.replace("TRmgGridPoint operator+(", "TRmgGridPoint TRmgGridPoint::operator+(", 1)
    return method, signature + ";", ordinary


def make_visibility_manifest(originals, parents):
    source_name = "src/rmg_terrain.cpp"
    if len(parents) != 10 or not any(texts == originals for _, texts in parents):
        raise ValueError("review ten proxy/grid parents including the unchanged source")
    original = grid_definition(originals[HEADER])
    surrounding = originals[HEADER].replace(original, "")
    proxy = proxy_module()
    constructor = proxy.constructor_definition(originals[source_name])
    source_surrounding = originals[source_name].replace(constructor, "")
    anchors = (
        ("before_proxy", "TRmgLinePainterTile::TRmgLinePainterTile(\n"),
        ("before_refresh", "// Retail 0x4f9f00: preserve the original tile proxy across neighbour queries,"),
        ("after_refresh", "// Retail 0x4fa050: thiscall with a hidden twelve-byte return and point"),
        ("before_line_point", "// The line walk's shared point operation first removes an old line, assigns"),
        ("after_line_point", "// Refresh 0x4f9f77 copies the translated grid value before passing it to"),
    )
    if (any(originals[source_name].count(anchor) != 1 for _, anchor in anchors)
            or "TRmgGridPoint::operator+(" in originals[source_name]):
        raise ValueError("review the single grid translation and line-cluster source boundaries")
    options = []
    for label, texts in sorted(parents, key=lambda row: row[1] != originals):
        point = grid_definition(texts[HEADER])
        proxy_constructor = proxy.constructor_definition(texts[source_name])
        if (texts[HEADER].replace(point, "") != surrounding
                or texts[source_name].replace(proxy_constructor, "") != source_surrounding):
            raise ValueError("review parent changes outside the grid class and proxy constructor")
        method, declaration, ordinary = translation_method(point)
        proxy_edit = dict(source=source_name, find=constructor, replace=proxy_constructor)
        options.append(dict(name=label + "/in_class", replace=point, extra_edits=[proxy_edit]))
        options.extend(dict(name=label + "/ordinary_" + name, replace=point.replace(method, declaration),
                            extra_edits=[proxy_edit, dict(source=source_name, insert_before=anchor,
                                                         text=ordinary + "\n\n")]) for name, anchor in anchors)
    return dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__ +
                "\nFollow-up: grid translation has no Dreamcast-proven inline declaration. Compare "
                "the current in-class definition with one ordinary body in the painting TU at five "
                "real line-cluster boundaries, carrying ten reproduced grid/proxy parents. Retail "
                "refresh/line paintPoint expand translation but retain nested compound-add, copy and "
                "proxy operations differently. The already admitted ordinary += body at 0x4fa540 "
                "is unchanged. Do not paste either arithmetic body into callers, add a false inline "
                "qualifier, suppress automatic inlining, or remove the explicit copy interface.",
                axes=[dict(name="proxy_grid_parent_visibility", source=HEADER, find=original, options=options)])


def load_binding_parents(checkpoint_path, originals):
    parents = dict(load_temporary_parents(checkpoint_path, originals))
    checkpoint = json.loads(checkpoint_path.read_text())
    baseline = next(row for row in checkpoint["records"] if not any(row["choices"]))
    # The next axis already crosses every proxy construction. Keep distinct
    # grid headers, not multiple parents differing only in that construction.
    unique = {}
    for row in sorted(checkpoint["elites"], key=rank, reverse=True):
        header_hash = row["source_hashes"][HEADER]
        if header_hash != baseline["source_hashes"][HEADER] and row["id"] in parents:
            unique.setdefault(header_hash, row)
    selected = select_elites(list(unique.values()), 4, baseline["scores"])
    if len(selected) != 4:
        raise ValueError("review four reproduced, distinct grid parents plus the baseline")
    return [("baseline", originals), *((row["id"], parents[row["id"]]) for row in selected)]


def make_binding_manifest(originals, parents):
    source_name = "src/rmg_terrain.cpp"
    if (len(parents) != 5 or len({texts[HEADER] for _, texts in parents}) != 5
            or not any(texts == originals for _, texts in parents)):
        raise ValueError("review five distinct grid parents including the unchanged source")
    original = grid_definition(originals[HEADER])
    proxy = proxy_module()
    constructor = proxy.constructor_definition(originals[source_name])
    declaration = proxy.constructor_declaration(originals[source_name])
    surrounding = originals[HEADER].replace(original, "").replace(declaration, "")
    source_surrounding = originals[source_name].replace(constructor, "")
    options = []
    for label, texts in sorted(parents, key=lambda row: row[1] != originals):
        point = grid_definition(texts[HEADER])
        proxy_constructor = proxy.constructor_definition(texts[source_name])
        proxy_declaration = proxy.constructor_declaration(texts[source_name])
        if (texts[HEADER].count(proxy_declaration) != 1
                or texts[HEADER].replace(point, "").replace(proxy_declaration, "") != surrounding
                or texts[source_name].replace(proxy_constructor, "") != source_surrounding):
            raise ValueError("review parent changes outside the grid class and proxy constructor")
        forms = [(binding + "+" + name, parameter, body)
                 for binding, parameter in (("reference", "const TRmgGridPoint&"), ("value", "TRmgGridPoint"))
                 for name, body in proxy.constructor_forms(parameter)]
        if not any(body == proxy_constructor for _, _, body in forms):
            raise ValueError("review the current proxy construction before changing its binding")
        for name, parameter, body in sorted(forms, key=lambda row: row[2] != proxy_constructor):
            prototype = f"    TRmgLinePainterTile(TRmgLinePainterInterface* painter, {parameter} point);"
            options.append(dict(name=label + "/" + name, replace=point, extra_edits=[
                dict(source=HEADER, find=declaration, replace=prototype),
                dict(source=source_name, find=constructor, replace=body)]))
    return dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__ +
                "\nFollow-up: select four distinct grid headers from reproduced aggregate/specialist "
                "parents, plus the unchanged control, and cross all twelve proxy parameter/member "
                "constructions. The retained at(const grid&) ABI at 0x4fa050 stays fixed (hidden "
                "return plus point reference, ret 8). Only its expanded inner constructor has no "
                "separately proved parameter ABI; copy its point by value or bind it by const reference. "
                "Change that declaration and ordinary definition together, preserving copied-coordinate "
                "ownership and all factory/caller interfaces. The preceding ordinary-translation "
                "visibility control changed no tracked score, so use its pre-visibility parents.",
                axes=[dict(name="grid_parent_proxy_binding", source=HEADER, find=original, options=options)])


def factory_returns():
    yield from proxy_module().return_forms()
    signature = "TRmgLinePainterTile TRmgLinePainterInterface::at(const TRmgGridPoint& point)\n{\n"
    yield "temporary_reference", signature + ("    const TRmgLinePainterTile& tile = TRmgLinePainterTile(this, point);\n"
                                               "    return tile;\n}")
    yield "named_reference", signature + ("    TRmgLinePainterTile tile(this, point);\n"
                                          "    const TRmgLinePainterTile& result = tile;\n"
                                          "    return result;\n}")


def make_factory_manifest(originals, parents):
    source_name = "src/rmg_terrain.cpp"
    if len(parents) != 10 or not any(texts == originals for _, texts in parents):
        raise ValueError("review ten proxy/grid parents including the unchanged source")
    original = grid_definition(originals[HEADER])
    proxy = proxy_module()
    constructor = proxy.constructor_definition(originals[source_name])
    declaration = proxy.constructor_declaration(originals[source_name])
    factory = proxy.helpers().definition(originals[source_name], "TRmgLinePainterInterface::at")
    surrounding = originals[HEADER].replace(original, "").replace(declaration, "")
    source_surrounding = originals[source_name].replace(constructor, "").replace(factory, "")
    options = []
    for label, texts in sorted(parents, key=lambda row: row[1] != originals):
        point = grid_definition(texts[HEADER])
        proxy_constructor = proxy.constructor_definition(texts[source_name])
        proxy_declaration = proxy.constructor_declaration(texts[source_name])
        proxy_factory = proxy.helpers().definition(texts[source_name], "TRmgLinePainterInterface::at")
        if (texts[HEADER].count(proxy_declaration) != 1
                or texts[HEADER].replace(point, "").replace(proxy_declaration, "") != surrounding
                or texts[source_name].replace(proxy_constructor, "").replace(proxy_factory, "") != source_surrounding):
            raise ValueError("review parent changes outside the grid class, proxy constructor and factory")
        forms = list(factory_returns())
        if proxy_factory not in dict(forms).values():
            raise ValueError("review the current factory return lifetime")
        for name, body in sorted(forms, key=lambda row: row[1] != proxy_factory):
            options.append(dict(name=label + "/" + name, replace=point, extra_edits=[
                dict(source=HEADER, find=declaration, replace=proxy_declaration),
                dict(source=source_name, find=constructor, replace=proxy_constructor),
                dict(source=source_name, find=factory, replace=body)]))
    return dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__ +
                "\nFollow-up: all reproduced proxy-binding elites retain the original reference "
                "parameter. Keep both caller at() expressions and the canonical value constructor. "
                "The retained factory 0x4fa050 has distinct saved destination/return registers, while "
                "the current direct return folds the copy differently. Cross ten grid/proxy parents "
                "with six returned-value lifetimes, including a const-reference-bound temporary and "
                "a reference to the named constructed value. The function still returns by value, "
                "so the copy finishes before any local temporary dies. No caller bypass, new "
                "destructor, dummy operation or inline qualifier is introduced.",
                axes=[dict(name="grid_proxy_parent_factory_return", source=HEADER, find=original, options=options)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parents_group = parser.add_mutually_exclusive_group()
    parents_group.add_argument("--parents-from", type=Path, help="cross six reproduced constructor parents with ten translations")
    parents_group.add_argument("--member-orders-from", type=Path, help="cross ten reproduced parents with six member-definition orders")
    parents_group.add_argument("--assignment-from", type=Path, help="cross ten reproduced parents with six copy-assignment forms")
    parents_group.add_argument("--proxy-from", type=Path, help="cross ten reproduced parents with six proxy member constructions")
    parents_group.add_argument("--temporaries-from", type=Path, help="carry ten proxy/grid parents through six returned-temporary forms")
    parents_group.add_argument("--visibility-from", type=Path, help="cross ten proxy/grid parents with in-class or ordinary translation visibility")
    parents_group.add_argument("--binding-from", type=Path, help="cross five distinct grid parents with twelve proxy parameter/construction forms")
    parents_group.add_argument("--factory-from", type=Path, help="cross ten proxy/grid parents with six factory return lifetimes")
    args = parser.parse_args()
    header = (HOMM3_DIR / HEADER).read_text()
    if args.factory_from:
        originals = {HEADER: header, "src/rmg_terrain.cpp": (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()}
        payload = make_factory_manifest(originals, load_temporary_parents(args.factory_from, originals))
        payload["parent_checkpoint"] = str(args.factory_from.resolve())
    elif args.binding_from:
        originals = {HEADER: header, "src/rmg_terrain.cpp": (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()}
        payload = make_binding_manifest(originals, load_binding_parents(args.binding_from, originals))
        payload["parent_checkpoint"] = str(args.binding_from.resolve())
    elif args.visibility_from:
        originals = {HEADER: header, "src/rmg_terrain.cpp": (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()}
        payload = make_visibility_manifest(originals, load_temporary_parents(args.visibility_from, originals))
        payload["parent_checkpoint"] = str(args.visibility_from.resolve())
    elif args.temporaries_from:
        originals = {HEADER: header, "src/rmg_terrain.cpp": (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()}
        payload = make_temporary_manifest(originals, load_temporary_parents(args.temporaries_from, originals))
        payload["parent_checkpoint"] = str(args.temporaries_from.resolve())
    elif args.proxy_from:
        payload = make_proxy_manifest(header, (HOMM3_DIR / "src/rmg_terrain.cpp").read_text(),
                                      load_member_order_parents(args.proxy_from, header))
        payload["parent_checkpoint"] = str(args.proxy_from.resolve())
    elif args.assignment_from:
        payload = make_assignment_manifest(header, load_member_order_parents(args.assignment_from, header))
        payload["parent_checkpoint"] = str(args.assignment_from.resolve())
    elif args.member_orders_from:
        payload = make_member_order_manifest(header, load_member_order_parents(args.member_orders_from, header))
        payload["parent_checkpoint"] = str(args.member_orders_from.resolve())
    elif args.parents_from:
        checkpoint = json.loads(args.parents_from.read_text())
        parent_dir = args.parents_from.parent
        parent_input = json.loads((parent_dir / "input.json").read_text())
        if parent_input.get("units") != ["rmg", "rmg_support", "rmg_terrain"]:
            raise ValueError("review the three-TU constructor checkpoint")
        for relative in (HEADER, "src/rmg.cpp", "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
            if (parent_dir / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
                raise ValueError("review the constructor parents against the current source: " + relative)
        baseline = next((row for row in checkpoint["records"] if row["choices"] == [0, 0, 0]), None)
        elites = checkpoint["elites"]
        if (not baseline or len(elites) != 6
                or baseline["object_hash"] != elites[0]["object_hash"]):
            raise ValueError("review the six reproduced parents and the baseline-equivalent leader")
        labels = [baseline["labels"], *(row["labels"] for row in elites[1:])]
        payload = make_parent_manifest(header, [(row["copy_constructor"], row["coordinate_constructor"]) for row in labels])
        payload["parent_checkpoint"] = str(args.parents_from.resolve())
    else:
        payload = make_manifest(header)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated 60 grid-constructor states ->", args.output)


if __name__ == "__main__":
    main()
