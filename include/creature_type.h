#ifndef HOMM3_CREATURE_TYPE_H
#define HOMM3_CREATURE_TYPE_H

// Creature identities recovered so far. Consumers retain integer-cast IDs
// where the original names have not yet been recovered.
// (Dreamcast CodeView types armies[] and IsMember's parameter as
// TCreatureType; retail compares slots against -1.)
enum TCreatureType {
    CREATURE_NONE = -1,
    // Original TCreatureType::Pikeman, ordinal zero; Dreamcast GetBaseCreature
    // returns this value on its out-of-range dwelling arm.
    CREATURE_PIKEMAN = 0,
    // The two griffins, byte-proven by ai_tactical's
    // get_counterstroke_value (0x439e80): it doubles the counterstrike
    // multiplier for 4 and refuses the spell outright for 5, which is
    // exactly the retaliation ladder (the Griffin retaliates twice, the
    // Royal Griffin already retaliates without limit, so buying it more
    // retaliations is worth nothing).
    CREATURE_GRIFFIN = 0x4,
    CREATURE_ROYAL_GRIFFIN = 0x5,
    // The four base elementals (NH3API enum spellings; IDs proven by
    // GetAlignments' compare chain at 0x44ac08).
    CREATURE_AIR_ELEMENTAL = 0x70,
    CREATURE_EARTH_ELEMENTAL = 0x71,
    CREATURE_FIRE_ELEMENTAL = 0x72,
    CREATURE_WATER_ELEMENTAL = 0x73,
    // Proven by GetArmyLuck's min-luck clamp at 0x44b415 (NH3API
    // spelling; the +1-luck special).
    CREATURE_HALFLING = 0x8a,
    // Proven by GetArmyMorale's min-morale clamp at 0x44b221 (the
    // always-positive-morale pair).
    CREATURE_MINOTAUR = 0x4e,
    CREATURE_MINOTAUR_KING = 0x4f,
    // Proven by GetLuck's enemy-group scans at 0x44b32b (the -1
    // enemy-luck special).
    CREATURE_DEVIL = 0x36,
    CREATURE_ARCH_DEVIL = 0x37,
    // Complete CastSpell's post-cast mana-channel walk compares every
    // opposing stack against 0x2b before returning one fifth of the mana
    // cost to the opposing hero. Dreamcast spells.cpp:1764 independently
    // names the same source comparison as Familiar.
    CREATURE_FAMILIAR = 0x2b,
    // Proven by GetMorale's two inlined IsMember scans: 0xc/0xd in the
    // OWN group add +1 morale (0x44af9c) and 0x44/0x45 in the ENEMY
    // group subtract 1 (0x44afd1) - exactly the Angel/Archangel and
    // Bone/Ghost Dragon morale rules, at the Complete ids those rules
    // belong to (NH3API spellings).
    // The two JOUSTERS, byte-proven by ai_tactical's
    // check_adjacent_hexes (0x436300): when the attacker's creatureType
    // is 10 or 11 AND the side is being played by the AI, a tie between
    // two candidate hexes is broken toward the one that costs MORE to
    // reach, and toward the cheaper one for every other creature. That
    // is the Champion's per-hex charge bonus and nothing else's - the
    // same joustBonus army.h already carries at +0x490 - and 10/11 is
    // where the Castle dwelling order puts the pair, immediately below
    // the Angel/Archangel 0xc/0xd this enum already proves.
    CREATURE_CAVALIER = 0xa,
    CREATURE_CHAMPION = 0xb,
    CREATURE_ANGEL = 0xc,
    CREATURE_ARCHANGEL = 0xd,
    CREATURE_BONE_DRAGON = 0x44,
    CREATURE_GHOST_DRAGON = 0x45,
    // Proven by modify_spell_damage's dispatch table at 0x44b58c:
    // the golem spell-damage ladder (1/2, 1/4, 15/100, 1/20) and the
    // upgraded elementals pairing with their base forms' counter-
    // spell lists.
    CREATURE_STONE_GOLEM = 0x20,
    CREATURE_IRON_GOLEM = 0x21,
    CREATURE_GOLD_GOLEM = 0x74,
    CREATURE_DIAMOND_GOLEM = 0x75,
    CREATURE_ICE_ELEMENTAL = 0x7b,
    CREATURE_MAGMA_ELEMENTAL = 0x7d,
    CREATURE_STORM_ELEMENTAL = 0x7f,
    CREATURE_ENERGY_ELEMENTAL = 0x81,
    // NH3API-lineage names for the Complete-numbering ids the AI and
    // spell-work code compare (the DC enum carries the older AB
    // numbering - Catapult=118 there vs 145 here).
    CREATURE_STONE_GARGOYLE = 0x1e,
    CREATURE_OBSIDIAN_GARGOYLE = 0x1f,
    CREATURE_TROGLODYTE = 0x46,
    CREATURE_INFERNAL_TROGLODYTE = 0x47,
    // The one creature with an innate Fire Shield: army::
    // get_fire_shield_strength (0x443130) and combatManager::
    // compute_fire_shield_damage (0x422440) both accept it in place of
    // a live fireShieldRounds counter. NH3API spelling, Complete
    // numbering (0x34 is the plain Efreeti).
    CREATURE_EFREET_SULTAN = 0x35,
    // The two remaining creature CASTERS, byte-proven by
    // army::cast_spell (0x448260): its creature switch routes 0x25 to
    // cast_caliph_spell - the Master Genie's random-beneficial roll -
    // and 0x5b to CastSpell(SPELL_BLOODLUST, ...), the Ogre Mage's one
    // spell. NH3API spellings, Complete numbering (0x24 the plain
    // Genie, 0x5a the plain Ogre).
    CREATURE_MASTER_GENIE = 0x25,
    CREATURE_OGRE_MAGE = 0x5b,
    // The on-attack debuff roster, byte-proven in one function:
    // army::check_special_attack (0x440500) switches over the
    // ATTACKER's creatureType and applies BIND for the dendroids,
    // BLIND for the unicorns, DISEASE for the zombie, CURSE for the
    // two knights and the mummy, AGE for the ghost dragon, STONE for
    // the medusas and basilisks, PARALYZE for the scorpicore, POISON
    // for the wyvern monarch and the SoD acid effect (spell id 80) for
    // the rust dragon. Every id lands where the Complete dwelling runs
    // this enum already anchors put it (POWER_LICH 65 and
    // BONE/GHOST_DRAGON 0x44/0x45 pin the Necropolis run, the
    // TROGLODYTE/MINOTAUR pairs pin Dungeon, HALFLING/ROGUE pin the
    // neutrals). NH3API spellings.
    // Three more of do_attack's own witnesses (0x441610): 0x2f takes
    // the Cerberus arm - the three-headed cleave that widens the
    // attack mask by the two ring neighbours - and 0x6e/0x6f are the
    // two ids its attack-frame chooser pins to cs_attack_r, the
    // hydras, at the Fortress top exactly where the Complete run ends.
    // NH3API spellings.
    CREATURE_CERBERUS = 0x2f,
    CREATURE_HYDRA = 0x6e,
    CREATURE_CHAOS_HYDRA = 0x6f,
    CREATURE_DENDROID_GUARD = 0x16,
    CREATURE_DENDROID_SOLDIER = 0x17,
    CREATURE_ZOMBIE = 0x3b,
    CREATURE_BLACK_KNIGHT = 0x42,
    CREATURE_DREAD_KNIGHT = 0x43,
    CREATURE_MEDUSA = 0x4c,
    CREATURE_MEDUSA_QUEEN = 0x4d,
    CREATURE_SCORPICORE = 0x51,
    CREATURE_BASILISK = 0x6a,
    CREATURE_GREATER_BASILISK = 0x6b,
    CREATURE_WYVERN_MONARCH = 0x6d,
    // Creature-bank table owners (DC guard_types/reward_types, 0x5601/0x5602).
    // Complete's 0x6702a0/0x67037c tables corroborate these stored values.
    // Original DC names: eCreatureCyclops, eCreatureImp, eCreatureNagaSentinel,
    // eCreatureDragonFly, eCreatureVampire, eCreatureWyvern.
    CREATURE_CYCLOPS = 94,
    CREATURE_IMP = 42,
    CREATURE_NAGA_SENTINEL = 38,
    CREATURE_DRAGON_FLY = 105,
    CREATURE_VAMPIRE = 62,
    CREATURE_WYVERN = 108,
    CREATURE_MUMMY = 0x8d,
    // Retail army::get_mirror_effect floors this creature's backlash chance
    // at Magic Mirror's base value. NH3API supplies the Complete-era spelling.
    CREATURE_FAERIE_DRAGON = 0x86,
    // The other two war machines, added 2026-08-08 for recruit.obj's
    // siege_artifact_to_creature (0x550360), whose jump table returns
    // 0x91..0x94 for the four siege artifacts in order. The numbering
    // is corroborated by its own neighbours - 0x93/0x94 below were
    // already byte-proven - and by the creature->artifact switch
    // inlined into recruitUnit::Update, which maps 0x91->3 and
    // 0x92->4. NH3API spellings, Complete numbering.
    // 0x8f, added 2026-08-08 for hero.obj's HeroFn_004E5DE0 (0x4e5de0)
    // and the same block inlined into hero::IsInIdentifyRange
    // (0x4e5e10): both `push 0x8f` into armyGroup::get_creature_total
    // and, when the count is nonzero, force a mastery field that is
    // otherwise below 3 up to 3. The id is INTERPOLATED between two ids
    // this enum already byte-proves, and the gap admits exactly one
    // ordering: Halfling 0x8a and Catapult 0x91 bracket six slots
    // (0x8b..0x90), which is exactly the six neutrals HoMM3 places
    // between them - Peasant, Boar, Mummy, Nomad, Rogue, Troll - so
    // Rogue lands on 0x8f. The role corroborates it from the other
    // side: Rogues are the army that reveals enemy hero detail, and the
    // only consumer of this compare is the identify-range gate. NH3API
    // spelling.
    CREATURE_ROGUE = 0x8f,
    CREATURE_CATAPULT = 0x91,
    CREATURE_BALLISTA = 0x92,
    CREATURE_FIRST_AID_TENT = 0x93,
    CREATURE_AMMO_CART = 0x94,
    CREATURE_ARROW_TOWER = 0x95,
    // The four shooters combatManager::ShotIsThroughWall lets past its
    // wall gate alongside the Arrow Tower: the compare chain at
    // 0x46753d tests 0x22, 0x23, 0x88, 0x89, 0x95 in that order and
    // answers "no wall between us" for each. That set is exactly
    // HoMM3's no-obstacle-penalty shooters - Mage/Arch Mage and
    // Enchanter/Sharpshooter - at Complete numbering, and the ids the
    // enum already byte-proves on both sides bracket them: the Tower
    // dwelling order puts the Mage pair straight after Stone/Iron
    // Golem (0x20/0x21 above), and the neutral block runs Azure
    // Dragon 0x84 / Crystal Dragon 0x85 ... Halfling 0x8a, leaving
    // 0x88/0x89 exactly where Enchanter/Sharpshooter sit in it.
    // NH3API spellings; values retail-proven.
    // Gelu's retail specialty upgrades Wood Elves (0x12) to Sharpshooters.
    CREATURE_WOOD_ELF = 0x12,
    CREATURE_MAGE = 0x22,
    CREATURE_ARCH_MAGE = 0x23,
    CREATURE_ENCHANTER = 0x88,
    CREATURE_SHARPSHOOTER = 0x89,
    // The rest of the two blocks the note above already brackets, added
    // 2026-08-20 for game::GetRandomMonster (0x4c92c0). That body strikes
    // ids out of its roll domain with PUSHED IMMEDIATES, so every value
    // here is retail-proven at the byte, and the two sets it forms are
    // each semantically closed. 0x84..0x89 is the six Armageddon's Blade
    // neutral specials - the same block this header already runs "Azure
    // Dragon 0x84 / Crystal Dragon 0x85 ... Halfling 0x8a" through, with
    // Enchanter/Sharpshooter already pinned inside it. 0x76..0x83, with
    // 0x7a/0x7c/0x7e/0x80 skipped, is every Conflux-exclusive creature:
    // the upgrade half of each elemental pair plus the two Phoenix rungs.
    // The four BASE elementals are deliberately absent from that set -
    // they are generic neutrals and GetRandomMonster leaves them in the
    // domain, which is what makes the skipped ids evidence rather than a
    // gap. Existing neighbours 0x7b/0x7d/0x7f/0x81/0x86/0x88/0x89/0x8a
    // bracket the new values with no slack. NH3API Complete spellings.
    // MAGIC_ELEMENTAL 0x79, AZURE_DRAGON 0x84 and CRYSTAL_DRAGON 0x85 are
    // NOT repeated here - get_spell_work_chance's immunity switch already
    // carries all three below, at the same values this body proves.

    // GATED, and it has to be - measured both ways 2026-08-20, exactly
    // like netmsg.h's RS_ERASE_OBJECT. Ungated, these six enumerators take
    // initialize ?initialize_game_data@@YIXXZ 100.00 -> 96.09 and events
    // ?monsters_sell_out@advManager@@... 100.00 -> 99.95. 96.09 is the
    // include-set canary's own signature value, and neither TU mentions a
    // creature id: this is the documented type/symbol-table population
    // class, not a modelling error. game.obj opens them for itself.
    CREATURE_PIXIE = 0x76,
    CREATURE_SPRITE = 0x77,
    CREATURE_PSYCHIC_ELEMENTAL = 0x78,
    CREATURE_FIREBIRD = 0x82,
    CREATURE_PHOENIX = 0x83,
    CREATURE_RUST_DRAGON = 0x87,
    // hero::GetManaCost (0x4e5240) crosses two pairs: 0x14/0x15 in the
    // ENEMY group add 2 to the cost, 0x22/0x23 in the caster's own
    // group take 2 off. The second pair is the Mage/Arch Mage above -
    // HoMM3's spell-cost reducers - so the first is the pair that
    // RAISES enemy spell cost, Pegasus/Silver Pegasus, and 0x14/0x15
    // is exactly where the Rampart dwelling order puts them (after
    // Wood Elf/Grand Elf 0x12/0x13, before Dendroid Guard 0x16).
    CREATURE_PEGASUS = 0x14,
    CREATURE_SILVER_PEGASUS = 0x15,
    // hero::GetNecromancyCreature (0x4e3c60) returns exactly these four
    // off the Necromancy mastery byte, in this order - HoMM3's
    // Necropolis undead ladder, and the Cloak of the Undead King's own
    // basic/advanced/expert upgrades. DC spellings for 58 and 59 are
    // eCreatureZombie / eCreatureZombieLord; the NH3API pair used here
    // (Walking Dead / Zombie) is the same two ids, kept because the
    // rest of this enum is NH3API-spelled.
    CREATURE_SKELETON = 0x38,
    CREATURE_WALKING_DEAD = 0x3a,
    CREATURE_WIGHT = 0x3c,
    CREATURE_LICH = 0x40,
    // get_spell_work_chance's per-creature immunity switch (0x44a9xx):
    // the magic-resistant dwarves/Crystal Dragon, the level-gated
    // dragons, and the spell-immune Magic Elemental (NH3API spellings;
    // Complete numbering).
    CREATURE_DWARF = 0x10,
    CREATURE_BATTLE_DWARF = 0x11,
    CREATURE_GREEN_DRAGON = 0x1a,
    CREATURE_GOLD_DRAGON = 0x1b,
    CREATURE_RED_DRAGON = 0x52,
    CREATURE_BLACK_DRAGON = 0x53,
    CREATURE_MAGIC_ELEMENTAL = 0x79,
    CREATURE_AZURE_DRAGON = 0x84,
    CREATURE_CRYSTAL_DRAGON = 0x85,
    // 0x8e, one slot below Rogue in the six-slot run the CREATURE_ROGUE
    // note above reconstructs (Peasant, Boar, Mummy, NOMAD, Rogue,
    // Troll). Retail pins the ROLE independently: findpath's
    // GetTerrainCost (0x4b18c0) asks get_creature_total(0x8e) > 0 and
    // hands the answer to CalcTerrainCost as the flag whose only effect
    // is `terrain == 1 -> terrain = 0` - it erases terrain 1's movement
    // penalty, terrain 1 is Sand (terrain.h's ten-mask permutation),
    // and Nomads are the creature that cancels the sand penalty.
    CREATURE_NOMAD = 0x8e
};

#endif
