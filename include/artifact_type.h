// Canonical artifact-id domain, shared by artifact and army consumers.
#ifndef HOMM3_ARTIFACT_TYPE_H
#define HOMM3_ARTIFACT_TYPE_H

// Dreamcast's TArtifact enum names eArtifactSpellbook=0 and
// eArtifactSpellScroll=1. The old separate EArtifactId made a spellbook
// select type_artifact(SpellID), since SpellID is currently an int alias.
// DC combatwindow.cpp:82 explicitly calls type_artifact(TArtifact);
// retail 0x472010+0xd3 stores {0,-1}, not the scroll payload {1,0}.
// Keep the existing byte-proven ordinals in this single enum so overload
// resolution preserves the artifact domain in every caller. This header
// contains only the domain, without artifact traits or STL dependencies.
// Before normalization (type): TArtifact.
enum Artifact {
    ARTIFACT_NONE = -1,
    // Retail witness: hero::TransferArtifacts (0x4e23d0) refuses to move
    // an artifact whose id is -1, 2, 0 or one of the four war machines
    // (`cmp eax,2`, `test eax,eax`, then 3/4/5/6) - the "not a real,
    // transferable artifact" set. DC spelling eArtifactHolyGrail.
    ARTIFACT_HOLY_GRAIL = 2,
    // Retail witness: hero::remove_artifact(TArtifact) 0x4e2dd0 opens
    // `cmp ebx,1 / je <return 0>` - a scroll cannot be removed by id,
    // because what distinguishes two scrolls is the `spell` dword, not
    // the artifact id. DC spelling eArtifactSpellScroll.
    ARTIFACT_SPELL_SCROLL = 1,
    ARTIFACT_CATAPULT = 3,
    ARTIFACT_BALLISTA = 4,
    ARTIFACT_AMMO_CART = 5,
    ARTIFACT_FIRST_AID_TENT = 6,
    // value_of_town's Dreamcast-named legion_artifacts table and the
    // retail .rdata row at 0x640558 independently fix this five-piece run.
    ARTIFACT_LEGS_OF_LEGION = 118,
    ARTIFACT_LOINS_OF_LEGION = 119,
    ARTIFACT_TORSO_OF_LEGION = 120,
    ARTIFACT_ARMS_OF_LEGION = 121,
    ARTIFACT_HEAD_OF_LEGION = 122,
    // The two campaign combination-quest triples CheckForArtifactWin
    // (0x5f1610) collects into its piece vector: the Armor of the
    // Damned components on campaign 7 map 1, and the two Angelic
    // Alliance triples on campaign 18 maps 8/9. Ids are the canonical
    // artifact ordinals; gated with the rest of the VLC view.
    ARTIFACT_SWORD_OF_HELLFIRE = 0xb,
    ARTIFACT_SHIELD_OF_THE_DAMNED = 0x11,
    ARTIFACT_BREASTPLATE_OF_BRIMSTONE = 0x1d,
    ARTIFACT_ARMOR_OF_WONDER = 0x1f,
    ARTIFACT_SANDALS_OF_THE_SAINT = 0x20,
    ARTIFACT_CELESTIAL_NECKLACE_OF_BLISS = 0x21,
    ARTIFACT_LIONS_SHIELD_OF_COURAGE = 0x22,
    ARTIFACT_SWORD_OF_JUDGEMENT = 0x23,
    ARTIFACT_HELM_OF_HEAVENLY_ENLIGHTENMENT = 0x24,
    // CheckForDefeatedHeroLoss's quest-artifact compares (0x5f2a40):
    // the two Shadow of Death necromancy accessories the Gem campaign
    // escorts. Canonical ordinals, same gate. (The fifth orb the same
    // arm tests is armygrp.h's ungated ARTIFACT_ORB_OF_INHIBITION.)
    ARTIFACT_VAMPIRES_COWL = 0x37,
    ARTIFACT_DEAD_MANS_BOOTS = 0x38,
    // hero::GetVisibility (0x4e4070) adds one tile of scouting radius
    // for each of 0x34/0x35; DC 52/53 are eArtifactSpeculum and
    // eArtifactSpyglass, HoMM3's two scouting artifacts.
    // Retail witness: hero::equip_artifact (0x4e2a00) equips a SPELLBOOK
    // into slot 17 first when this artifact lands on a hero who has
    // none - which is Titan's Thunder's published behaviour, and 135 is
    // its id in Shadow of Death's combination block (129 Angelic
    // Alliance .. 140 Cornucopia). Name from the behaviour; the id and
    // the spellbook rule are byte-proven.
    ARTIFACT_TITANS_THUNDER = 0x87,
    ARTIFACT_SPECULUM = 0x34,
    ARTIFACT_SPYGLASS = 0x35,
    // hero::GetMagicResistanceFactor (0x4e46e0) adds +0.05/+0.10/+0.15
    // for 0x39/0x3a/0x3b; DC 57/58/59 are
    // eArtifactGarnitureOfInterference / eArtifactSurcoatOfCounterpoise
    // / eArtifactBootsOfPolarity - the magic-resistance set.
    ARTIFACT_GARNITURE_OF_INTERFERENCE = 0x39,
    ARTIFACT_SURCOAT_OF_COUNTERPOISE = 0x3a,
    ARTIFACT_BOOTS_OF_POLARITY = 0x3b,
    // hero::GetArcheryFactor (0x4e4160) adds +0.05/+0.10/+0.15 for
    // 0x3c/0x3d/0x3e; DC 60/61/62 are eArtifactBowOfElvenCherrywood /
    // eArtifactBowstringOfTheUnicornsMane / eArtifactAngelFeatherArrows
    // - and IsWieldingArtifact's combination recursion pairs exactly
    // those three with armygrp.h's byte-proven
    // ARTIFACT_BOW_OF_THE_SHARPSHOOTER (0x89), the artifact they
    // assemble into.
    ARTIFACT_BOW_OF_ELVEN_CHERRYWOOD = 0x3c,
    ARTIFACT_BOWSTRING_OF_THE_UNICORNS_MANE = 0x3d,
    ARTIFACT_ANGEL_FEATHER_ARROWS = 0x3e,
    // hero::GetEagleEyeChance (0x4e4420) adds +0.05/+0.10/+0.15 for
    // 0x3f/0x40/0x41; DC 63/64/65 are eArtifactBirdOfPerception /
    // eArtifactStoicWatchman / eArtifactEmblemOfCognizance.
    ARTIFACT_BIRD_OF_PERCEPTION = 0x3f,
    ARTIFACT_STOIC_WATCHMAN = 0x40,
    ARTIFACT_EMBLEM_OF_COGNIZANCE = 0x41,
    // hero::GetSurrenderCostFactor (0x4e4580) adds +0.10 for each of
    // 0x42/0x43/0x44; DC 66/67/68 are eArtifactStatesmansMedal /
    // eArtifactDiplomatsRing / eArtifactAmbassadorsSash.
    ARTIFACT_STATESMANS_MEDAL = 0x42,
    ARTIFACT_DIPLOMATS_RING = 0x43,
    ARTIFACT_AMBASSADORS_SASH = 0x44,
    // hero::get_combat_speed_bonus (0x4e5aa0) adds +1/+1/+2 creature
    // speed for 0x45/0x61/0x63; DC 69/97/99 are
    // eArtifactRingOfTheWayfarer / eArtifactNecklaceOfSwiftness /
    // eArtifactCapeOfVelocity, whose HoMM3 effects are exactly that.
    ARTIFACT_RING_OF_THE_WAYFARER = 0x45,
    // hero::GetMysticismBonus (0x4e3f40) adds +1/+2/+3 mana per day for
    // 0x49/0x4a/0x4b; DC 73/74/75 are eArtifactCharmOfMana /
    // eArtifactTalismanOfMana / eArtifactMysticOrbOfMana, whose HoMM3
    // effects are +1/+2/+3 mana per day - the bonus ladder IS the
    // identification.
    ARTIFACT_CHARM_OF_MANA = 0x49,
    ARTIFACT_TALISMAN_OF_MANA = 0x4a,
    ARTIFACT_MYSTIC_ORB_OF_MANA = 0x4b,
    // hero::GetSpellDurationBonus (0x4e4db0) adds +1/+2/+3 rounds for
    // 0x4c/0x4d/0x4e; DC 76/77/78 are eArtifactCollarOfConjuring /
    // eArtifactRingOfConjuring / eArtifactCapeOfConjuring, +1/+2/+3
    // spell rounds in HoMM3.
    ARTIFACT_COLLAR_OF_CONJURING = 0x4c,
    ARTIFACT_RING_OF_CONJURING = 0x4d,
    ARTIFACT_CAPE_OF_CONJURING = 0x4e,
    // hero::modify_spell_damage (0x4e5760) pairs each of these with one
    // school bit of the spell's traits row - 0x4f with air, 0x50 with
    // earth, 0x51 with fire, 0x52 with water - and multiplies the
    // damage by 1.5 when the matching orb is worn. DC 79..82 are
    // eArtifactOrbOfTheFirmament / eArtifactOrbOfSilt /
    // eArtifactOrbOfTempestuousFire / eArtifactOrbOfDrivingRain, whose
    // HoMM3 schools are air / earth / fire / water in exactly that
    // order.
    ARTIFACT_ORB_OF_THE_FIRMAMENT = 0x4f,
    ARTIFACT_ORB_OF_SILT = 0x50,
    ARTIFACT_ORB_OF_TEMPESTUOUS_FIRE = 0x51,
    ARTIFACT_ORB_OF_DRIVING_RAIN = 0x52,
    // The four Tomes, byte-proven by mark_spells (0x4d9350): each of
    // these ids selects an arm that sweeps the whole 70-entry
    // akSpellTraits table and grants every spell whose SCHOOL MASK at
    // +0x1c carries one specific bit - 0x57 tests bit 1 (eSchoolAir),
    // 0x56 bit 2 (eSchoolFire), 0x58 bit 4 (eSchoolWater), 0x59 bit 8
    // (eSchoolEarth). "Grants every spell of exactly one school" is the
    // Tome of <school> Magic and nothing else, and the bit -> school
    // pairing is itself already byte-proven by hero::GetHighestSchool
    // (see spellschool.h). That fixes all four against HoMM3's own
    // 86/87/88/89 fire/air/water/earth ordering.
    ARTIFACT_TOME_OF_FIRE_MAGIC = 0x56,
    ARTIFACT_TOME_OF_AIR_MAGIC = 0x57,
    ARTIFACT_TOME_OF_WATER_MAGIC = 0x58,
    ARTIFACT_TOME_OF_EARTH_MAGIC = 0x59,
    // Two more mark_spells arms, identified by what they grant rather
    // than by any roster: 0x7b grants exactly spells 0 and 1 - Summon
    // Boat and Scuttle Boat, the Sea Captain's Hat's whole effect - and
    // 0x7c grants every spell whose traits level is 5, which is the
    // Spellbinder's Hat and only it. 123/124 is where HoMM3's numbering
    // puts that pair, just below Armageddon's Blade at 128, which this
    // enum already byte-proves.
    ARTIFACT_SEA_CAPTAINS_HAT = 0x7b,
    ARTIFACT_SPELLBINDERS_HAT = 0x7c,
    // hero::get_hit_point_bonus (0x4e5b80) gates +1 / +1 / +2 hit
    // points on 0x5e / 0x5f / 0x60 and a further quarter of the
    // creature's own hit points on 0x83. DC 94/95/96 are
    // eArtifactRingOfVitality / eArtifactRingOfLife /
    // eArtifactVialOfLifeblood, whose HoMM3 effects are exactly
    // +1/+1/+2 health, and 0x83 (131) is their Shadow of Death
    // combination, the Elixir of Life - past the DC roster, same
    // Complete band as the other combos above.
    ARTIFACT_RING_OF_VITALITY = 0x5e,
    ARTIFACT_RING_OF_LIFE = 0x5f,
    ARTIFACT_VIAL_OF_LIFEBLOOD = 0x60,
    ARTIFACT_ELIXIR_OF_LIFE = 0x83,
    ARTIFACT_COUNT = 144,
    ARTIFACT_NECKLACE_OF_SWIFTNESS = 0x61,
    ARTIFACT_CAPE_OF_VELOCITY = 0x63,
    // The ONE artifact hero::get_spell_level (0x4e5080) and
    // hero::GetManaCost (0x4e5240) special-case, and they special-case
    // it against exactly one spell: id 0x1a, SPELL_ARMAGEDDON. Wearing
    // it makes Armageddon EXPERT regardless of the hero's schools -
    // the Armageddon's Blade rule. 128 is past the DC's AB-era roster
    // (the blade is a Shadow of Death artifact), but the pairing with
    // SPELL_ARMAGEDDON is the identification, and 128 sits in the same
    // Complete-era band the combat artifact roster already byte-proves
    // twice (0x86 / 0x89).
    ARTIFACT_ARMAGEDDONS_BLADE = 0x80,
    // The two combination artifacts that auto-cast at the top of their
    // owner's combat turn, and combatManager::SetNextArmy (0x465330) is
    // the one body that proves both. It asks hero::IsWieldingArtifact
    // for 0x81 and casts exactly SPELL_PRAYER; it asks for 0x84 and
    // casts exactly SPELL_SLOW, SPELL_CURSE, SPELL_WEAKNESS and
    // SPELL_MISFORTUNE, in that order. Those two spell sets ARE the
    // identification - the Angelic Alliance's Prayer and the Armor of
    // the Damned's Slow/Curse/Weakness/Misfortune - and both ids fall
    // exactly where the Complete-era combination run this header
    // already anchors at both ends puts them: 0x80 Armageddon's Blade
    // above, 0x82 Cloak of the Undead King and 0x83 Elixir of Life
    // below, and 0x86 Power of the Dragon Father / 0x89 Bow of the
    // Sharpshooter / 0x8b Ring of the Magi further on.
    // Corroborated a second time by combatManager::ShowSpellMessage
    // (0x5a8950), which names the CASTER of an artifact-cast spell out
    // of akArtifactTraits at exactly these two rows - 0x81 for Prayer,
    // 0x84 for the Slow/Curse/Weakness/Misfortune group - and reaches
    // them from the spell ids alone, the same pairing read backwards.
    ARTIFACT_ANGELIC_ALLIANCE = 0x81,
    ARTIFACT_ARMOR_OF_THE_DAMNED = 0x84,
    // hero::GetNecromancyCreature (0x4e3c60) gates its whole
    // Walking Dead / Wight / Lich ladder on artifact 0x82 - which is
    // precisely what the Cloak of the Undead King does in HoMM3, and
    // the ladder IS the identification. Also a Shadow of Death combo,
    // hence past the DC roster.
    ARTIFACT_CLOAK_OF_THE_UNDEAD_KING = 0x82,
    // The fourth gate of GetSpellDurationBonus, worth +50 rounds. 139
    // is past the DC's AB-era roster, but the +50-round effect is
    // unique to the Ring of the Magi, and the Complete numbering that
    // places it at 139 is the one the combat artifact roster already
    // byte-proves twice - Power of the Dragon Father 0x86 (134) and
    // Bow of the Sharpshooter 0x89 (137).
    ARTIFACT_RING_OF_THE_MAGI = 0x8b,
    // Retail GetArmyMorale/GetLuck prove the morale/luck constants; spell
    // immunity gates prove the pendant, sphere and orb constants.
    // combatManager::can_cast_spells (0x41f890) gates a hero cast on
    // slot 0 and then refuses every cast when either combat hero
    // wields 0x7e - the pair the decode note above already calls out
    // (spellbook first, inhibition 0x7e).
    ARTIFACT_SPELLBOOK = 0x0,
    ARTIFACT_BADGE_OF_COURAGE = 0x31,
    ARTIFACT_SPIRIT_OF_OPPRESSION = 0x54,
    ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR = 0x55,
    // The two obstacle-penalty bows: combatManager::ShotIsThroughWall
    // (0x467510) waives the town-wall penalty for a shooter whose own
    // hero wields either of them, pushing 0x5b then 0x89 into
    // hero::IsWieldingArtifact. Exactly two artifacts in HoMM3 cancel
    // the obstacle/wall penalty - the Golden Bow and its combination
    // successor, the Bow of the Sharpshooter - so the semantics pin
    // the pairing. NH3API spellings; both values retail-proven by the
    // pushed immediates. (0x89 is also CREATURE_SHARPSHOOTER's id in
    // the unrelated creature domain - a coincidence of numbering, not
    // a shared enum.)
    ARTIFACT_GOLDEN_BOW = 0x5b,
    ARTIFACT_SPHERE_OF_PERMANENCE = 0x5c,
    ARTIFACT_ORB_OF_VULNERABILITY = 0x5d,
    ARTIFACT_PENDANT_OF_DISPASSION = 0x64,
    ARTIFACT_PENDANT_OF_SECOND_SIGHT = 0x65,
    ARTIFACT_PENDANT_OF_HOLINESS = 0x66,
    ARTIFACT_PENDANT_OF_LIFE = 0x67,
    ARTIFACT_PENDANT_OF_DEATH = 0x68,
    ARTIFACT_PENDANT_OF_FREE_WILL = 0x69,
    ARTIFACT_PENDANT_OF_NEGATIVITY = 0x6a,
    ARTIFACT_PENDANT_OF_TOTAL_RECALL = 0x6b,
    ARTIFACT_ORB_OF_INHIBITION = 0x7e,
    ARTIFACT_POWER_OF_THE_DRAGON_FATHER = 0x86,
    ARTIFACT_BOW_OF_THE_SHARPSHOOTER = 0x89
};

#endif // HOMM3_ARTIFACT_TYPE_H
