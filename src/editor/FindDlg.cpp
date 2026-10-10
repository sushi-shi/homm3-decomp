// FindDlg.cpp - the Edit > Find dialog (h3maped 0x41a7b8..0x41df9c; GOG
// only). Its list is a table of patterns built once, in display order:
// the general object types first, then the artifacts, creature banks,
// dwellings, garrisons, hero classes, mines, creatures, resources and town
// types, each matching an object's type and (for the latter) subtype.
#include "editor/stdafx.h"

#include <vector>

#include "artifact.h"
#include "objnames.h"
#include "retailobjecttype.h"
#include "va.h"
#include "editor/FindDlg.h"
#include "editor/GameObject.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/ObjectSpecializations.h"
#include "editor/Town.h"

namespace {

// One entry of the find list: its name and the objects it finds.
class TObjectPattern {
public:
    TObjectPattern(const char* pName) : _m_pName(pName) {}
    virtual ~TObjectPattern() {}

    virtual bool matches(const TGameObject& object) const = 0;

    const char* getName() const { return _m_pName; }

private:
    const char* _m_pName;
};

// The objects of one type.
class TSingleTypePattern : public TObjectPattern {
public:
    TSingleTypePattern(const char* pName, int type) : TObjectPattern(pName), _m_type(type) {}

    virtual bool matches(const TGameObject& object) const;

private:
    int _m_type;
};

// The objects of one type and subtype.
class TSingleTypeSingleExtraPattern : public TObjectPattern {
public:
    TSingleTypeSingleExtraPattern(const char* pName, int type, int extra)
        : TObjectPattern(pName), _m_type(type), _m_extra(extra) {}

    virtual bool matches(const TGameObject& object) const;

private:
    int _m_type;
    int _m_extra;
};

// The objects of one type and either of two subtypes.
class TSingleTypeDoubleExtraPattern : public TObjectPattern {
public:
    TSingleTypeDoubleExtraPattern(const char* pName, int type, int extra1, int extra2)
        : TObjectPattern(pName), _m_type(type), _m_extra1(extra1), _m_extra2(extra2) {}

    virtual bool matches(const TGameObject& object) const;

private:
    int _m_type;
    int _m_extra1;
    int _m_extra2;
};

// The objects of either of two types.
class TDoubleTypePattern : public TObjectPattern {
public:
    TDoubleTypePattern(const char* pName, int type1, int type2)
        : TObjectPattern(pName), _m_type1(type1), _m_type2(type2) {}

    virtual bool matches(const TGameObject& object) const;

private:
    int _m_type1;
    int _m_type2;
};

// The objects of either of two types and one subtype.
class TDoubleTypeSingleExtraPattern : public TObjectPattern {
public:
    TDoubleTypeSingleExtraPattern(const char* pName, int type1, int type2, int extra)
        : TObjectPattern(pName), _m_type1(type1), _m_type2(type2), _m_extra(extra) {}

    virtual bool matches(const TGameObject& object) const;

private:
    int _m_type1;
    int _m_type2;
    int _m_extra;
};

// An object type, named after it.
class TGeneralSingleTypePattern : public TSingleTypePattern {
public:
    TGeneralSingleTypePattern(int type) : TSingleTypePattern(akAdvObjectTypeTraits[type].m_name, type) {}
};

// An object type and its second spelling, named after the first.
class TGeneralDoubleTypePattern : public TDoubleTypePattern {
public:
    TGeneralDoubleTypePattern(int type1, int type2)
        : TDoubleTypePattern(akAdvObjectTypeTraits[type1].m_name, type1, type2) {}
};

class TArtifactPattern : public TSingleTypeSingleExtraPattern {
public:
    TArtifactPattern(int artifact)
        : TSingleTypeSingleExtraPattern(akArtifactTraits[artifact].m_name, ARTIFACT, artifact) {}
};

class TCreatureBankPattern : public TSingleTypeSingleExtraPattern {
public:
    TCreatureBankPattern(int bank)
        : TSingleTypeSingleExtraPattern(akCreatureBankTypeTraits[bank].m_name, CREATURE_BANK, bank) {}
};

class TGenerator1Pattern : public TSingleTypeSingleExtraPattern {
public:
    TGenerator1Pattern(int generator)
        : TSingleTypeSingleExtraPattern(TGenerator::s_akGenerator1TypeTraits[generator].m_name, CREATURE_GENERATOR_1,
                                        generator) {}
};

// A dwelling and its second subtype (the same creatures).
class TGenerator1DoublePattern : public TSingleTypeDoubleExtraPattern {
public:
    TGenerator1DoublePattern(int generator1, int generator2)
        : TSingleTypeDoubleExtraPattern(TGenerator::s_akGenerator1TypeTraits[generator1].m_name,
                                        CREATURE_GENERATOR_1, generator1, generator2) {}
};

class TGenerator4Pattern : public TSingleTypeSingleExtraPattern {
public:
    TGenerator4Pattern(int generator)
        : TSingleTypeSingleExtraPattern(TGenerator::s_akGenerator4TypeTraits[generator].m_name, CREATURE_GENERATOR_4,
                                        generator) {}
};

class TGarrisonPattern : public TDoubleTypeSingleExtraPattern {
public:
    TGarrisonPattern(int garrison)
        : TDoubleTypeSingleExtraPattern(TGarrison::s_akTypeTraits[garrison].m_name, GARRISON, GARRISON2, garrison) {}
};

class THeroClassPattern : public TSingleTypeSingleExtraPattern {
public:
    THeroClassPattern(int heroClass)
        : TSingleTypeSingleExtraPattern(THero::s_akClassTraits[heroClass].m_name, HERO, heroClass) {}
};

class TMinePattern : public TDoubleTypeSingleExtraPattern {
public:
    TMinePattern(int mine)
        : TDoubleTypeSingleExtraPattern(TMine::s_akMineTypeTraits[mine].m_name, MINE, ABANDONED_MINE, mine) {}
};

class TCreaturePattern : public TSingleTypeSingleExtraPattern {
public:
    TCreaturePattern(int creature)
        : TSingleTypeSingleExtraPattern(akCreatureTypeTraits[creature].m_name, MONSTER, creature) {}
};

class TResourcePattern : public TSingleTypeSingleExtraPattern {
public:
    TResourcePattern(int resource)
        : TSingleTypeSingleExtraPattern(akGameResourceTypeTraits[resource].m_name, RESOURCE, resource) {}
};

class TTownTypePattern : public TSingleTypeSingleExtraPattern {
public:
    TTownTypePattern(int townType)
        : TSingleTypeSingleExtraPattern(TTown::s_akTypeTraits[townType].m_pName, TOWN, townType) {}
};

// The find list, built on first use.
class TPatterns : public std::vector<const TObjectPattern*> {
public:
    TPatterns();

private:
    // Appends a table's entries (one instance per table type).
    template <class TTable>
    void _add(const TTable& aPattern)
    {
        for (unsigned int i = 0; i < sizeof(aPattern) / sizeof(aPattern[0]); i++)
            push_back(&aPattern[i]);
    }
};

}

VA(0x0041a7b8, 0x15)
bool TSingleTypePattern::matches(const TGameObject& object) const
{
    return object.getType() == _m_type;
}

VA(0x0041a7cd, 0x21)
bool TSingleTypeSingleExtraPattern::matches(const TGameObject& object) const
{
    return object.getType() == _m_type && object.getExtra() == _m_extra;
}

VA(0x0041a7ee, 0x26)
bool TSingleTypeDoubleExtraPattern::matches(const TGameObject& object) const
{
    return object.getType() == _m_type && (object.getExtra() == _m_extra1 || object.getExtra() == _m_extra2);
}

VA(0x0041a814, 0x1e)
bool TDoubleTypePattern::matches(const TGameObject& object) const
{
    return object.getType() == _m_type1 || object.getType() == _m_type2;
}

VA(0x0041a832, 0x26)
bool TDoubleTypeSingleExtraPattern::matches(const TGameObject& object) const
{
    return (object.getType() == _m_type1 || object.getType() == _m_type2) && object.getExtra() == _m_extra;
}

VA(0x0041a858, 0x2c57)
TPatterns::TPatterns()
{
    DATA_COMPGEN_GUARD(0x0059d1fc, generalPatternsGuard, akGeneralSingleTypePatterns)
    VA_COMPGEN(0x0041d809, 0x14, STATIC_DTOR, akGeneralSingleTypePatterns)
    DATA(0x0059cc50) static const TGeneralSingleTypePattern akGeneralSingleTypePatterns[] = {
        TGeneralSingleTypePattern(ALTAR_OF_SACRIFICE),
        TGeneralSingleTypePattern(ARENA),
        TGeneralSingleTypePattern(ARTIFACT),
        TGeneralSingleTypePattern(BLACK_BOX),
        TGeneralSingleTypePattern(BLACK_MARKET),
        TGeneralSingleTypePattern(BOAT),
        TGeneralSingleTypePattern(BORDER_GUARD),
        TGeneralSingleTypePattern(BORDER_TENT),
        TGeneralSingleTypePattern(BUOY),
        TGeneralSingleTypePattern(CAMPFIRE),
        TGeneralSingleTypePattern(CARTOGRAPHER),
        TGeneralSingleTypePattern(CLOVER_FIELD),
        TGeneralSingleTypePattern(COVER_OF_DARKNESS),
        TGeneralSingleTypePattern(DEAD_GUY),
        TGeneralSingleTypePattern(DEFENSE_TOWER),
        TGeneralSingleTypePattern(DERELICT_SHIP),
        TGeneralSingleTypePattern(DRAGON_CITY),
        TGeneralSingleTypePattern(EVENT),
        TGeneralSingleTypePattern(EYE_OF_MAGI),
        TGeneralSingleTypePattern(FAERIE_RING),
        TGeneralSingleTypePattern(FLOTSAM),
        TGeneralSingleTypePattern(FOUNTAIN_OF_FORTUNE),
        TGeneralSingleTypePattern(FOUNTAIN_OF_YOUTH),
        TGeneralSingleTypePattern(GARDEN_OF_REVELATION),
        TGeneralSingleTypePattern(HERO),
        TGeneralSingleTypePattern(HILL_FORT),
        TGeneralSingleTypePattern(HOLY_GRAIL),
        TGeneralSingleTypePattern(HUT_OF_MAGI),
        TGeneralSingleTypePattern(IDOL_OF_FORTUNE),
        TGeneralSingleTypePattern(LEAN_TO),
        TGeneralSingleTypePattern(LIBRARY),
        TGeneralSingleTypePattern(LIGHTHOUSE),
        TGeneralSingleTypePattern(LITH_ONEWAY_ENTRANCE),
        TGeneralSingleTypePattern(LITH_ONEWAY_EXIT),
        TGeneralSingleTypePattern(LITH_TWOWAY),
        TGeneralSingleTypePattern(MAGIC_SCHOOL),
        TGeneralSingleTypePattern(MAGIC_SPRING),
        TGeneralSingleTypePattern(MAGIC_WELL),
        TGeneralSingleTypePattern(MERC_CAMP),
        TGeneralSingleTypePattern(MERMAID),
        TGeneralSingleTypePattern(MONSTER),
        TGeneralSingleTypePattern(MYSTICAL_GARDEN),
        TGeneralSingleTypePattern(OASIS),
        TGeneralSingleTypePattern(OBELISK),
        TGeneralSingleTypePattern(OBSERVATORY),
        TGeneralSingleTypePattern(OCEAN_BOTTLE),
        TGeneralSingleTypePattern(PILLAR_OF_FIRE),
        TGeneralSingleTypePattern(POWER_SCHOOL),
        TGeneralSingleTypePattern(PRISON),
        TGeneralSingleTypePattern(PYRAMID),
        TGeneralSingleTypePattern(RALLY_FLAG),
        TGeneralSingleTypePattern(RANDOM_ARTIFACT),
        TGeneralSingleTypePattern(RANDOM_ARTIFACT_1),
        TGeneralSingleTypePattern(RANDOM_ARTIFACT_2),
        TGeneralSingleTypePattern(RANDOM_ARTIFACT_3),
        TGeneralSingleTypePattern(RANDOM_ARTIFACT_4),
        TGeneralSingleTypePattern(RANDOM_HERO),
        TGeneralSingleTypePattern(RANDOM_MONSTER),
        TGeneralSingleTypePattern(RANDOM_MONSTER_1),
        TGeneralSingleTypePattern(RANDOM_MONSTER_2),
        TGeneralSingleTypePattern(RANDOM_MONSTER_3),
        TGeneralSingleTypePattern(RANDOM_MONSTER_4),
        TGeneralSingleTypePattern(RANDOM_RESOURCE),
        TGeneralSingleTypePattern(RANDOM_TOWN),
        TGeneralSingleTypePattern(REFUGEE_CAMP),
        TGeneralSingleTypePattern(RESOURCE),
        TGeneralSingleTypePattern(SANCTUARY),
        TGeneralSingleTypePattern(SCHOLAR),
        TGeneralSingleTypePattern(SEA_CHEST),
        TGeneralSingleTypePattern(SEER),
        TGeneralSingleTypePattern(SEPULCHER),
        TGeneralSingleTypePattern(SHIPWRECK),
        TGeneralSingleTypePattern(SHIPWRECK_SURVIVOR),
        TGeneralSingleTypePattern(SHIPYARD),
        TGeneralSingleTypePattern(SHRINE1),
        TGeneralSingleTypePattern(SHRINE2),
        TGeneralSingleTypePattern(SHRINE3),
        TGeneralSingleTypePattern(SIGN),
        TGeneralSingleTypePattern(SIREN),
        TGeneralSingleTypePattern(SPELL_SCROLL),
        TGeneralSingleTypePattern(STABLES),
        TGeneralSingleTypePattern(TAVERN),
        TGeneralSingleTypePattern(TEMPLE),
        TGeneralSingleTypePattern(THIEVES_DEN),
        TGeneralSingleTypePattern(TOWN),
        TGeneralSingleTypePattern(TRAINING_GROUNDS),
        TGeneralSingleTypePattern(TREASURE_CHEST),
        TGeneralSingleTypePattern(TREE_OF_KNOWLEDGE),
        TGeneralSingleTypePattern(UNDERGROUND_GATE),
        TGeneralSingleTypePattern(UNIVERSITY),
        TGeneralSingleTypePattern(WAGON),
        TGeneralSingleTypePattern(WAR_MACHINE_FACTORY),
        TGeneralSingleTypePattern(WAR_SCHOOL),
        TGeneralSingleTypePattern(WARRIOR_TOMB),
        TGeneralSingleTypePattern(WATER_WHEEL),
        TGeneralSingleTypePattern(WATERING_HOLE),
        TGeneralSingleTypePattern(WHIRLPOOL),
        TGeneralSingleTypePattern(WINDMILL),
        TGeneralSingleTypePattern(WITCH_HUT),
        TGeneralSingleTypePattern(RANDOM_MONSTER_5),
        TGeneralSingleTypePattern(RANDOM_MONSTER_6),
        TGeneralSingleTypePattern(RANDOM_MONSTER_7),
        TGeneralSingleTypePattern(TERRAIN_DESERT_HILLS),
        TGeneralSingleTypePattern(TERRAIN_DIRT_HILLS),
        TGeneralSingleTypePattern(TERRAIN_GRASS_HILLS),
        TGeneralSingleTypePattern(TERRAIN_ROUGH_HILLS),
        TGeneralSingleTypePattern(TERRAIN_SUBTERRANEAN_ROCKS),
        TGeneralSingleTypePattern(TERRAIN_SWAMP_FOLIAGE),
        TGeneralSingleTypePattern(BORDER_GATE),
        TGeneralSingleTypePattern(FREELANCERS_GUILD),
        TGeneralSingleTypePattern(HERO_PLACEHOLDER),
        TGeneralSingleTypePattern(QUEST_GUARD),
        TGeneralSingleTypePattern(RANDOM_DWELLING),
        TGeneralSingleTypePattern(CLOVER_FIELD_2),
        TGeneralSingleTypePattern(EVIL_FOG),
        TGeneralSingleTypePattern(FAVORABLE_WINDS),
        TGeneralSingleTypePattern(FIERY_FIELDS),
        TGeneralSingleTypePattern(HOLY_GROUND),
        TGeneralSingleTypePattern(LUCID_POOLS),
        TGeneralSingleTypePattern(MAGIC_CLOUDS),
        TGeneralSingleTypePattern(ROCKLANDS)
    };
    VA_COMPGEN(0x0041d7f5, 0x14, STATIC_DTOR, akGeneralDoubleTypePatterns)
    DATA(0x0059c980) static const TGeneralDoubleTypePattern akGeneralDoubleTypePatterns[] = {
        TGeneralDoubleTypePattern(MINE, ABANDONED_MINE),
        TGeneralDoubleTypePattern(TRADING_POST, TRADING_POST_2),
        TGeneralDoubleTypePattern(TERRAIN_BRUSH, MAX_EVENT_TYPE),
        TGeneralDoubleTypePattern(TERRAIN_BUSH, TERRAIN_BUSH_2),
        TGeneralDoubleTypePattern(TERRAIN_CACTUS, TERRAIN_CACTUS_2),
        TGeneralDoubleTypePattern(TERRAIN_CANYON, TERRAIN_CANYON_2),
        TGeneralDoubleTypePattern(TERRAIN_CRATER, TERRAIN_CRATER_2),
        TGeneralDoubleTypePattern(TERRAIN_DEAD_VEGETATION, TERRAIN_DEAD_VEGETATION_2),
        TGeneralDoubleTypePattern(TERRAIN_FLOWER, TERRAIN_FLOWER_2),
        TGeneralDoubleTypePattern(TERRAIN_FROZEN_LAKE, TERRAIN_FROZEN_LAKE_2),
        TGeneralDoubleTypePattern(TERRAIN_HEDGE, TERRAIN_HEDGE_2),
        TGeneralDoubleTypePattern(TERRAIN_HILL, TERRAIN_HILL_2),
        TGeneralDoubleTypePattern(TERRAIN_HOLE, TERRAIN_HOLE_2),
        TGeneralDoubleTypePattern(TERRAIN_KELP, TERRAIN_KELP_2),
        TGeneralDoubleTypePattern(TERRAIN_LAKE, TERRAIN_LAKE_2),
        TGeneralDoubleTypePattern(TERRAIN_LAVA_FLOW, TERRAIN_LAVA_FLOW_2),
        TGeneralDoubleTypePattern(TERRAIN_LAVA_LAKE, TERRAIN_LAVA_LAKE_2),
        TGeneralDoubleTypePattern(TERRAIN_MUSHROOM, TERRAIN_MUSHROOM_2),
        TGeneralDoubleTypePattern(TERRAIN_LOG, TERRAIN_LOG_2),
        TGeneralDoubleTypePattern(TERRAIN_MANDRAKE, TERRAIN_MANDRAKE_2),
        TGeneralDoubleTypePattern(TERRAIN_MOSS, TERRAIN_MOSS_2),
        TGeneralDoubleTypePattern(TERRAIN_MOUND, TERRAIN_MOUND_2),
        TGeneralDoubleTypePattern(TERRAIN_MOUNTAIN, TERRAIN_MOUNTAIN_2),
        TGeneralDoubleTypePattern(TERRAIN_OAK_TREE, TERRAIN_OAK_TREE_2),
        TGeneralDoubleTypePattern(TERRAIN_OUTCROPPING, TERRAIN_OUTCROPPING_2),
        TGeneralDoubleTypePattern(TERRAIN_PINE_TREE, TERRAIN_PINE_TREE_2),
        TGeneralDoubleTypePattern(TERRAIN_PLANT, TERRAIN_PLANT_2),
        TGeneralDoubleTypePattern(TERRAIN_RIVER_DELTA, TERRAIN_RIVER_DELTA_2),
        TGeneralDoubleTypePattern(TERRAIN_ROCK, TERRAIN_ROCK_2),
        TGeneralDoubleTypePattern(TERRAIN_SAND_DUNE, TERRAIN_SAND_DUNE_2),
        TGeneralDoubleTypePattern(TERRAIN_SAND_PIT, TERRAIN_SAND_PIT_2),
        TGeneralDoubleTypePattern(TERRAIN_SHRUB, TERRAIN_SHRUB_2),
        TGeneralDoubleTypePattern(TERRAIN_SKULL, TERRAIN_SKULL_2),
        TGeneralDoubleTypePattern(TERRAIN_STALAGMITE, TERRAIN_STALAGMITE_2),
        TGeneralDoubleTypePattern(TERRAIN_STUMP, TERRAIN_STUMP_2),
        TGeneralDoubleTypePattern(TERRAIN_TAR_PIT, TERRAIN_TAR_PIT_2),
        TGeneralDoubleTypePattern(TERRAIN_TREE, TERRAIN_TREE_2),
        TGeneralDoubleTypePattern(TERRAIN_VINE, TERRAIN_VINE_2),
        TGeneralDoubleTypePattern(TERRAIN_VOLCANIC_VENT, TERRAIN_VOLCANIC_VENT_2),
        TGeneralDoubleTypePattern(TERRAIN_VOLCANO, TERRAIN_VOLCANO_2),
        TGeneralDoubleTypePattern(TERRAIN_WILLOW_TREE, TERRAIN_WILLOW_TREE_2),
        TGeneralDoubleTypePattern(TERRAIN_YUCCA_TREE, TERRAIN_YUCCA_TREE_2),
        TGeneralDoubleTypePattern(TERRAIN_REEF, TERRAIN_REEF_2),
        TGeneralDoubleTypePattern(CURSED_GROUND, CURSED_GROUND_2),
        TGeneralDoubleTypePattern(MAGIC_PLAINS, MAGIC_PLAINS_2)
    };
    VA_COMPGEN(0x0041d7de, 0x17, STATIC_DTOR, akArtifactPatterns)
    DATA(0x0059c120) static const TArtifactPattern akArtifactPatterns[] = {
        TArtifactPattern(7),
        TArtifactPattern(8),
        TArtifactPattern(9),
        TArtifactPattern(10),
        TArtifactPattern(11),
        TArtifactPattern(12),
        TArtifactPattern(13),
        TArtifactPattern(14),
        TArtifactPattern(15),
        TArtifactPattern(16),
        TArtifactPattern(17),
        TArtifactPattern(18),
        TArtifactPattern(19),
        TArtifactPattern(20),
        TArtifactPattern(21),
        TArtifactPattern(22),
        TArtifactPattern(23),
        TArtifactPattern(24),
        TArtifactPattern(25),
        TArtifactPattern(26),
        TArtifactPattern(27),
        TArtifactPattern(28),
        TArtifactPattern(29),
        TArtifactPattern(30),
        TArtifactPattern(31),
        TArtifactPattern(32),
        TArtifactPattern(33),
        TArtifactPattern(34),
        TArtifactPattern(35),
        TArtifactPattern(36),
        TArtifactPattern(37),
        TArtifactPattern(38),
        TArtifactPattern(39),
        TArtifactPattern(40),
        TArtifactPattern(41),
        TArtifactPattern(42),
        TArtifactPattern(43),
        TArtifactPattern(44),
        TArtifactPattern(45),
        TArtifactPattern(46),
        TArtifactPattern(47),
        TArtifactPattern(48),
        TArtifactPattern(49),
        TArtifactPattern(50),
        TArtifactPattern(51),
        TArtifactPattern(52),
        TArtifactPattern(53),
        TArtifactPattern(54),
        TArtifactPattern(55),
        TArtifactPattern(56),
        TArtifactPattern(57),
        TArtifactPattern(58),
        TArtifactPattern(59),
        TArtifactPattern(60),
        TArtifactPattern(61),
        TArtifactPattern(62),
        TArtifactPattern(63),
        TArtifactPattern(64),
        TArtifactPattern(65),
        TArtifactPattern(66),
        TArtifactPattern(67),
        TArtifactPattern(68),
        TArtifactPattern(69),
        TArtifactPattern(70),
        TArtifactPattern(71),
        TArtifactPattern(72),
        TArtifactPattern(73),
        TArtifactPattern(74),
        TArtifactPattern(75),
        TArtifactPattern(76),
        TArtifactPattern(77),
        TArtifactPattern(78),
        TArtifactPattern(79),
        TArtifactPattern(80),
        TArtifactPattern(81),
        TArtifactPattern(82),
        TArtifactPattern(83),
        TArtifactPattern(84),
        TArtifactPattern(85),
        TArtifactPattern(86),
        TArtifactPattern(87),
        TArtifactPattern(88),
        TArtifactPattern(89),
        TArtifactPattern(90),
        TArtifactPattern(91),
        TArtifactPattern(92),
        TArtifactPattern(93),
        TArtifactPattern(94),
        TArtifactPattern(95),
        TArtifactPattern(96),
        TArtifactPattern(97),
        TArtifactPattern(98),
        TArtifactPattern(99),
        TArtifactPattern(100),
        TArtifactPattern(101),
        TArtifactPattern(102),
        TArtifactPattern(103),
        TArtifactPattern(104),
        TArtifactPattern(105),
        TArtifactPattern(106),
        TArtifactPattern(107),
        TArtifactPattern(108),
        TArtifactPattern(109),
        TArtifactPattern(110),
        TArtifactPattern(111),
        TArtifactPattern(112),
        TArtifactPattern(113),
        TArtifactPattern(114),
        TArtifactPattern(115),
        TArtifactPattern(116),
        TArtifactPattern(117),
        TArtifactPattern(118),
        TArtifactPattern(119),
        TArtifactPattern(120),
        TArtifactPattern(121),
        TArtifactPattern(122),
        TArtifactPattern(123),
        TArtifactPattern(124),
        TArtifactPattern(125),
        TArtifactPattern(126),
        TArtifactPattern(127),
        TArtifactPattern(128),
        TArtifactPattern(129),
        TArtifactPattern(130),
        TArtifactPattern(131),
        TArtifactPattern(132),
        TArtifactPattern(133),
        TArtifactPattern(134),
        TArtifactPattern(135),
        TArtifactPattern(136),
        TArtifactPattern(137),
        TArtifactPattern(138),
        TArtifactPattern(139),
        TArtifactPattern(140)
    };
    VA_COMPGEN(0x0041d7ca, 0x14, STATIC_DTOR, akCreatureBankPatterns)
    DATA(0x0059c0b0) static const TCreatureBankPattern akCreatureBankPatterns[] = {
        TCreatureBankPattern(0),
        TCreatureBankPattern(1),
        TCreatureBankPattern(2),
        TCreatureBankPattern(3),
        TCreatureBankPattern(4),
        TCreatureBankPattern(5),
        TCreatureBankPattern(6)
    };
    VA_COMPGEN(0x0041d7b6, 0x14, STATIC_DTOR, akGenerator1Patterns)
    DATA(0x0059bc50) static const TGenerator1Pattern akGenerator1Patterns[] = {
        TGenerator1Pattern(0),
        TGenerator1Pattern(1),
        TGenerator1Pattern(2),
        TGenerator1Pattern(3),
        TGenerator1Pattern(4),
        TGenerator1Pattern(5),
        TGenerator1Pattern(6),
        TGenerator1Pattern(8),
        TGenerator1Pattern(9),
        TGenerator1Pattern(10),
        TGenerator1Pattern(11),
        TGenerator1Pattern(12),
        TGenerator1Pattern(14),
        TGenerator1Pattern(15),
        TGenerator1Pattern(17),
        TGenerator1Pattern(18),
        TGenerator1Pattern(19),
        TGenerator1Pattern(20),
        TGenerator1Pattern(21),
        TGenerator1Pattern(22),
        TGenerator1Pattern(23),
        TGenerator1Pattern(24),
        TGenerator1Pattern(25),
        TGenerator1Pattern(26),
        TGenerator1Pattern(27),
        TGenerator1Pattern(28),
        TGenerator1Pattern(29),
        TGenerator1Pattern(30),
        TGenerator1Pattern(31),
        TGenerator1Pattern(32),
        TGenerator1Pattern(33),
        TGenerator1Pattern(34),
        TGenerator1Pattern(35),
        TGenerator1Pattern(36),
        TGenerator1Pattern(37),
        TGenerator1Pattern(38),
        TGenerator1Pattern(39),
        TGenerator1Pattern(40),
        TGenerator1Pattern(41),
        TGenerator1Pattern(42),
        TGenerator1Pattern(43),
        TGenerator1Pattern(44),
        TGenerator1Pattern(45),
        TGenerator1Pattern(46),
        TGenerator1Pattern(48),
        TGenerator1Pattern(49),
        TGenerator1Pattern(50),
        TGenerator1Pattern(52),
        TGenerator1Pattern(53),
        TGenerator1Pattern(54),
        TGenerator1Pattern(55),
        TGenerator1Pattern(56),
        TGenerator1Pattern(57),
        TGenerator1Pattern(58),
        TGenerator1Pattern(59),
        TGenerator1Pattern(60),
        TGenerator1Pattern(61),
        TGenerator1Pattern(62),
        TGenerator1Pattern(63),
        TGenerator1Pattern(64),
        TGenerator1Pattern(65),
        TGenerator1Pattern(66),
        TGenerator1Pattern(67),
        TGenerator1Pattern(73),
        TGenerator1Pattern(74),
        TGenerator1Pattern(75),
        TGenerator1Pattern(76),
        TGenerator1Pattern(77),
        TGenerator1Pattern(78),
        TGenerator1Pattern(79)
    };
    VA_COMPGEN(0x0041d7a2, 0x14, STATIC_DTOR, akGenerator1DoublePatterns)
    DATA(0x0059bbe8) static const TGenerator1DoublePattern akGenerator1DoublePatterns[] = {
        TGenerator1DoublePattern(51, 68),
        TGenerator1DoublePattern(7, 69),
        TGenerator1DoublePattern(13, 70),
        TGenerator1DoublePattern(16, 71),
        TGenerator1DoublePattern(47, 72)
    };
    VA_COMPGEN(0x0041d78e, 0x14, STATIC_DTOR, akGenerator4Patterns)
    DATA(0x0059bbc8) static const TGenerator4Pattern akGenerator4Patterns[] = {
        TGenerator4Pattern(0),
        TGenerator4Pattern(1)
    };
    VA_COMPGEN(0x0041d77a, 0x14, STATIC_DTOR, akGarrisonPatterns)
    DATA(0x0059bba0) static const TGarrisonPattern akGarrisonPatterns[] = {
        TGarrisonPattern(0),
        TGarrisonPattern(1)
    };
    DATA_COMPGEN_GUARD(0x0059bb98, heroClassPatternsGuard, akHeroClassPatterns)
    VA_COMPGEN(0x0041d766, 0x14, STATIC_DTOR, akHeroClassPatterns)
    DATA(0x0059ba98) static const THeroClassPattern akHeroClassPatterns[] = {
        THeroClassPattern(classKnight),
        THeroClassPattern(classCleric),
        THeroClassPattern(classRanger),
        THeroClassPattern(classDruid),
        THeroClassPattern(classAlchemist),
        THeroClassPattern(classWizard),
        THeroClassPattern(classPagan),
        THeroClassPattern(classHeretic),
        THeroClassPattern(classDeathKnight),
        THeroClassPattern(classNecromancer),
        THeroClassPattern(classOverlord),
        THeroClassPattern(classWarlock),
        THeroClassPattern(classBarbarian),
        THeroClassPattern(classBattleMage),
        THeroClassPattern(classBeastmaster),
        THeroClassPattern(classWitch)
    };
    VA_COMPGEN(0x0041d752, 0x14, STATIC_DTOR, akMinePatterns)
    DATA(0x0059b9f8) static const TMinePattern akMinePatterns[] = {
        TMinePattern(0),
        TMinePattern(1),
        TMinePattern(2),
        TMinePattern(3),
        TMinePattern(4),
        TMinePattern(5),
        TMinePattern(6),
        TMinePattern(7)
    };
    VA_COMPGEN(0x0041d73b, 0x17, STATIC_DTOR, akCreaturePatterns)
    DATA(0x0059b128) static const TCreaturePattern akCreaturePatterns[] = {
        TCreaturePattern(CREATURE_PIKEMAN),
        TCreaturePattern(CREATURE_HALBERDIER),
        TCreaturePattern(CREATURE_ARCHER),
        TCreaturePattern(CREATURE_MARKSMAN),
        TCreaturePattern(CREATURE_GRIFFIN),
        TCreaturePattern(CREATURE_ROYAL_GRIFFIN),
        TCreaturePattern(CREATURE_SWORDSMAN),
        TCreaturePattern(CREATURE_CRUSADER),
        TCreaturePattern(CREATURE_MONK),
        TCreaturePattern(CREATURE_ZEALOT),
        TCreaturePattern(CREATURE_CAVALIER),
        TCreaturePattern(CREATURE_CHAMPION),
        TCreaturePattern(CREATURE_ANGEL),
        TCreaturePattern(CREATURE_ARCHANGEL),
        TCreaturePattern(CREATURE_CENTAUR),
        TCreaturePattern(CREATURE_CENTAUR_CAPTAIN),
        TCreaturePattern(CREATURE_DWARF),
        TCreaturePattern(CREATURE_BATTLE_DWARF),
        TCreaturePattern(CREATURE_WOOD_ELF),
        TCreaturePattern(CREATURE_GRAND_ELF),
        TCreaturePattern(CREATURE_PEGASUS),
        TCreaturePattern(CREATURE_SILVER_PEGASUS),
        TCreaturePattern(CREATURE_DENDROID_GUARD),
        TCreaturePattern(CREATURE_DENDROID_SOLDIER),
        TCreaturePattern(CREATURE_UNICORN),
        TCreaturePattern(CREATURE_WAR_UNICORN),
        TCreaturePattern(CREATURE_GREEN_DRAGON),
        TCreaturePattern(CREATURE_GOLD_DRAGON),
        TCreaturePattern(CREATURE_GREMLIN),
        TCreaturePattern(CREATURE_MASTER_GREMLIN),
        TCreaturePattern(CREATURE_STONE_GARGOYLE),
        TCreaturePattern(CREATURE_OBSIDIAN_GARGOYLE),
        TCreaturePattern(CREATURE_STONE_GOLEM),
        TCreaturePattern(CREATURE_IRON_GOLEM),
        TCreaturePattern(CREATURE_MAGE),
        TCreaturePattern(CREATURE_ARCH_MAGE),
        TCreaturePattern(CREATURE_GENIE),
        TCreaturePattern(CREATURE_MASTER_GENIE),
        TCreaturePattern(CREATURE_NAGA_SENTINEL),
        TCreaturePattern(CREATURE_NAGA_QUEEN),
        TCreaturePattern(CREATURE_GIANT),
        TCreaturePattern(CREATURE_TITAN),
        TCreaturePattern(CREATURE_IMP),
        TCreaturePattern(CREATURE_FAMILIAR),
        TCreaturePattern(CREATURE_GOG),
        TCreaturePattern(CREATURE_MAGOG),
        TCreaturePattern(CREATURE_HELL_HOUND),
        TCreaturePattern(CREATURE_CERBERUS),
        TCreaturePattern(CREATURE_DEMON),
        TCreaturePattern(CREATURE_HORNED_DEMON),
        TCreaturePattern(CREATURE_PIT_FIEND),
        TCreaturePattern(CREATURE_PIT_LORD),
        TCreaturePattern(CREATURE_EFREETI),
        TCreaturePattern(CREATURE_EFREET_SULTAN),
        TCreaturePattern(CREATURE_DEVIL),
        TCreaturePattern(CREATURE_ARCH_DEVIL),
        TCreaturePattern(CREATURE_SKELETON),
        TCreaturePattern(CREATURE_SKELETON_WARRIOR),
        TCreaturePattern(CREATURE_WALKING_DEAD),
        TCreaturePattern(CREATURE_ZOMBIE),
        TCreaturePattern(CREATURE_WIGHT),
        TCreaturePattern(CREATURE_WRAITH),
        TCreaturePattern(CREATURE_VAMPIRE),
        TCreaturePattern(CREATURE_VAMPIRE_LORD),
        TCreaturePattern(CREATURE_LICH),
        TCreaturePattern(CREATURE_POWER_LICH),
        TCreaturePattern(CREATURE_BLACK_KNIGHT),
        TCreaturePattern(CREATURE_DREAD_KNIGHT),
        TCreaturePattern(CREATURE_BONE_DRAGON),
        TCreaturePattern(CREATURE_GHOST_DRAGON),
        TCreaturePattern(CREATURE_TROGLODYTE),
        TCreaturePattern(CREATURE_INFERNAL_TROGLODYTE),
        TCreaturePattern(CREATURE_HARPY),
        TCreaturePattern(CREATURE_HARPY_HAG),
        TCreaturePattern(CREATURE_BEHOLDER),
        TCreaturePattern(CREATURE_EVIL_EYE),
        TCreaturePattern(CREATURE_MEDUSA),
        TCreaturePattern(CREATURE_MEDUSA_QUEEN),
        TCreaturePattern(CREATURE_MINOTAUR),
        TCreaturePattern(CREATURE_MINOTAUR_KING),
        TCreaturePattern(CREATURE_MANTICORE),
        TCreaturePattern(CREATURE_SCORPICORE),
        TCreaturePattern(CREATURE_RED_DRAGON),
        TCreaturePattern(CREATURE_BLACK_DRAGON),
        TCreaturePattern(CREATURE_GOBLIN),
        TCreaturePattern(CREATURE_HOBGOBLIN),
        TCreaturePattern(CREATURE_WOLF_RIDER),
        TCreaturePattern(CREATURE_WOLF_RAIDER),
        TCreaturePattern(CREATURE_ORC),
        TCreaturePattern(CREATURE_ORC_CHIEFTAIN),
        TCreaturePattern(CREATURE_OGRE),
        TCreaturePattern(CREATURE_OGRE_MAGE),
        TCreaturePattern(CREATURE_ROC),
        TCreaturePattern(CREATURE_THUNDERBIRD),
        TCreaturePattern(CREATURE_CYCLOPS),
        TCreaturePattern(CREATURE_CYCLOPS_KING),
        TCreaturePattern(CREATURE_BEHEMOTH),
        TCreaturePattern(CREATURE_ANCIENT_BEHEMOTH),
        TCreaturePattern(CREATURE_GNOLL),
        TCreaturePattern(CREATURE_GNOLL_MARAUDER),
        TCreaturePattern(CREATURE_LIZARDMAN),
        TCreaturePattern(CREATURE_LIZARD_WARRIOR),
        TCreaturePattern(CREATURE_GORGON),
        TCreaturePattern(CREATURE_MIGHTY_GORGON),
        TCreaturePattern(CREATURE_SERPENT_FLY),
        TCreaturePattern(CREATURE_DRAGON_FLY),
        TCreaturePattern(CREATURE_BASILISK),
        TCreaturePattern(CREATURE_GREATER_BASILISK),
        TCreaturePattern(CREATURE_WYVERN),
        TCreaturePattern(CREATURE_WYVERN_MONARCH),
        TCreaturePattern(CREATURE_HYDRA),
        TCreaturePattern(CREATURE_CHAOS_HYDRA),
        TCreaturePattern(CREATURE_AIR_ELEMENTAL),
        TCreaturePattern(CREATURE_EARTH_ELEMENTAL),
        TCreaturePattern(CREATURE_FIRE_ELEMENTAL),
        TCreaturePattern(CREATURE_WATER_ELEMENTAL),
        TCreaturePattern(CREATURE_GOLD_GOLEM),
        TCreaturePattern(CREATURE_DIAMOND_GOLEM),
        TCreaturePattern(CREATURE_PIXIE),
        TCreaturePattern(CREATURE_SPRITE),
        TCreaturePattern(CREATURE_PSYCHIC_ELEMENTAL),
        TCreaturePattern(CREATURE_MAGIC_ELEMENTAL),
        TCreaturePattern(CREATURE_ICE_ELEMENTAL),
        TCreaturePattern(CREATURE_MAGMA_ELEMENTAL),
        TCreaturePattern(CREATURE_STORM_ELEMENTAL),
        TCreaturePattern(CREATURE_ENERGY_ELEMENTAL),
        TCreaturePattern(CREATURE_FIREBIRD),
        TCreaturePattern(CREATURE_PHOENIX),
        TCreaturePattern(CREATURE_AZURE_DRAGON),
        TCreaturePattern(CREATURE_CRYSTAL_DRAGON),
        TCreaturePattern(CREATURE_FAERIE_DRAGON),
        TCreaturePattern(CREATURE_RUST_DRAGON),
        TCreaturePattern(CREATURE_ENCHANTER),
        TCreaturePattern(CREATURE_SHARPSHOOTER),
        TCreaturePattern(CREATURE_HALFLING),
        TCreaturePattern(CREATURE_PEASANT),
        TCreaturePattern(CREATURE_BOAR),
        TCreaturePattern(CREATURE_MUMMY),
        TCreaturePattern(CREATURE_NOMAD),
        TCreaturePattern(CREATURE_ROGUE),
        TCreaturePattern(CREATURE_TROLL)
    };
    VA_COMPGEN(0x0041d727, 0x14, STATIC_DTOR, akResourcePatterns)
    DATA(0x0059b0b8) static const TResourcePattern akResourcePatterns[] = {
        TResourcePattern(0),
        TResourcePattern(1),
        TResourcePattern(2),
        TResourcePattern(3),
        TResourcePattern(4),
        TResourcePattern(5),
        TResourcePattern(6)
    };
    VA_COMPGEN(0x0041d713, 0x14, STATIC_DTOR, akTownTypePatterns)
    DATA(0x0059b028) static const TTownTypePattern akTownTypePatterns[] = {
        TTownTypePattern(TOWN_CASTLE),
        TTownTypePattern(TOWN_RAMPART),
        TTownTypePattern(TOWN_TOWER),
        TTownTypePattern(TOWN_INFERNO),
        TTownTypePattern(TOWN_NECROPOLIS),
        TTownTypePattern(TOWN_DUNGEON),
        TTownTypePattern(TOWN_STRONGHOLD),
        TTownTypePattern(TOWN_FORTRESS),
        TTownTypePattern(TOWN_CONFLUX)
    };


    reserve(sizeof(akGeneralSingleTypePatterns) / sizeof(akGeneralSingleTypePatterns[0])
            + sizeof(akGeneralDoubleTypePatterns) / sizeof(akGeneralDoubleTypePatterns[0])
            + sizeof(akArtifactPatterns) / sizeof(akArtifactPatterns[0])
            + sizeof(akCreatureBankPatterns) / sizeof(akCreatureBankPatterns[0])
            + sizeof(akGenerator1Patterns) / sizeof(akGenerator1Patterns[0])
            + sizeof(akGenerator1DoublePatterns) / sizeof(akGenerator1DoublePatterns[0])
            + sizeof(akGenerator4Patterns) / sizeof(akGenerator4Patterns[0])
            + sizeof(akGarrisonPatterns) / sizeof(akGarrisonPatterns[0])
            + sizeof(akHeroClassPatterns) / sizeof(akHeroClassPatterns[0])
            + sizeof(akMinePatterns) / sizeof(akMinePatterns[0])
            + sizeof(akCreaturePatterns) / sizeof(akCreaturePatterns[0])
            + sizeof(akResourcePatterns) / sizeof(akResourcePatterns[0])
            + sizeof(akTownTypePatterns) / sizeof(akTownTypePatterns[0]));
    _add(akGeneralSingleTypePatterns);
    _add(akGeneralDoubleTypePatterns);
    _add(akArtifactPatterns);
    _add(akCreatureBankPatterns);
    _add(akGenerator1Patterns);
    _add(akGenerator1DoublePatterns);
    _add(akGenerator4Patterns);
    _add(akGarrisonPatterns);
    _add(akHeroClassPatterns);
    _add(akMinePatterns);
    _add(akCreaturePatterns);
    _add(akResourcePatterns);
    _add(akTownTypePatterns);
}

// The patterns' implicit destructors fold onto one body.
VA_COMPGEN(0x0041d70c, 0x7, IMPLICIT_DTOR, TTownTypePattern)

static const TPatterns& getPatterns();

VA(0x0041d81d, 0x3b)
int TFindDlg::findType(const TGameObject& object)
{
    const TPatterns& patterns = getPatterns();
    for (unsigned int i = 0; i < patterns.size(); i++) {
        if (patterns[i]->matches(object))
            return i;
    }
    return -1;
}

VA(0x0041d858, 0x2b)
static const TPatterns& getPatterns()
{
    DATA_COMPGEN_GUARD(0x005aa6e0, patternsGuard, patterns)
    VA_COMPGEN(0x0041d883, 0xa, STATIC_DTOR, patterns)
    VA_COMPGEN(0x0041d88d, 0x5, IMPLICIT_DTOR, TPatterns)
    DATA(0x005aa6f0) static const TPatterns patterns;
    return patterns;
}

VA(0x0041d892, 0x19)
bool TFindDlg::matches(const TGameObject& object, int findType)
{
    return getPatterns()[findType]->matches(object);
}

VA(0x0041d8ab, 0x88)
TFindDlg::TFindDlg(CWnd* pParent, int findType)
    : CDialog(TFindDlg::IDD, pParent),
      _m_findType(findType),
      _m_bSearchBackwards(false)
{
}

VA_COMPGEN(0x0041d933, 0x1c, SCALAR_DELETING_DTOR, TFindDlg)
VA_COMPGEN(0x0041d94f, 0x56, IMPLICIT_DTOR, TFindDlg)

VA(0x0041d9a5, 0x40)
void TFindDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_FIND_PREV_BUTTON, _m_findPrevButton);
    DDX_Control(pDX, IDC_FIND_NEXT_BUTTON, _m_findNextButton);
    DDX_Control(pDX, IDC_FIND_WHAT_LIST, _m_findWhatList);
}

VA(0x0041d9e5, 0x6)
BEGIN_MESSAGE_MAP(TFindDlg, CDialog)
    ON_BN_CLICKED(IDC_FIND_NEXT_BUTTON, OnFindNextButton)
    ON_BN_CLICKED(IDC_FIND_PREV_BUTTON, OnFindPrevButton)
    ON_LBN_DBLCLK(IDC_FIND_WHAT_LIST, OnDblClkFindWhatList)
    ON_LBN_SELCHANGE(IDC_FIND_WHAT_LIST, OnSelChangeFindWhatList)
    ON_LBN_SELCANCEL(IDC_FIND_WHAT_LIST, OnSelCancelFindWhatList)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x0041d9eb, 0x25)
int TFindDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kFindCaptionStr);
    return 0;
}

VA(0x0041da10, 0x12c)
BOOL TFindDlg::OnInitDialog()
{
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(IDC_FIND_WHAT_STATIC)->SetWindowText(SFindDlgText::kFindWhatStaticStr);
    GetDlgItem(IDC_FIND_NEXT_BUTTON)->SetWindowText(SFindDlgText::kFindNextButtonStr);
    GetDlgItem(IDC_FIND_PREV_BUTTON)->SetWindowText(SFindDlgText::kFindPrevButtonStr);
    const TPatterns& patterns = getPatterns();
    CDialog::OnInitDialog();
    for (unsigned int i = 0; i < patterns.size(); i++)
        _m_findWhatList.SetItemData(_m_findWhatList.AddString(patterns[i]->getName()), i);
    if (_m_findType != -1) {
        int index = 0;
        while (int(_m_findWhatList.GetItemData(index)) != _m_findType)
            index++;
        _m_findWhatList.SetCurSel(index);
        _m_findWhatList.SetTopIndex(index);
    } else {
        _m_findNextButton.EnableWindow(FALSE);
        _m_findPrevButton.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x0041db3c, 0x39)
void TFindDlg::OnOK()
{
    CDialog::OnOK();
    _m_findType = _m_findWhatList.GetItemData(_m_findWhatList.GetCurSel());
}

VA(0x0041db75, 0xf)
void TFindDlg::OnFindNextButton()
{
    _m_bSearchBackwards = false;
    OnOK();
}

VA(0x0041db84, 0xf)
void TFindDlg::OnFindPrevButton()
{
    _m_bSearchBackwards = true;
    OnOK();
}

void TFindDlg::OnDblClkFindWhatList()
{
    OnOK();
}

VA(0x0041db93, 0x1c)
void TFindDlg::OnSelChangeFindWhatList()
{
    _m_findNextButton.EnableWindow(TRUE);
    _m_findPrevButton.EnableWindow(TRUE);
}

VA(0x0041dbaf, 0x1c)
void TFindDlg::OnSelCancelFindWhatList()
{
    _m_findNextButton.EnableWindow(FALSE);
    _m_findPrevButton.EnableWindow(FALSE);
}
