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

#include "va.h"
#include "armygrp.h"
#include "artifact.h"
#include "gameversion.h"
#include "town_type.h"
#include "editor/BitsetIterator.h"
#include "editor/GameMap.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/Player.h"
#include "editor/RawStream.h"
#include "editor/VictoryCondition.h"

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
        TCustomHero(int portrait, const std::string& name, TPlayerMask availability)
            : m_portrait(portrait), m_name(name), m_availability(availability) {}

        int m_portrait;
        std::string m_name;
        TPlayerMask m_availability;
    };

    TGameMapHeader() {}
    // The header as a map file of the format version has it: older formats
    // lack fields, which take their defaults.
    TGameMapHeader(TRawIStream& stream, int version);

    // The header in the edition's format.
    void write(TRawOStream& stream, EGameVersion version) const;

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

private:
    // The condition records (out of line after the map's templates, 0x43ad49
    // and 0x43aec6 read, 0x43af11 and 0x43b0b3 write).
    static void _readVictoryCondition(TVictoryConditionData& data, TRawIStream& stream, int version);
    static void _readLossCondition(TLossConditionData& data, TRawIStream& stream, int version);
    static void _writeVictoryCondition(const TVictoryConditionData& data, TRawOStream& stream,
                                       EGameVersion version);
    static void _writeLossCondition(const TLossConditionData& data, TRawOStream& stream, EGameVersion version);
};

// A location: three bytes, x 0xff for none (0x43acf0 reads, 0x43b070 writes).
VA(0x0043acf0, 0x59)
inline TRawIStream& operator>>(TRawIStream& stream, TMapLoc& loc)
{
    unsigned char value;
    stream >> value;
    loc.m_x = value;
    stream >> value;
    loc.m_y = value;
    stream >> value;
    loc.m_layer = value;
    if (loc.m_x == 0xff) {
        loc.m_x = -1;
        loc.m_y = -1;
        loc.m_layer = -1;
    }
    return stream;
}

VA(0x0043ad49, 0x17d)
inline void TGameMapHeader::_readVictoryCondition(TVictoryConditionData& data, TRawIStream& stream, int version)
{
    signed char value;
    stream >> value;
    data.m_type = value;
    if (data.m_type == eVCNone)
        return;
    stream >> value;
    data.m_bAllowNormalVictory = value != 0;
    stream >> value;
    data.m_bAppliesToComputer = value != 0;
    switch (data.m_type) {
    case eVCAquireArtifact:
        if (version >= 21) {
            short artifact;
            stream >> artifact;
            data.m_aquireArtifact.m_artifact = TArtifact(artifact);
        } else {
            signed char artifact;
            stream >> artifact;
            data.m_aquireArtifact.m_artifact = TArtifact(artifact);
        }
        break;
    case eVCAccumulateCreature:
        {
            if (version >= 20) {
                short creatureType;
                stream >> creatureType;
                data.m_accumulateCreature.m_creatureType = TCreatureType(creatureType);
            } else {
                unsigned char creatureType;
                stream >> creatureType;
                data.m_accumulateCreature.m_creatureType = TCreatureType(creatureType);
                if (creatureType == 0xff)
                    data.m_accumulateCreature.m_creatureType = TCreatureType(-1);
            }
            long quantity;
            stream >> quantity;
            data.m_accumulateCreature.m_quantity = quantity;
        }
        break;
    case eVCAccumulateResource:
        {
            signed char resourceType;
            stream >> resourceType;
            data.m_accumulateResource.m_resourceType = TGameResourceType(resourceType);
            long quantity;
            stream >> quantity;
            data.m_accumulateResource.m_quantity = quantity;
        }
        break;
    case eVCUpgradeTown:
        {
            stream >> data.m_upgradeTown.m_townLoc;
            signed char hallLevel;
            stream >> hallLevel;
            data.m_upgradeTown.m_hallLevel = hallLevel;
            signed char castleLevel;
            stream >> castleLevel;
            data.m_upgradeTown.m_castleLevel = castleLevel;
        }
        break;
    case eVCBuildHolyGrailStruct:
        stream >> data.m_buildHolyGrailStruct.m_townLoc;
        break;
    case eVCDefeatHero:
        stream >> data.m_defeatHero.m_heroLoc;
        break;
    case eVCCaptureTown:
        stream >> data.m_captureTown.m_townLoc;
        break;
    case eVCDefeatMonster:
        stream >> data.m_defeatMonster.m_monsterLoc;
        break;
    case eVCTransportArtifact:
        {
            unsigned char artifact;
            stream >> artifact;
            data.m_transportArtifact.m_artifact = TArtifact(artifact);
            stream >> data.m_transportArtifact.m_townLoc;
        }
        break;
    }
}

VA(0x0043aec6, 0x4b)
inline void TGameMapHeader::_readLossCondition(TLossConditionData& data, TRawIStream& stream, int version)
{
    signed char type;
    stream >> type;
    data.m_type = type;
    switch (data.m_type) {
    case eLCLoseTown:
        stream >> data.m_loseTown.m_townLoc;
        break;
    case eLCLoseHero:
        stream >> data.m_loseHero.m_heroLoc;
        break;
    case eLCTimeExpires:
        {
            short numDays;
            stream >> numDays;
            data.m_timeExpires.m_numDays = numDays;
        }
        break;
    }
}

VA(0x0043af11, 0x15f)
inline void TGameMapHeader::_writeVictoryCondition(const TVictoryConditionData& data, TRawOStream& stream,
                                                   EGameVersion version)
{
    stream << static_cast<signed char>(data.m_type);
    if (data.m_type == eVCNone)
        return;
    stream << static_cast<signed char>(data.m_bAllowNormalVictory);
    stream << static_cast<signed char>(data.m_bAppliesToComputer);
    switch (data.m_type) {
    case eVCAquireArtifact:
        if (version >= GAME_VERSION_AB) {
            short artifact = data.m_aquireArtifact.m_artifact;
            stream << artifact;
        } else {
            signed char artifact = data.m_aquireArtifact.m_artifact;
            stream << artifact;
        }
        break;
    case eVCAccumulateCreature:
        {
            if (version >= GAME_VERSION_AB) {
                short creatureType = data.m_accumulateCreature.m_creatureType;
                stream << creatureType;
            } else {
                unsigned char creatureType = data.m_accumulateCreature.m_creatureType;
                stream << creatureType;
            }
            long quantity = data.m_accumulateCreature.m_quantity;
            stream << quantity;
        }
        break;
    case eVCAccumulateResource:
        {
            signed char resourceType = data.m_accumulateResource.m_resourceType;
            stream << resourceType;
            long quantity = data.m_accumulateResource.m_quantity;
            stream << quantity;
        }
        break;
    case eVCUpgradeTown:
        {
            stream << data.m_upgradeTown.m_townLoc;
            signed char level = data.m_upgradeTown.m_hallLevel;
            stream << level;
            level = data.m_upgradeTown.m_castleLevel;
            stream << level;
        }
        break;
    case eVCBuildHolyGrailStruct:
        stream << data.m_buildHolyGrailStruct.m_townLoc;
        break;
    case eVCDefeatHero:
        stream << data.m_defeatHero.m_heroLoc;
        break;
    case eVCCaptureTown:
        stream << data.m_captureTown.m_townLoc;
        break;
    case eVCDefeatMonster:
        stream << data.m_defeatMonster.m_monsterLoc;
        break;
    case eVCTransportArtifact:
        {
            unsigned char artifact = data.m_transportArtifact.m_artifact;
            stream << artifact;
            stream << data.m_transportArtifact.m_townLoc;
        }
        break;
    }
}

VA(0x0043b070, 0x43)
inline TRawOStream& operator<<(TRawOStream& stream, const TMapLoc& loc)
{
    stream << static_cast<unsigned char>(loc.m_x);
    stream << static_cast<unsigned char>(loc.m_y);
    stream << static_cast<unsigned char>(loc.m_layer);
    return stream;
}

VA(0x0043b0b3, 0x4c)
inline void TGameMapHeader::_writeLossCondition(const TLossConditionData& data, TRawOStream& stream,
                                                EGameVersion version)
{
    stream << static_cast<signed char>(data.m_type);
    switch (data.m_type) {
    case eLCLoseTown:
        stream << data.m_loseTown.m_townLoc;
        break;
    case eLCLoseHero:
        stream << data.m_loseHero.m_heroLoc;
        break;
    case eLCTimeExpires:
        stream << static_cast<short>(data.m_timeExpires.m_numDays);
        break;
    }
}

VA(0x0043b0ff, 0x5c4)
inline TGameMapHeader::TGameMapHeader(TRawIStream& stream, int version)
{
    if (version >= 10) {
        signed char bPlayable;
        stream >> bPlayable;
        m_bPlayable = bPlayable != 0;
    } else {
        m_bPlayable = true;
    }
    long dimension;
    stream >> dimension;
    m_dimension = dimension;
    signed char bTwoLayer;
    stream >> bTwoLayer;
    m_bTwoLayer = bTwoLayer != 0;
    stream >> m_name >> m_desc;
    signed char difficulty;
    stream >> difficulty;
    m_difficulty = difficulty;
    if (version >= 20) {
        unsigned char maxHeroLevel;
        stream >> maxHeroLevel;
        m_maxHeroLevel = maxHeroLevel;
    } else {
        m_maxHeroLevel = 0;
    }
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        TPlayerSlot& slot = m_aPlayer[player];
        signed char bHumanPlayable;
        stream >> bHumanPlayable;
        slot.m_bHumanPlayable = bHumanPlayable != 0;
        signed char bComputerPlayable;
        stream >> bComputerPlayable;
        slot.m_bComputerPlayable = bComputerPlayable != 0;
        signed char behaviorType;
        stream >> behaviorType;
        slot.m_behaviorType = TPlayerInfo::TBehaviorType(behaviorType);
        if (version >= 8) {
            if (version >= 26) {
                signed char bCustomTownTypes;
                stream >> bCustomTownTypes;
                slot.m_bCustomTownTypes = bCustomTownTypes != 0;
            }
            if (version >= 16) {
                readBitset(stream, slot.m_townTypes);
            } else {
                std::bitset<8> townTypes;
                readBitset(stream, townTypes);
                std::copy(TBitsetIterator<8>(townTypes, 0), TBitsetIterator<8>(townTypes, 8),
                          TBitsetIterator<kNumTownTypes>(slot.m_townTypes, 0));
            }
            signed char bRandomTown;
            stream >> bRandomTown;
            slot.m_bRandomTown = bRandomTown != 0;
        }
        if (version >= 9) {
            signed char bHasMainTown;
            stream >> bHasMainTown;
            slot.m_bHasMainTown = bHasMainTown != 0;
            if (slot.m_bHasMainTown) {
                if (version >= 17) {
                    signed char bGenerateHero;
                    stream >> bGenerateHero;
                    slot.m_bGenerateHero = bGenerateHero != 0;
                    signed char mainTownType;
                    stream >> mainTownType;
                    slot.m_mainTownType = mainTownType;
                } else {
                    slot.m_mainTownType = -1;
                    slot.m_bGenerateHero = true;
                }
                stream >> slot.m_mainTownLoc;
            } else {
                slot.m_bGenerateHero = false;
            }
        }
        if (version >= 13) {
            signed char bRandomHero;
            stream >> bRandomHero;
            slot.m_bRandomHero = bRandomHero != 0;
            unsigned char heroID;
            stream >> heroID;
            slot.m_heroID = heroID;
            if (heroID != 0xff) {
                unsigned char heroPortrait;
                stream >> heroPortrait;
                slot.m_heroPortrait = heroPortrait;
                stream >> slot.m_heroName;
            }
        }
        if (version >= 17) {
            signed char numPlaceholders;
            stream >> numPlaceholders;
            slot.m_numPlaceholders = numPlaceholders;
        } else {
            slot.m_numPlaceholders = 0;
        }
        if (version >= 18) {
            unsigned int numHeroes;
            stream >> numHeroes;
            slot.m_heroes.resize(numHeroes, THeroIdentity());
            for (unsigned int i = 0; i < numHeroes; i++) {
                unsigned char heroID;
                stream >> heroID;
                slot.m_heroes[i].m_heroID = heroID;
                stream >> slot.m_heroes[i].m_name;
            }
        }
    }
    if (version < 9) {
        signed char unused;
        stream >> unused;
    }
    _readVictoryCondition(m_vcData, stream, version);
    _readLossCondition(m_lcData, stream, version);
    signed char numTeams;
    stream >> numTeams;
    m_numTeams = numTeams;
    if (m_numTeams > 0) {
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            signed char team;
            stream >> team;
            m_aTeam[player] = team;
        }
    } else {
        std::fill_n(m_aTeam, static_cast<unsigned int>(kNumPlayers), 0);
    }
    if (version >= 16) {
        readBitset(stream, m_availableHeroes);
    } else if (version >= 8) {
        std::bitset<128> availableHeroes;
        readBitset(stream, availableHeroes);
        std::copy(TBitsetIterator<128>(availableHeroes, 0), TBitsetIterator<128>(availableHeroes, 128),
                  TBitsetIterator<kNumHeroes>(m_availableHeroes, 0));
    }
    if (version >= 17) {
        unsigned int numPlaceholderHeroes;
        stream >> numPlaceholderHeroes;
        m_placeholderHeroIDs.reserve(numPlaceholderHeroes);
        for (unsigned int i = 0; i < numPlaceholderHeroes; i++) {
            unsigned char heroID;
            stream >> heroID;
            m_placeholderHeroIDs.push_back(heroID);
        }
    }
    if (version >= 25) {
        unsigned char numCustomHeroes;
        stream >> numCustomHeroes;
        for (unsigned int i = 0; i < numCustomHeroes; i++) {
            std::string name;
            TPlayerMask availability;
            unsigned char heroID;
            stream >> heroID;
            unsigned char portrait;
            stream >> portrait;
            int portraitID = portrait;
            if (portraitID == 0xff)
                portraitID = -1;
            stream >> name;
            if (version >= 26)
                readBitset(stream, availability);
            else
                availability = 0xff;
            m_customHeroes.insert(std::map<int, TCustomHero>::value_type(
                heroID, TCustomHero(portraitID, name, availability)));
        }
    }
}

VA(0x0043b6c8, 0x3ee)
inline void TGameMapHeader::write(TRawOStream& stream, EGameVersion version) const
{
    stream << static_cast<signed char>(m_bPlayable);
    stream << static_cast<long>(m_dimension);
    stream << static_cast<signed char>(m_bTwoLayer);
    stream << m_name << m_desc;
    stream << static_cast<signed char>(m_difficulty);
    if (version >= GAME_VERSION_AB)
        stream << static_cast<unsigned char>(m_maxHeroLevel);
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        const TPlayerSlot& slot = m_aPlayer[player];
        stream << static_cast<signed char>(slot.m_bHumanPlayable);
        stream << static_cast<signed char>(slot.m_bComputerPlayable);
        stream << static_cast<signed char>(slot.m_behaviorType);
        if (version >= GAME_VERSION_SOD)
            stream << static_cast<signed char>(slot.m_bCustomTownTypes);
        if (version >= GAME_VERSION_AB) {
            writeBitset(stream, slot.m_townTypes);
        } else {
            std::bitset<8> townTypes;
            std::copy(TBitsetIterator<kNumTownTypes>(slot.m_townTypes, 0),
                      TBitsetIterator<kNumTownTypes>(slot.m_townTypes, 8), TBitsetIterator<8>(townTypes, 0));
            writeBitset(stream, townTypes);
        }
        stream << static_cast<signed char>(slot.m_bRandomTown);
        if (version >= GAME_VERSION_AB) {
            stream << static_cast<signed char>(slot.m_bHasMainTown);
            if (slot.m_bHasMainTown) {
                stream << static_cast<signed char>(slot.m_bGenerateHero);
                stream << static_cast<signed char>(slot.m_mainTownType);
                stream << slot.m_mainTownLoc;
            }
        } else {
            stream << static_cast<signed char>(slot.m_bHasMainTown);
            if (slot.m_bHasMainTown)
                stream << slot.m_mainTownLoc;
        }
        stream << static_cast<signed char>(slot.m_bRandomHero);
        stream << static_cast<unsigned char>(slot.m_heroID);
        if (static_cast<unsigned char>(slot.m_heroID) != 0xff) {
            stream << static_cast<signed char>(slot.m_heroPortrait);
            stream << slot.m_heroName;
        }
        if (version >= GAME_VERSION_AB) {
            stream << static_cast<signed char>(slot.m_numPlaceholders);
            stream << static_cast<long>(slot.m_heroes.size());
            for (std::vector<THeroIdentity>::const_iterator pHero = slot.m_heroes.begin();
                 pHero != slot.m_heroes.end(); ++pHero) {
                stream << static_cast<unsigned char>(pHero->m_heroID);
                stream << pHero->m_name;
            }
        }
    }
    _writeVictoryCondition(m_vcData, stream, version);
    _writeLossCondition(m_lcData, stream, version);
    stream << static_cast<signed char>(m_numTeams);
    if (m_numTeams > 0) {
        for (unsigned int player = 0; player < kNumPlayers; player++)
            stream << static_cast<signed char>(m_aTeam[player]);
    }
    if (version >= GAME_VERSION_AB) {
        writeBitset(stream, m_availableHeroes);
    } else {
        std::bitset<128> availableHeroes;
        std::copy(TBitsetIterator<kNumHeroes>(m_availableHeroes, 0),
                  TBitsetIterator<kNumHeroes>(m_availableHeroes, 128), TBitsetIterator<128>(availableHeroes, 0));
        writeBitset(stream, availableHeroes);
    }
    if (version >= GAME_VERSION_AB) {
        stream << static_cast<long>(m_placeholderHeroIDs.size());
        for (std::vector<int>::const_iterator pHeroID = m_placeholderHeroIDs.begin();
             pHeroID != m_placeholderHeroIDs.end(); ++pHeroID)
            stream << static_cast<unsigned char>(*pHeroID);
    }
    if (version >= GAME_VERSION_SOD) {
        stream << static_cast<unsigned char>(m_customHeroes.size());
        for (std::map<int, TCustomHero>::const_iterator pHero = m_customHeroes.begin(); pHero != m_customHeroes.end();
             ++pHero) {
            stream << static_cast<unsigned char>(pHero->first);
            stream << static_cast<unsigned char>(pHero->second.m_portrait);
            stream << pHero->second.m_name;
            writeBitset(stream, pHero->second.m_availability);
        }
    }
}

#endif  /* HOMM3_EDITOR_GAMEMAPHEADER_H */
