"""Recover the native artifact pointer at the market entry/state boundary.

Dreamcast DoBlackMarket (dc 0x1886d4, tradpost.cpp:693) takes TArtifact*;
the public name and DispatchEvent's native TBlackMarket subscript agree.
Complete's 42-byte entry stores that pointer unchanged, and the buy panel
indexes dword artifact IDs, including the ARTIFACT_NONE sentinel. Both
current producers already own TArtifact arrays: game::m_marketArtifacts and
TBlackMarket::m_artifacts. The char* declaration and three-view pointer union
are leftover bootstrap ownership, not a surviving byte-buffer API.

The finite two-state control changes declaration, definition, global and all
readers/writers together, checking all five direct header consumers. The old
entry label may score zero until normal source-owned relabeling: compare its
actual body and caller relocation target under the evidenced signature rename.
No helper, cast, alternate view, or inline directive replaces the union.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def replace(source, old, new, count=1):
    if source.count(old) != count:
        raise ValueError("Review market ownership anchor: " + old)
    return source.replace(old, new)


def make_manifest():
    source = (ROOT / "src/tradpost.cpp").read_text()
    start = source.index("union TMarketArtifactList {\n")
    union = source[start:source.index("\n};\n", start) + 4]
    candidate = replace(source, union, "")
    candidate = replace(candidate, "static TMarketArtifactList g_marketArtifacts;",
                         "static TArtifact* g_marketArtifacts;")
    for view in ("m_asArtifacts", "m_asIds", "m_asBytes"):
        candidate = candidate.replace("g_marketArtifacts." + view, "g_marketArtifacts")
    candidate = replace(candidate, "void doBlackMarket(hero* inHero, char* blackArtifacts)",
                         "void doBlackMarket(hero* inHero, TArtifact* blackArtifacts)")
    candidate = replace(candidate, "g_marketArtifacts[i] != -1",
                         "g_marketArtifacts[i] != ARTIFACT_NONE")
    old_argument = ("static_cast<char*>(static_cast<void*>(\n"
                    "                              &g_game->m_blackMarkets[cell->m_extraInfo]))")
    return dict(
        schema=1, source="src/tradpost.cpp", evidence=__doc__,
        units=["tradpost", "events", "philai", "ai_player", "townmgr"],
        axes=[dict(name="artifact-pointer-owner", find=source,
                   options=[dict(name="unchanged"),
                            dict(name="native-artifact-array", replace=candidate,
                                 extra_edits=[
                                     dict(source="include/tradpost.h",
                                          find="void doBlackMarket(hero* inHero, char* blackArtifacts);",
                                          replace="void doBlackMarket(hero* inHero, TArtifact* blackArtifacts);"),
                                     dict(source="src/events.cpp", find=old_argument,
                                          replace="g_game->m_blackMarkets[cell->m_extraInfo].m_artifacts")])])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
