"""Test the Dreamcast-proven mutable cell -> zCell boundary without a TU fork.

DC MapCell.h:897 (advmgr.obj:0x1f9c8) calls the private helper at 0x1f974,
whose line 850 computes cellData + x + y*size + z*size*size. Retail's
retained cell at 0x408770 expands that arithmetic. Compare the canonical
header pair with the current flattened, TU-forked and auto-inline-pinned
implementation. No option adds a pragma or changes the public signature.

Use --current after adopting the canonical header. The other switches replay
the inspected historical fork and intentionally refuse an already changed
boundary; they never recreate the fork in authored source.
"""
import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFINE = "#define HOMM3_NEWFULLMAP_CELL_OUTOFLINE  // owns the 0x408770 COMDAT copy of cell(x,y,z)\n"
BODY = """#pragma auto_inline(off)
VA(0x00408770, 0x31)  // anchor-callee, dc 0x1f9c8
NewmapCell* NewfullMap::cell(int x, int y, int z)
{
    return &m_cellData[(z * m_size + y) * m_size + x];
}
#pragma auto_inline(on)"""
HEADER = """#ifdef HOMM3_NEWFULLMAP_CELL_OUTOFLINE
    NewmapCell* cell(int x, int y, int z);
#else
    NewmapCell* cell(int x, int y, int z)
    {
        return &m_cellData[(z * m_size + y) * m_size + x];
    }
#endif"""
POINT_CALL = """    NewmapCell* cell(type_point point)
    {
        return cell(point.m_x, point.m_y, point.m_z);
    }"""
CONST_CALL = """    const NewmapCell* cell(int x, int y, int z) const
    {
        return zCell(x, y, z);
    }"""


def make_manifest():
    for name, anchor in (("src/advmgr.cpp", BODY), ("src/advmgr.cpp", DEFINE),
                         ("include/game.h", HEADER)):
        if (ROOT / name).read_text().count(anchor) != 1:
            raise ValueError("Review changed cell boundary: " + name)
    unpinned = BODY.replace("#pragma auto_inline(off)\n", "").replace(
        "\n#pragma auto_inline(on)", "")
    claimed = "#if 0  // claim-only home for the canonical MapCell.h inline\n" + unpinned + "\n#endif"
    options = [dict(name="unchanged"), dict(name="remove-auto-inline-only", replace=unpinned)]
    flat = HEADER.split("#else\n", 1)[1].split("\n#endif", 1)[0]
    for name, statement in (
        ("header-flat-control", None),
        ("header-zcell-pointer-sum", "return m_cellData + x + y * m_size + z * m_size * m_size;"),
        ("header-zcell-factored-index", "return &m_cellData[(z * m_size + y) * m_size + x];"),
        ("header-zcell-named-index", "int index = x + y * m_size + z * m_size * m_size;\n        return &m_cellData[index];"),
    ):
        header = flat if statement is None else (
            "private:\n"
            "    // DC MapCell.h:850: mutable partner of the const zCell below.\n"
            "    NewmapCell* zCell(int x, int y, int z)\n"
            "    {\n        " + statement + "\n    }\n"
            "public:\n"
            "    NewmapCell* cell(int x, int y, int z)\n"
            "    {\n        return zCell(x, y, z);\n    }")
        options.append(dict(name=name, replace=claimed, extra_edits=[
            dict(source="src/advmgr.cpp", find=DEFINE, replace=""),
            dict(source="include/game.h", find=HEADER, replace=header),
        ]))
    return dict(schema=1, source="src/advmgr.cpp",
                units=["advmgr", "viewwrld", "hero", "game", "philai", "seerhut", "initialize", "command"],
                evidence=__doc__, axes=[dict(name="cell-boundary", find=BODY, options=options)])


def make_bounds_manifest():
    manifest = make_manifest()
    header = (ROOT / "include/game.h").read_text()
    for anchor in (POINT_CALL, CONST_CALL):
        if header.count(anchor) != 1:
            raise ValueError("Review changed coordinate wrapper before probing bounds")
    options = [dict(name="unchanged")]
    xyz = "x >= 0 && x < m_size && y >= 0 && y < m_size && z >= 0 && z <= m_hasTwoLevels"
    xy = "x >= 0 && x < m_size && y >= 0 && y < m_size"
    checks = [
        ("unchecked", ""),
        ("xy-bounds", "        HOMM3_RELEASE_VERIFY(" + xy + ");\n"),
        ("xyz-bounds", "        HOMM3_RELEASE_VERIFY(" + xyz + ");\n"),
        ("split-xyz-bounds", "        HOMM3_RELEASE_VERIFY(x >= 0 && x < m_size);\n"
         "        HOMM3_RELEASE_VERIFY(y >= 0 && y < m_size);\n"
         "        HOMM3_RELEASE_VERIFY(z >= 0 && z <= m_hasTwoLevels);\n"),
    ]
    for name, check in checks:
        for point_direct in (False, True):
            source_option = json.loads(json.dumps(manifest["axes"][0]["options"][3]))
            source_option["name"] = name + ("-point-zcell" if point_direct else "-point-cell-control")
            if check:
                check_comment = ("        // Probe: cell callers retain 0x408770. DC MapCell.h:896\n"
                                 "        // is a one-line gap before zCell; unchecked/flattened\n"
                                 "        // variants are controls, not assertion-source proof.\n")
                source_option["extra_edits"][1]["replace"] = source_option["extra_edits"][1]["replace"].replace(
                    "        return zCell(x, y, z);", check_comment + check + "        return zCell(x, y, z);")
            if point_direct:
                source_option["extra_edits"].append(dict(
                    source="include/game.h", find=POINT_CALL,
                    replace=POINT_CALL.replace("return cell(", "return zCell(")))
            options.append(source_option)
            if name == "xyz-bounds" and point_direct:
                paired = json.loads(json.dumps(source_option))
                paired["name"] = "xyz-bounds-both-const-overloads-point-zcell"
                paired["extra_edits"].append(dict(source="include/game.h", find=CONST_CALL,
                    replace=CONST_CALL.replace("        return zCell", check_comment + check + "        return zCell")))
                options.append(paired)
    manifest["axes"][0]["options"] = options
    manifest["evidence"] += (
        "\nFollow-up: DC MapCell.h:907 calls zCell directly from cell(type_point), "
        "not the scalar cell wrapper. Scalar const/mutable rows have one-line gaps "
        "890/896 before zCell. Test their real coordinate-domain invariant, without "
        "assuming a gap proves assertions. All helper declarations remain canonical.")
    return manifest


def make_ordered_manifest():
    manifest = make_manifest()
    old = HEADER + """
private:
    // DC MapCell.h:847, dc 0xbc8dc: canonical const map-index helper.
    const NewmapCell* zCell(int x, int y, int z) const
    {
        return m_cellData + x + y * m_size + z * m_size * m_size;
    }
public:
    // DC MapCell.h:889, dc 0xbc930: forwards to the private zCell helper.
""" + CONST_CALL + "\n" + POINT_CALL
    if (ROOT / "include/game.h").read_text().count(old) != 1:
        raise ValueError("Review changed MapCell.h method ordering")
    options = [dict(name="unchanged")]
    for label, check in (
        ("unchecked", ""),
        ("xy", "        HOMM3_RELEASE_VERIFY(x >= 0 && x < m_size && y >= 0 && y < m_size);\n"),
        ("xyz", "        HOMM3_RELEASE_VERIFY(x >= 0 && x < m_size && y >= 0 && y < m_size && z >= 0 && z <= m_hasTwoLevels);\n"),
    ):
        for both in (False, True) if check else (False,):
            option = json.loads(json.dumps(manifest["axes"][0]["options"][3]))
            option["name"] = "dc-method-order-" + label + ("-both-scalars" if both else "-mutable")
            comment = ("        // Probe: cell callers retain 0x408770; DC MapCell.h:890/896\n"
                       "        // gaps permit bounds verification, not recovered assert text.\n"
                       "        // Unchecked canonical wrappers are the negative control.\n") if check else ""
            const_check = comment + check if both else ""
            ordered = """private:
    // DC MapCell.h:847/850: const zCell precedes mutable zCell.
    const NewmapCell* zCell(int x, int y, int z) const
    {
        return m_cellData + x + y * m_size + z * m_size * m_size;
    }
    NewmapCell* zCell(int x, int y, int z)
    {
        return m_cellData + x + y * m_size + z * m_size * m_size;
    }
public:
    // DC MapCell.h:889/895/906: const, mutable, packed-point wrappers.
    const NewmapCell* cell(int x, int y, int z) const
    {
""" + const_check + """        return zCell(x, y, z);
    }
    NewmapCell* cell(int x, int y, int z)
    {
""" + comment + check + """        return zCell(x, y, z);
    }
    NewmapCell* cell(type_point point)
    {
        return zCell(point.m_x, point.m_y, point.m_z);
    }"""
            option["extra_edits"][1] = dict(source="include/game.h", find=old, replace=ordered)
            options.append(option)
    manifest["axes"][0]["options"] = options
    manifest["evidence"] += ("\nDC method order is const/mutable zCell at 847/850, "
        "const/mutable scalar cell at 889/895, and packed-point cell at 906. "
        "The packed-point wrapper calls zCell directly, as its DC call proves.")
    return manifest


def make_storage_manifest():
    manifest = make_ordered_manifest()
    unchecked = manifest["axes"][0]["options"][1]
    options = [dict(name="unchanged"), unchecked]
    mutable = """    NewmapCell* cell(int x, int y, int z)
    {
        return zCell(x, y, z);
    }"""
    const = mutable.replace("NewmapCell* cell", "const NewmapCell* cell").replace(
        "int z)\n", "int z) const\n")
    for name, predicate in (("storage", "m_cellData != 0"),
                            ("initialized-map", "m_cellData != 0 && m_size > 0")):
        for both in (False, True):
            option = json.loads(json.dumps(unchecked))
            option["name"] = name + ("-both-scalars" if both else "-mutable")
            header = option["extra_edits"][1]["replace"]
            check = ("        // Probe: scalar cell callers retain 0x408770; DC 890/896\n"
                     "        // has a gap before indexing. Storage must exist; unchecked\n"
                     "        // wrappers are the negative control, not ASSERT-text proof.\n"
                     "        HOMM3_RELEASE_VERIFY(" + predicate + ");\n")
            for anchor in (mutable, const) if both else (mutable,):
                if header.count(anchor) != 1:
                    raise ValueError("Review scalar wrapper before testing its storage invariant")
                header = header.replace(anchor, anchor.replace("        return zCell", check + "        return zCell"))
            option["extra_edits"][1]["replace"] = header
            options.append(option)
    manifest["axes"][0]["options"] = options
    manifest["evidence"] += "\nTest the actual initialized-map precondition as well as coordinate bounds."
    return manifest


def make_current_manifest():
    source = (ROOT / "src/advmgr.cpp").read_text()
    header = (ROOT / "include/game.h").read_text()
    marker = "VA(0x00408770,"
    if source.count(marker) != 1 or DEFINE in source or HEADER in header:
        raise ValueError("Review canonical cell ownership before current controls")
    begin = source.index(marker)
    closing = "    return zCell(x, y, z);\n}"
    end = source.index(closing, begin) + len(closing)
    body = source[begin:end]
    check = "HOMM3_RELEASE_VERIFY(m_cellData != 0);"
    if body.count(check) != 1 or header.count(check) != 1:
        raise ValueError("Review changed cell storage precondition")
    options = [dict(name="unchanged")]
    for label, replacement in (
        ("unchecked-canonical-wrappers", ""),
        ("initialized-map", "HOMM3_RELEASE_VERIFY(m_cellData != 0 && m_size > 0);"),
        ("coordinate-bounds", "HOMM3_RELEASE_VERIFY(x >= 0 && x < m_size && y >= 0 && y < m_size);"),
    ):
        options.append(dict(name=label, replace=body.replace(check, replacement),
                            extra_edits=[dict(source="include/game.h", find=check,
                                              replace=replacement)]))
    return dict(schema=1, source="src/advmgr.cpp",
                units=["advmgr", "viewwrld", "hero", "game", "philai", "seerhut", "initialize", "command"],
                evidence=__doc__, axes=[dict(name="canonical-cell-precondition", find=body, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--bounds", action="store_true")
    parser.add_argument("--ordered", action="store_true")
    parser.add_argument("--storage", action="store_true")
    parser.add_argument("--current", action="store_true")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if sum((args.bounds, args.ordered, args.storage, args.current)) > 1:
        parser.error("select one family mode")
    manifest = make_current_manifest() if args.current else make_storage_manifest() if args.storage else make_ordered_manifest() if args.ordered else make_bounds_manifest() if args.bounds else make_manifest()
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
