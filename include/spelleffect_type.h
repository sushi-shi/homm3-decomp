#ifndef HOMM3_SPELLEFFECT_TYPE_H
#define HOMM3_SPELLEFFECT_TYPE_H

// Original global TSpellEffectID, Dreamcast NB11 enum record 0x1f15.
// TSpellTraits.m_effect (+0x08) references this record directly.
// Complete retains the Dreamcast ordinals 0..80 and int-wide storage, then
// inserts one row: g_spellEffectTraits (.rdata 0x641e08) has 83 rows, row 81
// is "c0acid.def"/"AcidBreath" and row 82 is "poof.def"/"Poof", and both
// combatManager::spellEffect overloads (0x496840 and its hex twin) reject
// `effect >= 83`. kNumSpellEffects (DC 82) therefore counts 83 here.
enum TSpellEffectID {
    eSpellEffectNone = -1,
    eSpellEffectPrayer = 0,
    // Original eSpellEffectLightning_Bolt.
    eSpellEffectLightningBolt = 1,
    eSpellEffectAirShield = 2,
    eSpellEffectBacklash = 3,
    eSpellEffectAnimateDead = 4,
    eSpellEffectAntiMagic = 5,
    eSpellEffectBlind = 6,
    eSpellEffectCounterstroke = 7,
    eSpellEffectDeathRipple = 8,
    eSpellEffectFireblast = 9,
    eSpellEffectDecay = 10,
    eSpellEffectFireShield = 11,
    eSpellEffectFirestorm = 12,
    // Original eSpellEffectDisruptiveRay_Ray.
    eSpellEffectDisruptiveRayRay = 13,
    // Original eSpellEffectDisruptiveRay_Burst.
    eSpellEffectDisruptiveRayBurst = 14,
    eSpellEffectFear = 15,
    eSpellEffectMeteorShower = 16,
    eSpellEffectFrenzy = 17,
    eSpellEffectFortune = 18,
    eSpellEffectMuckAndMire = 19,
    eSpellEffectMirth = 20,
    eSpellEffectHypnotize = 21,
    eSpellEffectProtectionFromAir = 22,
    eSpellEffectProtectionFromWater = 23,
    eSpellEffectProtectionFromFire = 24,
    eSpellEffectPrecision = 25,
    eSpellEffectProtectionFromEarth = 26,
    eSpellEffectShield = 27,
    eSpellEffectSlayer = 28,
    eSpellEffectSacredBreath = 29,
    eSpellEffectSorrow = 30,
    eSpellEffectTailWind = 31,
    // Original eSpellEffectForcefield_2.
    eSpellEffectForcefield2 = 32,
    // Original eSpellEffectForcefield_3.
    eSpellEffectForcefield3 = 33,
    eSpellEffectRemoveObstacle = 34,
    eSpellEffectBerserk = 35,
    eSpellEffectBless = 36,
    // Original eSpellEffectChainLightning_Bolt.
    eSpellEffectChainLightningBolt = 37,
    // Original eSpellEffectChainLightning_Dust.
    eSpellEffectChainLightningDust = 38,
    eSpellEffectCure = 39,
    eSpellEffectCurse = 40,
    eSpellEffectDispel = 41,
    eSpellEffectForgetfulness = 42,
    // Original eSpellEffectFirewall_2.
    eSpellEffectFirewall2 = 43,
    // Original eSpellEffectFirewall_3.
    eSpellEffectFirewall3 = 44,
    eSpellEffectFrostRing = 45,
    // Original eSpellEffectIceRay_Burst.
    eSpellEffectIceRayBurst = 46,
    eSpellEffectLandMine = 47,
    eSpellEffectMisfortune = 48,
    // Original eSpellEffectLightning_Dust.
    eSpellEffectLightningDust = 49,
    eSpellEffectResurrection = 50,
    eSpellEffectSacrifice_Slay = 51,
    // Original eSpellEffectSacrifice_Resurrect.
    eSpellEffectSacrificeResurrect = 52,
    eSpellEffectSpontaneousCombustion = 53,
    eSpellEffectToughSkin = 54,
    eSpellEffectQuicksand = 55,
    eSpellEffectWeakness = 56,
    eSpellEffectLandMineExplosion = 57,
    eSpellEffectDispelQuicksand = 58,
    eSpellEffectDispelLandMine = 59,
    // Original eSpellEffectDispelForcefield_2.
    eSpellEffectDispelForcefield2 = 60,
    // Original eSpellEffectDispelForcefield_3.
    eSpellEffectDispelForcefield3 = 61,
    // Original eSpellEffectDispelFirewall_2.
    eSpellEffectDispelFirewall2 = 62,
    // Original eSpellEffectDispelFirewall_3.
    eSpellEffectDispelFirewall3 = 63,
    // Original eSpellEffectMagicBolt_Burst.
    eSpellEffectMagicBoltBurst = 64,
    // Original eSpellEffectFirewall_1.
    eSpellEffectFirewall1 = 65,
    // Original eSpellEffectDispelFirewall_1.
    eSpellEffectDispelFirewall1 = 66,
    eSpellEffectPoison = 67,
    eSpellEffectBind = 68,
    eSpellEffectDisease = 69,
    eSpellEffectParalyze = 70,
    eSpellEffectAge = 71,
    eSpellEffectDeathCloud = 72,
    eSpellEffectDeathBlow = 73,
    eSpellEffectDrainLife = 74,
    eSpellEffectMagicChannel_Suck = 75,
    eSpellEffectMagicChannel_Spew = 76,
    eSpellEffectMagicDrain = 77,
    eSpellEffectMagicResistance = 78,
    // Original eSpellEffectRegenerate.
    eSpellEffectRegeneration = 79,
    eSpellEffectDeathStare = 80,
    // Complete row 81 is "AcidBreath" and Poof moves to row 82 (see above).
    // Renaming or adding an enumerator here reorders VC6's .bss layout and
    // reschedules advManager::doCombat, so the Dreamcast spellings remain.
    eSpellEffectPoof = 81,
    kNumSpellEffects = 83,
};

#endif
