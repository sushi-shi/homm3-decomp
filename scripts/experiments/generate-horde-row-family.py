"""Finite row-bounded horde traversal hypotheses; DC town.cpp:938..960.

Keep Complete's nine towns, inclusive slot-seven search, and store order.
DC positively supplies short indices, even entry steps, a creature snapshot,
and a separate upgraded-entry address. Row cursors are a PC traversal
hypothesis, confined to one actual four-element array at a time.
"""
import itertools
import json
from pathlib import Path
import sys

source = Path("src/town.cpp").read_text()
start = source.index("void town::initializeHordes()")
end = source.index("\n}\n", start) + 2
original = source[start:end]
options = [{"name": "unchanged"}]
for traversal, town_type, capture, upgraded in itertools.product(
    ("pair-index", "even-index", "row-cursor"),
    ("int", "short"), (False, True), (False, True)
):
    if traversal == "row-cursor":
        loop = """        type_horde_effect* effect = s_constHordeEffects[townType];
        for (int pair = 0; pair < 2; pair++, effect += 2) {"""
    elif traversal == "even-index":
        loop = """        for (short entry = 0; entry < 4; entry += 2) {
            type_horde_effect* effect = &s_constHordeEffects[townType][entry];"""
    else:
        loop = """        for (int pair = 0; pair < 2; pair++) {
            type_horde_effect* effect = &s_constHordeEffects[townType][2 * pair];"""
    declarations = ""
    if capture:
        declarations += "\n            TCreatureType creature = effect->m_creature;"
    if upgraded:
        declarations += "\n            type_horde_effect* upgrade = effect + 1;"
    test = "creature" if capture else "effect->m_creature"
    dest = "upgrade->" if upgraded else "effect[1]."
    body = f"""void town::initializeHordes()
{{
    int creatureBase = 0;
    for ({town_type} townType = 0; townType < TOWN_TYPE_COUNT; townType++) {{
{loop}{declarations}
            short slot;
            for (slot = 0; slot <= TOWN_DWELLING_COUNT; slot++) {{
                if ({test} == g_townDwellingCreatures[creatureBase + slot])
                    break;
            }}
            if (slot <= TOWN_DWELLING_COUNT) {{
                effect->m_dwelling = slot;
                slot += TOWN_DWELLING_COUNT;
                {dest}m_creature = g_townDwellingCreatures[creatureBase + slot];
                {dest}m_dwelling = slot;
                const short* bonus = &effect->m_bonus;
                {dest}m_bonus = *bonus;
            }}
        }}
        creatureBase += 2 * TOWN_DWELLING_COUNT;
    }}
}}"""
    if body == original:
        continue
    options.append({"name": f"{traversal}-{town_type}-snapshot{int(capture)}-upgrade{int(upgraded)}",
                    "replace": body})
manifest = {"schema": 1, "source": "src/town.cpp", "units": ["town"],
            "axes": [{"name": "row-traversal", "find": original, "options": options}]}
Path(sys.argv[1]).write_text(json.dumps(manifest, indent=2) + "\n")
print(f"Wrote {len(options)} states to {sys.argv[1]}")
