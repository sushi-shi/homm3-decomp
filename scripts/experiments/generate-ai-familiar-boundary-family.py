"""Recover castSpell's ordinary hasCreature call alongside mass damage.

DC 0x2b094 line 1047 calls has_creature(CREATURE_FAMILIAR). The canonical
helper is the const method at 0x2ab3c, ai_combat.cpp:694, before the mass-
damage valuation helper at :711. It scans vector::size backwards and tests
type before positive count. Retail 0x425bd0's familiar scan corroborates
the same member/stride, predicates and one mana update after success.

Keep all 48 reproduced mass-damage controls and cross only this recovered
source boundary. The source must not paste the scan merely to enlarge its
caller or invent another inline qualifier to influence nested damage calls.
"""

import argparse
import importlib.util
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest(parent):
    checkpoint = json.loads((parent / "checkpoint.json").read_text())
    if len(checkpoint["records"]) != 48 or len(checkpoint["seen"]) != 48:
        raise ValueError("The 48-state mass-damage parent must be exhausted")
    for relative in ("src/ai_combat.cpp", "src/ai_player.cpp", "include/ai_combat.h"):
        if (ROOT / relative).read_bytes() != (parent / "snapshot" / relative).read_bytes():
            raise ValueError("Current source differs from verified parent: " + relative)
    spec = importlib.util.spec_from_file_location("mass_family", Path(__file__).with_name(
        "generate-ai-mass-damage-boundary-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    manifest = module.make_manifest()
    if manifest != json.loads((parent / "input.json").read_text()):
        raise ValueError("The mass-damage manifest no longer matches its parent")
    for row in checkpoint["elites"]:
        repeat = json.loads((parent / "candidates" / row["id"] / "repeat/result.json").read_text())
        if repeat["scores"] != row["scores"] or repeat["object_hash"] != row["object_hash"]:
            raise ValueError("Parent elite did not reproduce: " + row["id"])

    source = (ROOT / "src/ai_combat.cpp").read_text()
    header = (ROOT / "include/ai_combat.h").read_text()
    old_scan = """    if (defender.m_myHero) {
        for (long i = defender.m_creatures.size(); i-- > 0; ) {
            if (defender.m_creatures[i].m_type == CREATURE_FAMILIAR
                && defender.m_creatures[i].m_number > 0) {
                defender.m_mana += bestManaCost / 5;
                break;
            }
        }
    }"""
    new_scan = """    if (defender.m_myHero && defender.hasCreature(CREATURE_FAMILIAR))
        defender.m_mana += bestManaCost / 5;"""
    stub = r"""#if 0  // @carcass

// E:\gamedcs\ai_combat.cpp:694
DC_ONLY(0x2ab3c, 0x4C)
unsigned char type_AI_combat_data::has_creature(TCreatureType creature)
{
    // @stub
}

#endif  // @carcass

"""
    position = "// E:\\gamedcs\\ai_combat.cpp:711\n"
    declaration_position = "    // Before normalization (function): type_AI_combat_data::get_mass_damage_value.\n"
    if (source.count(old_scan) != 1 or source.count(stub) != 1 or source.count(position) != 1
            or header.count(declaration_position) != 2):
        raise ValueError("Review the familiar scan and canonical helper's source position")
    # Include the following first-overload declaration to make the anchor unique.
    declaration_position += "    long getMassDamageValue(type_spell_choice& choice,\n"
    if header.count(declaration_position) != 1:
        raise ValueError("The first mass-damage declaration is no longer unique")
    helper = """// E:\\gamedcs\\ai_combat.cpp:694; original has_creature, dc 0x2ab3c.
unsigned char type_AI_combat_data::hasCreature(TCreatureType creature) const
{
    for (long i = m_creatures.size(); i-- > 0; ) {
        if (m_creatures[i].m_type == creature && m_creatures[i].m_number > 0)
            return 1;
    }
    return 0;
}

"""
    manifest["axes"].append(dict(name="familiar-source-boundary", find=old_scan, options=[
        dict(name="pasted-scan-control"),
        dict(name="canonical-ordinary-has-creature", replace=new_scan, extra_edits=[
            dict(source="src/ai_combat.cpp", find=stub, replace=""),
            dict(source="src/ai_combat.cpp", insert_before=position, text=helper),
            dict(source="include/ai_combat.h", insert_before=declaration_position,
                 text="    // DC original: has_creature.\n    unsigned char hasCreature(TCreatureType creature) const;\n"),
        ]),
    ]))
    manifest["evidence"] = __doc__
    manifest["parent_context"] = parent.name
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(args.parent), indent=2) + "\n")
