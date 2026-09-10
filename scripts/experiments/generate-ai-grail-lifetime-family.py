"""Natural Grail artifact construction and operand lifetimes.

Run after restoring the reference interfaces and game accessor. DC
ai_player.obj:0x32e30 line 3205 constructs type_artifact(TArtifact) and calls
the player-level AI_get_value_of_artifact overload in the same statement.
Its DC 0x37514 body and Complete 0x433aa0 both take const type_artifact& and
long player_id, floor the value at 10 and walk the player's heroes. The
two-argument reconstruction constructor and a named artifact are controls;
typed named/temporary/reference bindings preserve the same two fields.
Player pointer/reference and friendly-distance value/condition bindings
test real operand lifetimes without changing guards or helper definitions.
The inherited valuation pin occurs only in the unchanged control forms.
"""

import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 2]


def helper_forms(original):
    artifact_start = original.index("                            type_artifact grail(")
    artifact_end = original.index("                        }\n", artifact_start)
    old_artifact = original[artifact_start:artifact_end]
    named = """                            type_artifact grail(ARTIFACT_HOLY_GRAIL, -1);
                            point.m_value = aiGetValueOfArtifact(
                                grail, currentHero->m_owner);
"""
    temporary = """                            point.m_value = aiGetValueOfArtifact(
                                type_artifact(ARTIFACT_HOLY_GRAIL, -1),
                                currentHero->m_owner);
"""
    reference = """                            const type_artifact& grail =
                                type_artifact(ARTIFACT_HOLY_GRAIL);
                            point.m_value = aiGetValueOfArtifact(
                                grail, currentHero->m_owner);
"""
    artifacts = [
        ("fenced-named-control", old_artifact),
        ("natural-named-two-arguments", named),
        ("natural-temporary-two-arguments", temporary),
        ("natural-named-typed", named.replace("ARTIFACT_HOLY_GRAIL, -1", "ARTIFACT_HOLY_GRAIL")),
        ("natural-temporary-typed", temporary.replace("ARTIFACT_HOLY_GRAIL, -1", "ARTIFACT_HOLY_GRAIL")),
        ("natural-reference-typed", reference),
    ]
    friendly = """                    unsigned short friendlyCost = friendlyDistances[
                        (point.m_point.m_z * g_mapHeight
                         + point.m_point.m_y)
                            * g_mapWidth
                        + point.m_point.m_x];
                    if (point.m_moveCost <= friendlyCost) {"""
    direct = """                    if (point.m_moveCost <= friendlyDistances[
                            (point.m_point.m_z * g_mapHeight
                             + point.m_point.m_y) * g_mapWidth
                            + point.m_point.m_x]) {"""
    if original.count(friendly) != 1:
        raise ValueError("Review the friendly-distance guard")
    player = "    playerData* player = &g_game->m_players[currentHero->m_owner];"
    if original.count(player) != 1:
        raise ValueError("Review the player binding")
    for (name, artifact), player_ref, direct_distance in itertools.product(
            artifacts, (False, True), (False, True)):
        body = original.replace(old_artifact, artifact)
        if player_ref:
            body = body.replace(player, "    playerData& player = g_game->m_players[currentHero->m_owner];")
            body = body.replace("player->", "player.")
        if direct_distance:
            body = body.replace(friendly, direct)
        yield f"{name}/player-reference={player_ref}/direct-distance={direct_distance}", body


def make_manifest():
    source = (HOMM3_DIR / "src/ai_player.cpp").read_text()
    original = function(source, "static void checkHolyGrail(")
    options = []
    for name, body in helper_forms(original):
        option = {"name": name}
        if body != original:
            option["replace"] = body
        options.append(option)
    return {
        "schema": 1,
        "source": "src/ai_player.cpp",
        "units": ["ai_player"],
        "evidence": __doc__,
        "axes": [{"name": "grail-operand-lifetimes", "find": original, "options": options}],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
