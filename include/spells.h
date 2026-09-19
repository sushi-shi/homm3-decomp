// spells.h - prototypes of spells.cpp (compiland spells.obj)
#ifndef HOMM3_SPELLS_H
#define HOMM3_SPELLS_H

#include "armygrp.h"

TCreatureType getElementalType(SpellID spell);

// DrawBolt's Chain Lightning arm is the one bolt colour shaded
// PROCEDURALLY instead of from a span table: it steps red and green
// down 255 -> 240 -> 224 -> 216 -> 200 -> 192 with depth into the drawn
// span while blue stays saturated, and retail spells all six steps out
// one at a time rather than through a table (each arm re-expands
// RGBto16 in full; only the last channel term is tail-merged). Depth 0
// is the span's outer rim, and anything deeper than four is the floor.
enum EBoltSpanDepth {
    BOLT_SPAN_DEPTH_0 = 0,
    BOLT_SPAN_DEPTH_1 = 1,
    BOLT_SPAN_DEPTH_2 = 2,
    BOLT_SPAN_DEPTH_3 = 3,
    BOLT_SPAN_DEPTH_4 = 4
};

// --- globals ---
extern unsigned char g_boltGreenSpanColors[5][3];
extern unsigned char g_boltWhiteSpanColors[5][3];
extern unsigned char g_boltSpectrumColors[15][3];

// CODEVIEW(E:\gamedcs\spells.cpp:2061, dc 0x152240) void mark_area_highlights(SpellID spell, TSkillMastery mastery, long hex);
// CODEVIEW(E:\gamedcs\spells.cpp:2133, dc 0x152484) void clear_area_highlights();
// CODEVIEW(E:\gamedcs\spells.cpp:2266, dc 0x1527bc) int HandleCastWallSpell(message* msg);

// --- combatManager ---
// CODEVIEW(E:\gamedcs\spells.cpp:97, dc 0x14ea14) SpellID combatManager::ViewSpells();
// CODEVIEW(E:\gamedcs\spells.cpp:176, dc 0x14ecbc) void combatManager::InitiateSpell(SpellID spellToCast);
// CODEVIEW(E:\gamedcs\spells.cpp:616, dc 0x14f7dc) void combatManager::CastSpell(SpellID spellId, int targetIndex, unsigned char bIsMonsterSpell, int secondaryIndex, TSkillMastery monster_skill, long monster_power);
// CODEVIEW(E:\gamedcs\spells.cpp:2645, dc 0x152edc) unsigned char combatManager::ValidSpellTarget(SpellID spellId, TSkillMastery mastery, int targetIndex, int casting_side, unsigned char first_target, unsigned char creature_spell);
// CODEVIEW(E:\gamedcs\spells.cpp:3103, dc 0x153638) tagPOINT combatManager::hex_to_point(long hex);
// CODEVIEW(E:\gamedcs\spells.cpp:3121, dc 0x15368c) long combatManager::point_to_hex(tagPOINT point);
// CODEVIEW(E:\gamedcs\spells.cpp:3138, dc 0x1536d4) long combatManager::get_distance(tagPOINT start, tagPOINT stop);
// CODEVIEW(E:\gamedcs\spells.cpp:3214, dc 0x153884) void combatManager::mark_wall_area_effect(long target_hex, TSkillMastery mastery, std::vector<long,std::allocator<long>* result);
// CODEVIEW(E:\gamedcs\spells.cpp:3324, dc 0x153b60) void combatManager::AreaEffect(int targetCell, int iSpellType, TSkillMastery mastery, int power);
// CODEVIEW(E:\gamedcs\spells.cpp:3389, dc 0x153d2c) void combatManager::Armageddon(int level, int power);
// CODEVIEW(E:\gamedcs\spells.cpp:3572, dc 0x1542b4) void combatManager::ResetBoltAngle(SBolt* psBolt);
// CODEVIEW(E:\gamedcs\spells.cpp:4255, dc 0x155664) void combatManager::ChainLightning(int index, int level, int power);
// CODEVIEW(E:\gamedcs\spells.cpp:4424, dc 0x155b28) void combatManager::ShowMassSpell([]* bEffected, int spellEffect, unsigned char bShowWince);
// CODEVIEW(E:\gamedcs\spells.cpp:4705, dc 0x15627c) void combatManager::SummonElemental(SpellID spell, TCreatureType iMonType, int iSpellPower, int level);
// CODEVIEW(E:\gamedcs\spells.cpp:4765, dc 0x1564c4) void combatManager::DoLuck(int iTargetGroup, int iTargetIndex);
// CODEVIEW(E:\gamedcs\spells.cpp:4838, dc 0x15668c) void combatManager::remove_corpse(army* corpse);
// CODEVIEW(E:\gamedcs\spells.cpp:4888, dc 0x156840) void combatManager::Resurrect(army* target_army, long hit_points_resurrected, unsigned char temporary);
// CODEVIEW(E:\gamedcs\spells.cpp:4984, dc 0x156a68) void combatManager::Resurrect(SpellID spell, int target_hex, int power, TSkillMastery mastery, const hero* casting_hero);
// CODEVIEW(E:\gamedcs\spells.cpp:5041, dc 0x156aec) void combatManager::ShowSpellCastFailure(army* targetArmy, int spellId);
// Complete expands this helper into CastSpell's failure path while Dreamcast
// also emits the standalone inline body.
// CODEVIEW(E:\gamedcs\spells.cpp:5086, dc 0x156c30) int combatManager::ModifySpellDamage(int base_damage, int iSpellType, const hero* castingHero, const hero* affectedHero, const army* targetArmy, unsigned char print_result);
// CODEVIEW(E:\gamedcs\spells.cpp:5164, dc 0x156ec4) void combatManager::Earthquake(int level);
// CODEVIEW(E:\gamedcs\spells.cpp:5749, dc 0x157ae4) void combatManager::ShowSpellMessage(int bIsMonsterSpell, int spellId, army* targetArmy);
// CODEVIEW(E:\gamedcs\spells.cpp:5912, dc 0x1580c4) unsigned char combatManager::AbleToSummonElemental(SpellID spell, long side);
// CODEVIEW(E:\gamedcs\spells.cpp:5928, dc 0x158108) int combatManager::GetSpellWallHex(int base_index, int row_offset, int side);

#endif  /* HOMM3_SPELLS_H */
