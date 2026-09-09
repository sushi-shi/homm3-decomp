"""Recover AI turn-close purchase and warning-string source boundaries.

DC end_turn (ai_player.obj:0x2e7d8), line 439, calls purchase_buildings;
its ordinary void/no-argument member at 0x31094 owns prohibited_creatures
and the fill_prohibited_array / purchase_building loop (1841..1843).
Retail 0x428dd0 expands that helper with Complete's 145-entry flag array.
DC line 493 calls format_string then string::operator+= and destroys the
temporary. Retail's warning arm passes the formatter's returned pointer to
the retained append(string, pos, count), not the old named-copy address.
DC's warning counter increments through a signed short; retail has two
independent ascending pointers and a seven-count backedge. Test those
equivalent bounded walks, the real string operation/temporary lifetimes,
the purchase boundary, and retail's unsigned-positive length guard.

The inherited append fence is a negative/control state only. No new inline
keyword, pragma, dummy operation, copied library body or helper is invented.
All current ai_player.h consumers are read from the full build's Ninja deps.
"""

import argparse
import itertools
import json
from pathlib import Path
import subprocess
import tomllib


ROOT = Path(__file__).resolve().parents[2]
PURCHASE = """    unsigned char prohibitedCreatures[145];
    fillProhibitedArray(&g_game->m_players[m_team], prohibitedCreatures);
    while (purchaseBuilding(prohibitedCreatures)) {
    }
"""
STUB = """// E:\\gamedcs\\ai_player.cpp:1838
DC_ONLY(0x31094, 0x60)
void type_AI_player::purchaseBuildings()
{
    // @stub
}
"""
HELPER = """// E:\\gamedcs\\ai_player.cpp:1838, dc 0x31094.
// Ordinary purchase_buildings boundary, called by end_turn at DC line 439.
// Complete expands it in 0x428dd0 and extends the prohibited flag table to 145.
// Before normalization (local): prohibited_creatures.
DC_ONLY(0x31094, 0x60)
void type_AI_player::purchaseBuildings()
{
""" + PURCHASE + "}\n"
DECLARATION = """    // Before normalization (function): type_AI_player::purchase_buildings.
    // Before normalization (locals): prohibited_creatures.
    unsigned char purchaseBuildings(unsigned char* prohibitedCreatures);"""


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 2]


def consumers():
    configured = {row["unit"] for row in tomllib.loads(
        (ROOT / "config/units.toml").read_text())["unit"]}
    output = subprocess.check_output(["ninja", "-t", "deps"], cwd=ROOT, text=True)
    selected = set()
    for block in output.split("\n\n"):
        lines = block.splitlines()
        if not lines or str(ROOT / "include/ai_player.h") not in (
                line.strip() for line in lines[1:]):
            continue
        if "(VALID)" not in lines[0]:
            raise ValueError("Refresh full-build dependencies: " + lines[0])
        unit = Path(lines[0].split(":", 1)[0]).stem
        if unit not in configured:
            raise ValueError("Unknown header consumer: " + unit)
        selected.add(unit)
    if not {"ai_player", "ai_combat", "advmgr"}.issubset(selected):
        raise ValueError("Missing current ai_player.h dependency closure")
    return sorted(selected)


def warning_forms(original):
    call = """formatString(
                g_aiResourceWarningFormat,
                *warningAmount,
                *warningName)"""
    append = """            std::string formatted = """ + call + ";\n" + """#pragma inline_depth(0)
            warning.append(formatted, 0, std::string::npos);
#pragma inline_depth()"""
    if original.count(append) != 1:
        raise ValueError("Review the warning append and its temporary")
    calls = [
        ("fenced-copy-control", append),
        ("natural-copy-append", append.replace("#pragma inline_depth(0)\n", "").replace(
            "\n#pragma inline_depth()", "")),
        ("copy-plus-equal", "            std::string formatted = " + call +
         ";\n            warning += formatted;"),
        ("const-reference-plus-equal", "            const std::string& formatted = " + call +
         ";\n            warning += formatted;"),
        ("temporary-plus-equal", "            warning += " + call + ";"),
    ]
    header = """    long* warningAmount = player->m_resources;
    const char** warningName = g_resourceNames;
    int warningCount = 7;
    do {
"""
    tail = """        warningAmount++;
        warningName++;
    } while (--warningCount);
"""
    if original.count(header) != 1 or original.count(tail) != 1:
        raise ValueError("Review the warning loop's bounds and cursor ownership")
    for loop, operation in itertools.product(("pointers", "int-index", "short-index"), calls):
        name, body = operation
        result = original.replace(append, body)
        if loop != "pointers":
            kind = loop.removesuffix("-index")
            result = result.replace(header, "    for (" + kind +
                " warningResource = 0; warningResource < 7; ++warningResource) {\n")
            result = result.replace(tail, "    }\n")
            result = result.replace("*warningAmount", "player->m_resources[warningResource]")
            result = result.replace("*warningName", "g_resourceNames[warningResource]")
        yield loop + "/" + name, result


def make_manifest():
    source = (ROOT / "src/ai_player.cpp").read_text()
    caller = function(source, "void type_AI_player::endTurn()")
    start = caller.index("    std::string warning;\n")
    end = caller.index("    if (warning.length())", start)
    original = caller[start:end]
    warnings = []
    for name, body in warning_forms(original):
        option = dict(name=name)
        if body != original:
            option["replace"] = body
        warnings.append(option)
    return dict(schema=1, source="src/ai_player.cpp", units=consumers(),
                evidence=__doc__, axes=[
        dict(name="warning-operation-and-walk", find=original, options=warnings),
        dict(name="purchase-helper-boundary", find=PURCHASE, options=[
            dict(name="pasted-purchase-control"),
            dict(name="ordinary-purchase-member", replace="    purchaseBuildings();\n",
                 extra_edits=[
                     dict(find=STUB, replace="#endif  // @carcass\n\n" + HELPER +
                          "\n#if 0  // @carcass\n"),
                     dict(source="include/ai_player.h", find=DECLARATION,
                          replace="    // Before normalization (function): type_AI_player::purchase_buildings.\n"
                          "    void purchaseBuildings();"),
                 ]),
        ]),
        dict(name="warning-length-guard", find="    if (warning.length())\n", options=[
            dict(name="truth-test-control"),
            dict(name="unsigned-positive", replace="    if (warning.length() > 0)\n"),
        ]),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    print("60 source states; consumers:", ", ".join(payload["units"]))
