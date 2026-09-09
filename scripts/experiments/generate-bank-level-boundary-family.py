"""Restore the ordinary bank-level reader before its loader caller.

DC dc:0x70fe0 (creature_bank.cpp:32) is a static ordinary helper taking
type_creature_bank_level& and const vector<char*>&. The loader calls it
at :136 after installing guard/reward types. Retail 0x47ab30 expands the
reader; absence of an out-of-line retail call does not justify pasting it.

Cross the untouched pasted control and four real helper implementations
with the guard-table copy's unsigned/signed index. The latter is supported
by retail's signed jl back edge, not a pointer-shape analogy with SH4.
The four helper forms vary only the real column cursor's initial lifetime
(three folded leading indices or the DC :33 cursor) and advancing past a
guard count before/after testing that count. No extra operations, false
inline declarations, override pragmas or alternative entry signatures.
"""

import argparse
import itertools
import json
from pathlib import Path
import textwrap


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/creature_bank.cpp").read_text()
    start = source.index("            const std::vector<char*>& cells = sheet->getRow(row);")
    end = source.index("\n\n            ++row;", start)
    pasted = source[start:end]
    body = textwrap.dedent(pasted.split("\n", 1)[1])
    body = body.replace("level->", "traits.")
    # The actual DC reference parameter is `resource`; give its index an
    # unambiguous semantic name rather than shadowing that parameter.
    body = body.replace("int resource = 0; resource < 7; ++resource",
                        "int resourceId = 0; resourceId < 7; ++resourceId")
    body = body.replace("m_resources[resource]", "m_resources[resourceId]")
    body = body.replace("cells[", "resource[")
    leading = """traits.m_chance = atoi(resource[2]);
traits.m_guards.m_numTroops[0] = atoi(resource[3]);
traits.m_upgradeChance = atoi(resource[5]);

int column = 6;"""
    cursor = """int column = 2;
traits.m_chance = atoi(resource[column++]);
traits.m_guards.m_numTroops[0] = atoi(resource[column]);
column += 2;
traits.m_upgradeChance = atoi(resource[column++]);"""
    old_guard = """    if (traits.m_guards.m_numTroops[guard] == 0)
        traits.m_guards.m_armyTypes[guard] = CREATURE_NONE;
    column += 2;"""
    new_guard = """    column += 2;
    if (traits.m_guards.m_numTroops[guard] == 0)
        traits.m_guards.m_armyTypes[guard] = CREATURE_NONE;"""
    if body.count(leading) != 1 or body.count(old_guard) != 1:
        raise ValueError("Review the actual bank-level parser parent")
    stub = """// E:\\gamedcs\\creature_bank.cpp:32
DC_ONLY(0x70fe0, 0x14A)
void initialize_creature_bank_level(type_creature_bank_level* traits, const std::vector<char* resource)
{
    // @stub
}

"""
    if source.count(stub) != 1:
        raise ValueError("Review the malformed legacy DC helper stub")
    options = [dict(name="pasted-reader-control")]
    for front_cursor, before_test in itertools.product(range(2), range(2)):
        candidate = body.replace(leading, cursor) if front_cursor else body
        if before_test:
            candidate = candidate.replace(old_guard, new_guard)
        helper = ("// E:\\gamedcs\\creature_bank.cpp:32; original initialize_creature_bank_level.\n"
                  "// DC proves static linkage and both reference parameters. Retail expands\n"
                  "// the one source call in initializeCreatureBankTraits; keep the real body.\n"
                  "DC_ONLY(0x70fe0, 0x14A)\n"
                  "static void initializeCreatureBankLevel(type_creature_bank_level& traits,\n"
                  "                                       const std::vector<char*>& resource)\n"
                  "{\n" + textwrap.indent(candidate, "    ") + "\n}\n\n")
        options.append(dict(name=("cursor-from-two" if front_cursor else "folded-leading-columns")
                            + ("-advance-before-test" if before_test else "-advance-after-test"),
                            replace="            initializeCreatureBankLevel(*level, sheet->getRow(row));",
                            extra_edits=[
                                dict(source="src/creature_bank.cpp", find=stub, replace=""),
                                dict(source="src/creature_bank.cpp",
                                     insert_before="// E:\\gamedcs\\creature_bank.cpp:67, dc 0x7112c.", text=helper)]))
    index = "for (unsigned int slot = 0; slot < 5 && bankGuardTypes[slot] != CREATURE_NONE; ++slot)"
    if source.count(index) != 1:
        raise ValueError("Review the guard-table index parent")
    return dict(schema=1, source="src/creature_bank.cpp", units=["creature_bank"], evidence=__doc__, axes=[
        dict(name="level-reader-boundary", find=pasted, options=options),
        dict(name="guard-table-index", find=index, options=[
            dict(name="unsigned-control"), dict(name="retail-signed-index", replace=index.replace("unsigned int", "int"))]),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
