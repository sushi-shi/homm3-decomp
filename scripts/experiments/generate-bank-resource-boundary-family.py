"""Restore the evidenced player-id resource-cost forwarding boundary.

DC 0x10f2f8, philai.cpp:1331, positively calls the player-pointer overload
from AI_resource_cost(long,const int*). Its current loop is hand-expanded.
DC bank 0x110808 calls that player-id overload with the hero owner at :2064.
Retail 0x529920 expands the resource walk; its two early dispatcher expansions
instead call the pointer overload. This is compatible with an expanded ordinary
id wrapper followed by distinct nested pointer-overload decisions; retail
does not refute the source-real boundary merely by retaining an inline walk.

Cross the canonical id wrapper, bank's call to it and the existing/removed
artifact-size fence. Preserve ordinary helper declarations, source order,
arithmetic, one canonical pointer loop, and all three bank calls. No false
inline declaration, duplicated helper, added assertion or new fence is used.
The prior 60-state lifetime family is exhausted at two score vectors; this
follow-up explores a newly established positive helper-boundary fact.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/philai.cpp").read_text()
    wrapper = """int aiResourceCost(long playerId, const int* resources)
{
    int value = 0;
    for (int resource = 0; resource < NUM_RESOURCES; resource++)
        value += resources[resource]
            * g_game->m_players[playerId].m_ai.m_resourceValue[resource];
    return value;
}"""
    forwarding = """int aiResourceCost(long playerId, const int* resources)
{
    return aiResourceCost(&g_game->m_players[playerId], resources);
}"""
    begin = source.index("inline long valueOfBank(")
    end = source.index("\n}", begin) + 2
    bank = source[begin:end]
    old_call = """    value += aiResourceCost(
        &g_game->m_players[currentHero->m_owner], bank.m_resources);"""
    new_call = "    value += aiResourceCost(currentHero->m_owner, bank.m_resources);"
    if source.count(wrapper) != 1 or bank.count(old_call) != 1:
        raise ValueError("Review the resource-cost overload and bank-call parents")
    options = []
    for call, unfenced in itertools.product(range(2), range(2)):
        candidate = bank.replace(old_call, new_call) if call else bank
        if unfenced:
            candidate = candidate.replace("#pragma inline_depth(0)\n", "").replace("\n#pragma inline_depth()", "")
        option = dict(name=("owner-id" if call else "pointer-control")
                      + ("-unfenced" if unfenced else "-existing-fence"))
        if candidate != bank:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source="src/philai.cpp", units=["philai"], evidence=__doc__, axes=[
        dict(name="resource-id-boundary", find=wrapper, options=[
            dict(name="duplicated-loop-control"), dict(name="ordinary-forwarding-call", replace=forwarding)]),
        dict(name="bank-call-and-fence", find=bank, options=options),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
