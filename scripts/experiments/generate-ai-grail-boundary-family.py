#!/usr/bin/env python3
"""Test the evidenced Grail helper interfaces and natural call boundaries.

Run against the pre-adoption 182b7a26 source. By default preserve the scored
function's mangled name (18 states). --signature-controls reproduces the
original 36-state diagnostic manifest, but its reference-signature states
cannot be ranked: the runner still looks up the old pointer-signature row
and reports zero. Verify those interface migrations through a full build,
which regenerates labels and delinks the correctly named target.
"""

import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--signature-controls", action="store_true")
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/ai_player.cpp").read_text()
    helper = source[source.index("static void checkHolyGrail("):]
    helper = helper[:helper.index("\n// Residual (96.3651%")]
    cell_start = helper.index("            // Dreamcast calls game::get_cell here")
    cell_end = helper.index("            if (!(mapCell->m_type")
    cell = helper[cell_start:cell_end]
    direct_cell = """            NewmapCell* mapCell = g_game->m_worldMap.cell(
                destination.m_point.m_x, destination.m_point.m_y,
                destination.m_point.m_z);
"""
    artifact_start = helper.index("                            type_artifact grail(")
    artifact_end = helper.index("                        }\n", artifact_start)
    artifact = helper[artifact_start:artifact_end]
    natural_artifact = """                            type_artifact grail(ARTIFACT_HOLY_GRAIL, -1);
                            destination.m_value = aiGetArtifactPlayerValue(
                                grail, currentHero->m_owner);
"""
    signature = """long findAllDestinations(hero* currentHero, searchArray* currentSearchArray,
                           std::vector<HeroDestination>* destinations,
"""
    payload = {
        "schema": 1,
        "source": "src/ai_player.cpp",
        "units": ["advmgr", "ai", "ai_combat", "ai_player", "philai"],
        "evidence": (
            "DC check_holy_grail ai_player.obj:0x32e30 and find_all_destinations "
            "0x33038 both take vector<HeroDestination>&. DC line 3181 calls "
            "game::get_cell; the current canonical header body delegates to "
            "NewfullMap::cell(int,int,int), the call retained by retail 0x42edd0 "
            "at +0x694. Thus the historical comment rejecting the wrapper "
            "based on an older accessor body must be retested. DC line 3205 "
            "constructs the artifact temporary and calls its valuation helper "
            "in one statement; Complete retains aiGetArtifactPlayerValue. "
            "DC line 3235 calls game::GetNumMapLevels. Preserve all branches, "
            "coordinate assignments, real helper definitions, and push_back. "
            "Only the inherited fences are control states: no added fences, "
            "false inline declarations or filler operations. All five current "
            "ai_player.h consumers are scored; source names are unchanged "
            "during these independently isolated interface controls."
        ),
        "axes": [
            {
                "name": "map-cell-boundary",
                "find": cell,
                "options": [
                    {"name": "fenced-direct-control"},
                    {"name": "natural-direct", "replace": direct_cell},
                    {
                        "name": "canonical-game-accessor",
                        "replace": """            NewmapCell* mapCell = g_game->getCell(destination.m_point);
""",
                    },
                ],
            },
            {
                "name": "artifact-temporary-boundary",
                "find": artifact,
                "options": [
                    {"name": "fenced-named-control"},
                    {"name": "natural-named", "replace": natural_artifact},
                    {
                        "name": "natural-temporary",
                        "replace": """                            destination.m_value = aiGetArtifactPlayerValue(
                                type_artifact(ARTIFACT_HOLY_GRAIL, -1),
                                currentHero->m_owner);
""",
                    },
                ],
            },
            {
                "name": "destination-reference-interface",
                "find": """    const hero* currentHero, const searchArray* currentSearchArray,
    std::vector<HeroDestination>* destinations,
""",
                "options": [
                    {"name": "pointer-control"},
                    {
                        "name": "original-references",
                        "replace": """    const hero* currentHero, const searchArray* currentSearchArray,
    std::vector<HeroDestination>& destinations,
""",
                        "extra_edits": [
                            {"find": signature, "replace": signature.replace("* destinations", "& destinations")},
                            {"source": "include/ai_player.h", "find": signature, "replace": signature.replace("* destinations", "& destinations")},
                            {"find": "destinations->push_back(destination);", "replace": "destinations.push_back(destination);"},
                            {"find": "destinations->push_back(point);", "replace": "destinations.push_back(point);"},
                            {"find": "&destinations, maxDistance, 0,", "replace": "destinations, maxDistance, 0,"},
                            {"find": "findAllDestinations(candidate, currentSearchArray, &destinations, 0x7fff,", "replace": "findAllDestinations(candidate, currentSearchArray, destinations, 0x7fff,"},
                        ],
                    },
                ],
            },
            {
                "name": "map-level-accessor",
                "find": "int levelCells = g_game->m_worldMap.getNumLevels() * levelSize;",
                "options": [
                    {"name": "direct-map-control"},
                    {"name": "original-game-accessor", "replace": "int levelCells = g_game->getNumMapLevels() * levelSize;"},
                ],
            },
        ],
    }
    if not args.signature_controls:
        payload["axes"][2]["options"] = payload["axes"][2]["options"][:1]
    args.output.write_text(json.dumps(payload, indent=2) + "\n")


if __name__ == "__main__":
    main()
