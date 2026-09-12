#!/usr/bin/env python3
"""Verify all shipyard query forms against ordered independent probes."""
from pathlib import Path
import re
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator


def main():
    root = HOMM3_DIR
    module = generator("generate-rmg-shipyard-query-family.py")
    helpers = generator("generate-rmg-position-family.py")
    header = (root / "include/rmg.h").read_text()
    source = (root / "src/rmg.cpp").read_text()
    support = (root / "src/rmg_support.cpp").read_text()
    def block(name):
        start = header.index("struct " + name + " {")
        return header[start:header.index("\n};", start) + 3]
    types = "\n".join(block(n) for n in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgGroundTile", "TRmgGroundTileData"))
    imported = helpers.definition(source, "TRmgMapPosition::TRmgMapPosition")
    imported += "\n" + helpers.definition(source, "TRmgMapPosition::operator+=")
    # Select the value-position overload, not the separate two-scalar body.
    lookup = source.index("TRmgMapItem* type_random_map::getMapItem(TRmgMapPosition point)")
    imported += "\n" + source[lookup:source.index("\n}", lookup) + 2]
    accessor = re.search(r"inline TRmgMapItem\* getMapItem\(int x, int y, int z\)\s*\{[^}]+}", header)[0]
    accessor = accessor.replace("{", """{
        if (x < 0 || x >= m_size.m_x || y < 0 || y >= m_size.m_y || z < 0 || z > 1) {
            m_badRead = true;
            return &m_invalid;
        }
        m_reads.push_back((z * m_size.m_y + y) * m_size.m_x + x);""", 1)
    original = helpers.definition(source, "type_random_map_generator::canPlaceShipyard")
    alternatives = [("Source", original, True)]
    alternatives += [("Candidate" + str(i), body, True) for i, (_, body) in enumerate(module.forms(original))]
    for name, before, after in (
            ("WrongGate", "terrain == eTerrainWater && item->hasSubterraneanGate()", "terrain == eTerrainWater"),
            ("WrongWater", "if (item->m_tile.m_landType == eTerrainWater)", "if (item->m_tile.m_landType != eTerrainWater)"),
            ("WrongSide", "nearby.m_x -= 3;", "nearby.m_x -= 2;"),
            ("WrongFootprint", "position.m_x - 2", "position.m_x - 1")):
        assert before in original
        alternatives.append((name, original.replace(before, after), False))
    alternatives.append(("WrongLevel", original.replace("{", "{\n    position.m_z = 0;", 1), False))
    candidates, checks = [], []
    for name, body, good in alternatives:
        candidates.append("struct " + name + " : Root { unsigned char canPlaceShipyard(TRmgMapPosition); };\n" + body.replace("type_random_map_generator::", name + "::"))
        checks.append('if (' + ('!' if good else '') + 'check<' + name + '>()) { std::fprintf(stderr, "failed ' + name + '\\n"); return 1; }')
    offsets = re.search(r"TPoint g_rmgShipyardWaterOffsets\[[^]]+\] = \{.*?\n};", source, re.S)[0]
    offsets = "enum { RMG_SHIPYARD_WATER_OFFSET_COUNT = 4 };\n" + offsets
    program = (root / "scripts/experiments/rmg-shipyard-query-oracle.cpp").read_text()
    for marker, value in (("TYPES", types), ("OFFSETS", offsets), ("HELPERS", imported),
            ("GATE", re.search(r"unsigned char hasSubterraneanGate\(\) const\s*\{[^}]+}", header)[0]),
            ("ACCESSOR", accessor), ("CANDIDATES", "\n".join(candidates)), ("CHECKS", "\n".join(checks))):
        program = program.replace("// @" + marker + "@", value)
    with tempfile.TemporaryDirectory(prefix="rmg-shipyard-query-") as raw:
        path = Path(raw) / "oracle.cpp"
        path.write_text(program)
        exe = Path(raw) / "oracle"
        subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", "-I", str(root / "include"), str(path), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("61 shipyard forms x 4,800 scenarios; five negative controls rejected")


if __name__ == "__main__":
    main()
