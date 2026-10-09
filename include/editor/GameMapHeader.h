// GameMapHeader.h - the map file's header: what the scenario list shows and
// the map reads before its cells and objects (h3maped's reader 0x43b0ff
// and writer 0x43b6c8 follow GameMap.cpp's template instances; the map
// reads one, 0x41f350, and writes one, 0x423b20).
//
// The layout follows the reader and the implicit constructors and
// destructors the map emits (0x424750, 0x41fe38; a player slot's 0x4247f1
// and 0x41fea9; a custom hero's 0x424730).
#ifndef HOMM3_EDITOR_GAMEMAPHEADER_H
#define HOMM3_EDITOR_GAMEMAPHEADER_H

#include <bitset>
#include <map>
#include <string>
#include <vector>

#include "armygrp.h"
#include "artifact.h"
#include "town_type.h"
#include "editor/GameMap.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/Player.h"

class TRawIStream;
class TRawOStream;

// An object's location as the map's condition records keep it: -1s for
// no object (h3maped 0x42921c fills it).
struct TMapLoc {
    int m_x;
    int m_y;
    int m_layer;
};

// A victory condition as the map keeps it (h3maped's visitors write the
// kind's ordinal as a dword, the two flags at +4/+5, the kind's data at +8).
struct TVictoryConditionData {
    int m_type;
    bool m_bAllowNormalVictory;
    bool m_bAppliesToComputer;
    union {
        struct {
            TArtifact m_artifact;
        } m_aquireArtifact;
        struct {
            TCreatureType m_creatureType;
            unsigned int m_quantity;
        } m_accumulateCreature;
        struct {
            TGameResourceType m_resourceType;
            unsigned int m_quantity;
        } m_accumulateResource;
        struct {
            TMapLoc m_townLoc;
            int m_hallLevel;
            int m_castleLevel;
        } m_upgradeTown;
        struct {
            TMapLoc m_townLoc;
        } m_buildHolyGrailStruct;
        struct {
            TMapLoc m_heroLoc;
        } m_defeatHero;
        struct {
            TMapLoc m_townLoc;
        } m_captureTown;
        struct {
            TMapLoc m_monsterLoc;
        } m_defeatMonster;
        struct {
            TArtifact m_artifact;
            TMapLoc m_townLoc;
        } m_transportArtifact;
    };
};

// A loss condition as the map keeps it.
struct TLossConditionData {
    int m_type;
    union {
        struct {
            TMapLoc m_townLoc;
        } m_loseTown;
        struct {
            TMapLoc m_heroLoc;
        } m_loseHero;
        struct {
            unsigned int m_numDays;
        } m_timeExpires;
    };
};

// The header proper.
class TGameMapHeader {
public:
    // A hero a player starts with, by id and name.
    struct THeroIdentity {
        int m_heroID;
        std::string m_name;
    };

    // One player's slot (0x54 bytes).
    struct TPlayerSlot {
        bool m_bHumanPlayable;
        bool m_bComputerPlayable;
        TPlayerInfo::TBehaviorType m_behaviorType;
        bool m_bCustomTownTypes;
        std::bitset<kNumTownTypes> m_townTypes;
        bool m_bRandomTown;
        bool m_bHasMainTown;
        bool m_bGenerateHero;
        int m_mainTownType;
        TMapLoc m_mainTownLoc;
        bool m_bRandomHero;
        int m_heroID;
        int m_heroPortrait;
        std::string m_heroName;
        int m_numPlaceholders;
        std::vector<THeroIdentity> m_heroes;
    };

    // A hero the map customizes beyond its prototype: its portrait (none:
    // -1), its name and who may hire it.
    struct TCustomHero {
        TCustomHero() : m_portrait(-1) {}

        int m_portrait;
        std::string m_name;
        TPlayerMask m_availability;
    };

    TGameMapHeader() {}
    TGameMapHeader(TRawIStream& stream, int version);

    void write(TRawOStream& stream, int version) const;

    bool m_bPlayable;
    unsigned int m_dimension;
    bool m_bTwoLayer;
    std::string m_name;
    std::string m_desc;
    int m_difficulty;
    unsigned int m_maxHeroLevel;
    TPlayerSlot m_aPlayer[kNumPlayers];
    TVictoryConditionData m_vcData;
    TLossConditionData m_lcData;
    unsigned int m_numTeams;
    int m_aTeam[kNumPlayers];
    std::bitset<kNumHeroes> m_availableHeroes;
    std::vector<int> m_placeholderHeroIDs;
    std::map<int, TCustomHero> m_customHeroes;
};

#endif  /* HOMM3_EDITOR_GAMEMAPHEADER_H */
