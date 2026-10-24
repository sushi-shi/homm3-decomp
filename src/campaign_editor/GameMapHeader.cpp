// GameMapHeader.cpp - the campaign editor's map header reader (h3ccmped
// 0x422640..0x424e30, between Error.cpp and GameResource.cpp; file name
// inferred). The scenario map's constructor reads each map's header through
// it. The map editor's reader is GameMap.cpp's; this copy reads its bit sets
// through the campaign editor's RawStream.h, reads the main town's location
// in each branch and fills a missing hero availability with ~TPlayerMask().
#include "campaign_editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "campaign_editor/RawStream.h"
#include "editor/BitsetIterator.h"
#include "editor/GameMapHeader.h"

// A location: three bytes, x 0xff for none.
VA(0x00422670, 0xd3)
TRawIStream& operator>>(TRawIStream& stream, TMapLoc& loc)
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

// Retail reuses each branch's scalar-read slot for its quantity or next town
// level; value reads preserve those short lifetimes through the common helper.
VA(0x00422750, 0x378)
void TGameMapHeader::_readVictoryCondition(TVictoryConditionData& data, TRawIStream& stream, int version)
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
                data.m_accumulateCreature.m_creatureType = TCreatureType(readValue<unsigned char>(stream));
                if (static_cast<unsigned char>(data.m_accumulateCreature.m_creatureType) == 0xff)
                    data.m_accumulateCreature.m_creatureType = TCreatureType(-1);
            }
            data.m_accumulateCreature.m_quantity = readValue<long>(stream);
        }
        break;
    case eVCAccumulateResource:
        {
            data.m_accumulateResource.m_resourceType = TGameResourceType(readValue<signed char>(stream));
            data.m_accumulateResource.m_quantity = readValue<long>(stream);
        }
        break;
    case eVCUpgradeTown:
        {
            stream >> data.m_upgradeTown.m_townLoc;
            data.m_upgradeTown.m_hallLevel = readValue<signed char>(stream);
            data.m_upgradeTown.m_castleLevel = readValue<signed char>(stream);
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

// The type read's temporary ends before the switch; retail reuses its slot
// for the signed-short day count.
VA(0x00422ad0, 0x98)
void TGameMapHeader::_readLossCondition(TLossConditionData& data, TRawIStream& stream, int version)
{
    data.m_type = readValue<signed char>(stream);
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

// int/long count probes changed only folded _read call identities; the
// container counts retain their unsigned size domain. The first hero ID is
// compared as a byte and widened for its field; container keys bind as
// int without unsigned-to-int reference temporaries.
// Bind each hero identity before extracting its ID and retain that receiver
// through the name read, as in the native record loop.
VA(0x00422b70, 0xd4a)
TGameMapHeader::TGameMapHeader(TRawIStream& stream, int version)
{
    if (version >= 10) {
        m_bPlayable = readValue<signed char>(stream) != 0;
    } else {
        m_bPlayable = true;
    }
    m_dimension = readValue<long>(stream);
    m_bTwoLayer = readValue<signed char>(stream) != 0;
    stream >> m_name >> m_desc;
    m_difficulty = readValue<signed char>(stream);
    if (version >= 20) {
        m_maxHeroLevel = readValue<unsigned char>(stream);
    } else {
        m_maxHeroLevel = 0;
    }
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        TPlayerSlot& slot = m_aPlayer[player];
        slot.m_bHumanPlayable = readValue<signed char>(stream) != 0;
        slot.m_bComputerPlayable = readValue<signed char>(stream) != 0;
        slot.m_behaviorType = TPlayerInfo::TBehaviorType(readValue<signed char>(stream));
        if (version >= 8) {
            if (version >= 26) {
                slot.m_bCustomTownTypes = readValue<signed char>(stream) != 0;
            }
            if (version >= 16) {
                readBitset(stream, &slot.m_townTypes);
            } else {
                std::bitset<8> townTypes;
                readBitset(stream, &townTypes);
                std::copy(TBitsetIterator<8>(townTypes, 0), TBitsetIterator<8>(townTypes, 8),
                          TBitsetIterator<kNumTownTypes>(slot.m_townTypes, 0));
            }
            slot.m_bRandomTown = readValue<signed char>(stream) != 0;
        }
        if (version >= 9) {
            slot.m_bHasMainTown = readValue<signed char>(stream) != 0;
            if (slot.m_bHasMainTown) {
                if (version >= 17) {
                    slot.m_bGenerateHero = readValue<signed char>(stream) != 0;
                    slot.m_mainTownType = readValue<signed char>(stream);
                    stream >> slot.m_mainTownLoc;
                } else {
                    slot.m_bGenerateHero = true;
                    slot.m_mainTownType = -1;
                    stream >> slot.m_mainTownLoc;
                }
            } else {
                slot.m_bGenerateHero = false;
            }
        }
        if (version >= 13) {
            slot.m_bRandomHero = readValue<signed char>(stream) != 0;
            unsigned char heroID = readValue<unsigned char>(stream);
            slot.m_heroID = heroID;
            if (heroID != 0xff) {
                slot.m_heroPortrait = readValue<unsigned char>(stream);
                stream >> slot.m_heroName;
            }
        }
        if (version >= 17) {
            slot.m_numPlaceholders = readValue<signed char>(stream);
        } else {
            slot.m_numPlaceholders = 0;
        }
        if (version >= 18) {
            unsigned int numHeroes = readValue<unsigned int>(stream);
            slot.m_heroes.resize(numHeroes, THeroIdentity());
            for (unsigned int i = 0; i < numHeroes; i++) {
                THeroIdentity& identity = slot.m_heroes[i];
                identity.m_heroID = readValue<unsigned char>(stream);
                stream >> identity.m_name;
            }
        }
    }
    if (version < 9) {
        char unused;
        stream.read(&unused, sizeof(unused));
    }
    _readVictoryCondition(m_vcData, stream, version);
    _readLossCondition(m_lcData, stream, version);
    m_numTeams = readValue<signed char>(stream);
    if (m_numTeams > 0) {
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            m_aTeam[player] = readValue<signed char>(stream);
        }
    } else {
        std::fill_n(m_aTeam, static_cast<unsigned int>(kNumPlayers), 0);
    }
    if (version >= 16) {
        readBitset(stream, &m_availableHeroes);
    } else if (version >= 8) {
        std::bitset<128> availableHeroes;
        readBitset(stream, &availableHeroes);
        std::copy(TBitsetIterator<128>(availableHeroes, 0), TBitsetIterator<128>(availableHeroes, 128),
                  TBitsetIterator<kNumHeroes>(m_availableHeroes, 0));
    }
    if (version >= 17) {
        unsigned int numPlaceholderHeroes = readValue<unsigned int>(stream);
        m_placeholderHeroIDs.reserve(numPlaceholderHeroes);
        for (unsigned int i = 0; i < numPlaceholderHeroes; i++) {
            int heroID = readValue<unsigned char>(stream);
            m_placeholderHeroIDs.push_back(heroID);
        }
    }
    if (version >= 25) {
        unsigned int numCustomHeroes = readValue<unsigned char>(stream);
        for (unsigned int i = 0; i < numCustomHeroes; i++) {
            std::string name;
            TPlayerMask availability;
            int heroID = readValue<unsigned char>(stream);
            int portraitID = readValue<unsigned char>(stream);
            if (portraitID == 0xff)
                portraitID = -1;
            stream >> name;
            if (version >= 26)
                readBitset(stream, &availability);
            else
                availability = ~TPlayerMask();
            m_customHeroes.insert(std::map<int, TCustomHero>::value_type(
                heroID, TCustomHero(portraitID, name, availability)));
        }
    }
}

// reserve checks this four-byte-element capacity: all 19 retail bytes agree,
// with no relocation operands. The rest of the emitted COMDAT is NOP padding.
VA_COMPGEN(0x00423e40, 0x13, VECTOR_CAPACITY, int)

// The placeholder-hero push_back retains the single-element insert. Retail
// returns its int iterator, takes two arguments and calls _Construct<int>.
VA_COMPGEN(0x00423e60, 0x19c, VECTOR_INSERT_SINGLE, int)

// Its fill branch calls this int construction loop: four-byte stride,
// a placement-new null guard and three stack arguments (ret 12).
VA_COMPGEN(0x00424580, 0x25, VECTOR_UFILL, int)
