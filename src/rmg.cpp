// rmg.cpp - Complete-only random-map generator support.
//
// The Dreamcast build has no RMG compiland. Retail's direct caller graph
// reaches this library from TSingleSelectionWindow::GenerateRandomMap, and
// the tree node layout proves an eight-byte TPoint value ordered by y, then x.
#include <va.h>
#define _MT
#include <yvals.h>
#undef _MT
#include <set>
#include "armygrp.h"
#include "rmg.h"

typedef std::set<TPoint> TRmgPointSet;

// Complete-only helper called by InitializeObjectGenerators at 0x538b10.
// The four argument loads, five stores, vtable relocation, and `ret 0x10`
// independently prove this constructor and the shared 0x14-byte prefix.
VA(0x00534160, 0x27)
type_treasure_def::type_treasure_def(
    int newObjectType, int newSubtype, int newValue, int newDensity)
{
    objectType = newObjectType;
    subtype = newSubtype;
    value = newValue;
    density = newDensity;
}

// The compiler expands the common four-store constructor in each of these
// derived definitions; retail retains only the derived vptr store.  This is
// ordinary /Ob2 expansion of a real helper boundary, not a hand-flattened
// substitute for that boundary.
VA(0x00534250, 0xB5)
type_black_box_creature_def::type_black_box_creature_def(int newCreatureType)
    : type_treasure_def(6, 0, -1, 3),
      creatureType(newCreatureType)
{
    adjustedValue =
        gRmgCreatureValueByLevel[akCreatureTypeTraits[newCreatureType].level]
        / akCreatureTypeTraits[newCreatureType].AI_value;

    if (adjustedValue > 50)
        adjustedValue = ((adjustedValue + 5) / 10) * 10;
    else if (adjustedValue > 12)
        adjustedValue = ((adjustedValue + 2) / 5) * 5;
    else if (adjustedValue > 5)
        adjustedValue = ((adjustedValue + 1) / 2) * 2;
}

VA(0x005349D0, 0x29)
type_shrine_def::type_shrine_def(int newObjectType, int newValue)
    : type_treasure_def(newObjectType, 0, newValue, 100)
{
}

VA(0x00534A60, 0x25)
type_witch_hut_def::type_witch_hut_def()
    : type_treasure_def(0x71, 0, 1500, 80)
{
}

VA(0x00534EA0, 0x30)
type_spell_scroll_def::type_spell_scroll_def(int newSpellLevel, int newValue)
    : type_treasure_def(0x5d, 0, newValue, 30)
{
    spellLevel = newSpellLevel;
}

// Complete-only object-generator roster.  Retail proves the source-level
// `push_back(new ...)` chain through all three stages of VC6's real inline
// ladder: early sites retain vector::insert(pos, value), middle sites retain
// vector::insert(pos, 1, value), and late sites retain vector::push_back.
// Those are compiler expansion choices for one honest source operation, not
// three manually selected overloads.  The four loops below are likewise the
// only repeated structures present in retail; every other registration is an
// unrolled source statement.
VA(0x00538B10, 0x2241)
void type_random_map_generator::InitializeObjectGenerators()
{
    objectGenerators.push_back(new type_treasure_def(2, 0, 100, 20));
    objectGenerators.push_back(new type_treasure_def(4, 0, 3000, 50));

    {
        int creatureCount = mapVersion >= 1 ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (akCreatureTypeTraits[creature].level >= 0)
                objectGenerators.push_back(
                    new type_black_box_creature_def(creature));
        }
    }

    objectGenerators.push_back(
        new type_black_box_experience_def(6000, 5000));
    objectGenerators.push_back(
        new type_black_box_experience_def(12000, 10000));
    objectGenerators.push_back(
        new type_black_box_experience_def(18000, 15000));
    objectGenerators.push_back(
        new type_black_box_experience_def(24000, 20000));

    objectGenerators.push_back(new type_black_box_gold_def(5000, 5000));
    objectGenerators.push_back(new type_black_box_gold_def(10000, 10000));
    objectGenerators.push_back(new type_black_box_gold_def(15000, 15000));
    objectGenerators.push_back(new type_black_box_gold_def(20000, 20000));

    objectGenerators.push_back(new type_black_box_spells_def(5000, 1, 1, 15));
    objectGenerators.push_back(new type_black_box_spells_def(7500, 2, 2, 15));
    objectGenerators.push_back(new type_black_box_spells_def(10000, 3, 3, 15));
    objectGenerators.push_back(new type_black_box_spells_def(12500, 4, 4, 15));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 5, 5, 15));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 1));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 2));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 4));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 8));
    objectGenerators.push_back(new type_black_box_spells_def(30000, 1, 5, 15));

    {
        int player = playerSlots.size();
        disabledKeyTents.resize(player);
        for (; player--;) {
            disabledKeyTents[player] = 0;
            objectGenerators.push_back(new type_key_tent_def(player, 5000));
            objectGenerators.push_back(new type_key_tent_def(player, 7500));
            objectGenerators.push_back(new type_key_tent_def(player, 10000));
            objectGenerators.push_back(new type_key_tent_def(player, 15000));
            objectGenerators.push_back(new type_key_tent_def(player, 20000));
        }
    }

    objectGenerators.push_back(new type_treasure_def(7, 0, 8000, 20));
    objectGenerators.push_back(new type_treasure_def(11, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(12, 0, 2000, 500));
    objectGenerators.push_back(new type_treasure_def(13, 0, 5000, 20));
    objectGenerators.push_back(new type_treasure_def(13, 1, 10000, 20));
    objectGenerators.push_back(new type_treasure_def(13, 2, 7500, 20));
    objectGenerators.push_back(new type_treasure_def(14, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(16, 0, 3000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 1, 2000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 2, 2000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 3, 5000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 4, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(16, 5, 3000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 6, 9000, 100));

    int dwelling = 80;
    if (mapVersion < 1)
        dwelling = 58;
    for (; dwelling--;)
        objectGenerators.push_back(new type_map_dwelling_def(dwelling));

    objectGenerators.push_back(new type_treasure_def(22, 0, 500, 100));
    objectGenerators.push_back(new type_treasure_def(23, 0, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(24, 0, 4000, 20));
    objectGenerators.push_back(new type_treasure_def(25, 0, 10000, 100));
    objectGenerators.push_back(new type_treasure_def(28, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(29, 0, 500, 1000));
    objectGenerators.push_back(new type_treasure_def(30, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(31, 0, 100, 50));
    objectGenerators.push_back(new type_treasure_def(32, 0, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(35, 0, 7000, 20));
    objectGenerators.push_back(new type_treasure_def(38, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(39, 0, 500, 100));
    objectGenerators.push_back(new type_treasure_def(41, 0, 12000, 20));
    objectGenerators.push_back(new type_treasure_def(47, 0, 1000, 50));
    objectGenerators.push_back(new type_treasure_def(48, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(49, 0, 250, 100));
    objectGenerators.push_back(new type_treasure_def(51, 0, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(52, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(55, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(56, 0, 100, 50));
    objectGenerators.push_back(new type_treasure_def(57, 0, 3500, 200));
    objectGenerators.push_back(new type_treasure_def(58, 0, 750, 100));
    objectGenerators.push_back(new type_treasure_def(60, 0, 750, 100));
    objectGenerators.push_back(new type_treasure_def(61, 0, 1500, 100));

    objectGenerators.push_back(new type_prison_def(2500, 0));
    objectGenerators.push_back(new type_prison_def(5000, 5000));
    objectGenerators.push_back(new type_prison_def(10000, 15000));
    objectGenerators.push_back(new type_prison_def(20000, 90000));
    objectGenerators.push_back(new type_prison_def(30000, 500000));
    objectGenerators.push_back(new type_treasure_def(63, 0, 5000, 20));
    objectGenerators.push_back(new type_treasure_def(64, 0, 100, 100));

    objectGenerators.push_back(new type_artifact_def(66, 2000));
    objectGenerators.push_back(new type_artifact_def(67, 5000));
    objectGenerators.push_back(new type_artifact_def(68, 10000));
    objectGenerators.push_back(new type_artifact_def(69, 20000));

    objectGenerators.push_back(new type_resource_lump_def(76, 0, 1500, 2000));
    objectGenerators.push_back(new type_treasure_def(78, 0, 5000, 20));
    objectGenerators.push_back(new type_resource_lump_def(79, 0, 1400, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 2, 1400, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 1, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 3, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 4, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 5, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 6, 750, 300));
    objectGenerators.push_back(new type_treasure_def(80, 0, 100, 50));
    objectGenerators.push_back(new type_scholar_def());
    objectGenerators.push_back(new type_treasure_def(82, 0, 1500, 500));

    for (int quest = 0; quest < questSlots.size(); ++quest) {
        int creatureCount = mapVersion >= 1 ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (akCreatureTypeTraits[creature].level >= 0)
                objectGenerators.push_back(
                    new type_quest_creature_def(creature, quest));
        }

        objectGenerators.push_back(
            new type_quest_experience_def(quest, 2000, 5000));
        objectGenerators.push_back(
            new type_quest_experience_def(quest, 5333, 10000));
        objectGenerators.push_back(
            new type_quest_experience_def(quest, 8666, 15000));
        objectGenerators.push_back(
            new type_quest_experience_def(quest, 12000, 20000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 2000, 5000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 5333, 10000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 8666, 15000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 12000, 20000));
    }

    objectGenerators.push_back(new type_treasure_def(84, 0, 1000, 100));
    objectGenerators.push_back(new type_treasure_def(85, 0, 2000, 100));
    objectGenerators.push_back(new type_treasure_def(86, 0, 1500, 50));
    objectGenerators.push_back(new type_shrine_def(88, 500));
    objectGenerators.push_back(new type_shrine_def(89, 2000));
    objectGenerators.push_back(new type_shrine_def(90, 3000));
    objectGenerators.push_back(new type_treasure_def(92, 0, 100, 20));
    objectGenerators.push_back(new type_spell_scroll_def(1, 500));
    objectGenerators.push_back(new type_spell_scroll_def(2, 2000));
    objectGenerators.push_back(new type_spell_scroll_def(3, 3000));
    objectGenerators.push_back(new type_spell_scroll_def(4, 4000));
    objectGenerators.push_back(new type_spell_scroll_def(5, 5000));
    objectGenerators.push_back(new type_treasure_def(94, 0, 200, 40));
    objectGenerators.push_back(new type_treasure_def(95, 0, 100, 20));
    objectGenerators.push_back(new type_treasure_def(96, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(97, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(99, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(100, 0, 1500, 200));
    objectGenerators.push_back(new type_treasure_def(101, 0, 1500, 1000));
    objectGenerators.push_back(new type_treasure_def(102, 0, 2500, 50));
    objectGenerators.push_back(new type_treasure_def(104, 0, 2500, 20));
    objectGenerators.push_back(new type_treasure_def(105, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(106, 0, 1500, 50));
    objectGenerators.push_back(new type_treasure_def(107, 0, 1000, 50));
    objectGenerators.push_back(new type_treasure_def(108, 0, 6000, 20));
    objectGenerators.push_back(new type_treasure_def(109, 0, 750, 50));
    objectGenerators.push_back(new type_treasure_def(110, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(112, 0, 2500, 150));
    objectGenerators.push_back(new type_witch_hut_def());
}

// Retail 0x5b8bc0 is the nine-block Dinkumware tree-successor walk. Its
// sentinel at 0x6a52c4 is shared only by the RMG set cluster, while the
// enclosing callers lead back to GenerateRandomMap. Dreamcast's STLport
// _M_increment at dc 0x64214 corroborates the helper boundary and CFG shape;
// the RMG type itself is retail-only.
VA_COMPGEN(0x005B8BC0, 0xA3, TREE_CONST_ITERATOR_INC, TPoint)

// Minimum ODR use needed to retain the real VC6/Dinkumware COMDAT. This
// wrapper is not a retail claim and adds no target/report row.
void __fastcall EmitRmgPointSetIncrement(TRmgPointSet::const_iterator* it)
{
    ++*it;
}
