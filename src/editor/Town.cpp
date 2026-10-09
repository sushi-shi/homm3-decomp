// Town.cpp - the towns on the map (h3maped 0x4c16f0..0x4c36a6; Loki
// h3maped object 35): the town's timed events and generator bonuses, and
// the town with its binary form and map text. The Windows town is
// linkable (from map version 15), may name obligatory spells (Armageddon's
// Blade on) and keeps Shadow of Death's alignment (map version 28); its
// type traits moved to TownTypeTraits.cpp. The release drops Loki's
// asserts.
#include "editor/stdafx.h"

#include <ctype.h>
#include <stdio.h>
#include <algorithm>
#include <functional>

#include "va.h"
#include "adventureobjecttype.h"
#include "armygrp.h"
#include "spelldefs.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"
#include "editor/Town.h"

// The reserved byte counts of the map format records this file reads and
// writes.
const unsigned int kNumTownReserved = 3;
const unsigned int kNumTownEventReserved = 4;

unsigned int TTown::TGeneratorBonuses::get(TGeneratorType type) const
{
    return _m_bonuses[type];
}

void TTown::TGeneratorBonuses::set(TGeneratorType type, unsigned int newBonus)
{
    _m_bonuses[type] = newBonus;
}

TRawOStream& operator<<(TRawOStream& stream, const TTown::TGeneratorBonuses& bonuses)
{
    for (unsigned int type = 0; type < TTown::s_kNumGeneratorTypes; type++)
        stream << static_cast<short>(bonuses.get(TTown::TGeneratorType(type)));
    return stream;
}

VA(0x004c190e, 0x2a)
TRawIStream& operator>>(TRawIStream& stream, TTown::TGeneratorBonuses& bonuses)
{
    for (unsigned int type = 0; type < TTown::s_kNumGeneratorTypes; type++) {
        short bonus;
        stream >> bonus;
        bonuses.set(TTown::TGeneratorType(type), bonus);
    }
    return stream;
}

VA(0x004c1938, 0xa8)
void TTown::TTimedEvent::read(TRawIStream* pIStream, int version)
{
    static_cast< ::TTimedEvent*>(this)->read(pIStream, version);
    std::bitset<s_kNumBuildings> buildMask;
    readBitset(*pIStream, buildMask);
    setBuildMask(buildMask);
    TGeneratorBonuses generatorBonuses;
    *pIStream >> generatorBonuses;
    setGeneratorBonuses(generatorBonuses);
    signed char aReserved[kNumTownEventReserved];
    *pIStream >> aReserved;
}

VA(0x004c19e0, 0xa0)
void TTown::TTimedEvent::write(TRawOStream* pOStream, int version) const
{
    static_cast<const ::TTimedEvent*>(this)->write(pOStream, version);
    unsigned char aBuildMask[(s_kNumBuildings + 7) / 8];
    fill_n(aBuildMask, sizeof(aBuildMask), 0);
    for (unsigned int building = 0; building < s_kNumBuildings; building++)
        if (_m_buildMask.test(building))
            aBuildMask[building / 8] |= 1 << building % 8;
    *pOStream << aBuildMask << _m_generatorBonuses;
    signed char aReserved[kNumTownEventReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

DATA(0x005a50d0) const TTown::TTypeTraits* TTown::s_akTypeTraits = akTownTypeTraits;

VA(0x004c1a8b, 0x5)
void TTown::initialize()
{
    InitializeTownTypeTraitsTable();
}

VA(0x004c1a90, 0x22b)
TTown::TTown(const TTown& other)
    : TGameObject(other), TLinkableObject(other), TPlayableObject(other), _m_bCustomName(other._m_bCustomName),
      _m_bCustomGarrison(other._m_bCustomGarrison), _m_bCustomBuildings(other._m_bCustomBuildings),
      _m_bGroupedFormation(other._m_bGroupedFormation), _m_name(other._m_name), _m_garrison(other._m_garrison),
      _m_aBuildingState(other._m_aBuildingState), _m_obligatorySpellsMask(other._m_obligatorySpellsMask),
      _m_disabledSpellsMask(other._m_disabledSpellsMask), _m_events(other._m_events), _m_pVisitingHero(NULL),
      _m_alignment(other._m_alignment)
{
    if (other._m_pVisitingHero != NULL) {
        std::auto_ptr<TGameObject> pClone = other._m_pVisitingHero->clone();
        if (pClone.get() == NULL)
            throw TAllocationFailure();
        _m_pVisitingHero = dynamic_cast<THero*>(pClone.release());
    }
}

VA(0x004c1cbb, 0x10a)
TTown::TTown(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TLinkableObject(objType), TPlayableObject(objType, owner), _m_bCustomName(false),
      _m_bCustomGarrison(false), _m_bCustomBuildings(false), _m_bGroupedFormation(false), _m_pVisitingHero(NULL),
      _m_alignment(ePlayerNone)
{
    _m_aBuildingState[eBuildingFort].setBBuilt(true);
}

VA(0x004c1dc5, 0x4fc)
TTown::TTown(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TLinkableObject(objType), TPlayableObject(objType), _m_pVisitingHero(NULL)
{
    if (version >= 15)
        TLinkableObject::read(pIStream, version);
    TPlayableObject::read(pIStream, version);
    signed char bCustom;
    *pIStream >> bCustom;
    setBCustomName(bCustom != 0);
    if (_m_bCustomName) {
        std::string name;
        *pIStream >> name;
        if (name.size() > s_kMaxNameLen)
            name.erase(s_kMaxNameLen);
        setName(name);
    }
    if (version >= 7)
        *pIStream >> bCustom;
    else
        bCustom = 1;
    setBCustomGarrison(bCustom != 0);
    if (_m_bCustomGarrison)
        setGarrison(TArmy(pIStream, version));
    signed char bGroupedFormation;
    *pIStream >> bGroupedFormation;
    setBGroupedFormation(bGroupedFormation != 0);
    if (version >= 7)
        *pIStream >> bCustom;
    else
        bCustom = 1;
    setBCustomBuildings(bCustom != 0);
    if (_m_bCustomBuildings) {
        unsigned char aBuilt[(s_kNumBuildings + 7) / 8];
        unsigned char aDisabled[(s_kNumBuildings + 7) / 8];
        *pIStream >> aBuilt >> aDisabled;
        TArray<TBuildingState, s_kNumBuildings> aBuildingState;
        for (unsigned int building = 0; building < s_kNumBuildings; building++) {
            unsigned int index = building / 8;
            unsigned char mask = 1 << building % 8;
            aBuildingState[building].setBBuilt((aBuilt[index] & mask) != 0);
            aBuildingState[building].setBDisabled((aDisabled[index] & mask) != 0);
        }
        setBuildingStates(aBuildingState);
    } else {
        signed char bFortBuilt;
        *pIStream >> bFortBuilt;
        TArray<TBuildingState, s_kNumBuildings> aBuildingState;
        aBuildingState[eBuildingFort].setBBuilt(bFortBuilt != 0);
        setBuildingStates(aBuildingState);
    }
    TTownType type = getTownType();
    const TTypeTraits& typeTraits = getTownTypeTraits();
    std::bitset<kNumSpells> spellsMask;
    if (version >= 15) {
        readBitset(*pIStream, spellsMask);
        for (int spell = 0; spell < kNumSpells; spell++) {
            if (spellsMask[spell]
                && ((akSpellTraits[spell].m_flags & 0x2000) || akSpellTraits[spell].m_school == 0
                    || !typeTraits.hasMageGuildLevel(akSpellTraits[spell].m_level - 1)))
                spellsMask[spell] = false;
        }
        setObligatorySpellsMask(spellsMask);
    }
    readBitset(*pIStream, spellsMask);
    for (int spell = 0; spell < kNumSpells; spell++) {
        if (spellsMask[spell]
            && ((akSpellTraits[spell].m_flags & 0x2000) || akSpellTraits[spell].m_school == 0
                || !typeTraits.hasMageGuildLevel(akSpellTraits[spell].m_level - 1)
                || type < kNumTownTypes && akSpellTraits[spell].m_townGetsItChance[type] <= 0))
            spellsMask[spell] = false;
    }
    setDisabledSpellsMask(spellsMask);
    long numEvents;
    *pIStream >> numEvents;
    _m_events.resize(numEvents);
    for (unsigned int event = 0; event < numEvents; event++)
        _m_events[event].read(pIStream, version);
    if (version >= 28) {
        signed char alignment;
        *pIStream >> alignment;
        _m_alignment = TPlayer(alignment);
    } else {
        _m_alignment = ePlayerNone;
    }
    signed char aReserved[kNumTownReserved];
    *pIStream >> aReserved;
}

VA(0x004c22f5, 0x98)
TTown::~TTown()
{
    delete _m_pVisitingHero;
}

VA(0x004c238d, 0x17)
void TTown::setName(const std::string& newName)
{
    _m_name = newName;
}

VA(0x004c23a4, 0x2c)
void TTown::setGarrison(const TArmy& newGarrison)
{
    _m_garrison = newGarrison;
}

VA(0x004c23d0, 0x23)
void TTown::setBuildingStates(const TArray<TBuildingState, s_kNumBuildings>& newBuildingStates)
{
    _m_aBuildingState = newBuildingStates;
}

VA(0x004c23f3, 0x14)
void TTown::setObligatorySpellsMask(const std::bitset<kNumSpells>& newMask)
{
    _m_obligatorySpellsMask = newMask;
}

VA(0x004c2407, 0x14)
void TTown::setDisabledSpellsMask(const std::bitset<kNumSpells>& newMask)
{
    _m_disabledSpellsMask = newMask;
}

// h3maped 0x4c241b, which the runtime map names COleControl::
// GetStockTextMetrics (the same bytes).
void TTown::setTimedEvents(const std::vector<TTimedEvent>& newTimedEvents)
{
    _m_events = newTimedEvents;
}

VA(0x004c242d, 0xb5)
void TTown::setVisitingHero(const THero* pHero)
{
    delete _m_pVisitingHero;
    if (pHero != NULL) {
        std::auto_ptr<TGameObject> pClone = pHero->clone();
        if (pClone.get() == NULL)
            throw TAllocationFailure();
        _m_pVisitingHero = dynamic_cast<THero*>(pClone.release());
    } else {
        _m_pVisitingHero = NULL;
    }
}

VA(0x004c24e2, 0xd)
void TTown::setAlignment(TPlayer newAlignment)
{
    _m_alignment = newAlignment;
}

VA(0x004c24ef, 0x52e)
void TTown::importText(std::istream* pIStream, EGameVersion version)
{
    std::string line;
    if (_m_bCustomName) {
        getline(*pIStream, line);
        if (line != std::string(kNameStr) + ':')
            throw TImportTextFailure();
        getline(*pIStream, line);
        if (line.size() > s_kMaxNameLen)
            line.erase(s_kMaxNameLen);
        replace(line.begin(), line.end(), '\t', ' ');
        if (_m_bCustomName && find_if(line.begin(), line.end(), not1(ptr_fun(isspace))) == line.end())
            throw TImportTextFailure();
        setName(line);
    }
    if (_m_pVisitingHero != NULL && _m_pVisitingHero->hasText()) {
        getline(*pIStream, line);
        if (!line.empty())
            throw TImportTextFailure();
        getline(*pIStream, line);
        char buffer[256];
        sprintf(buffer, (std::string(kVisitingHeroFmtStr) + ':').c_str(), _m_pVisitingHero->getTypeName().c_str());
        if (line != buffer)
            throw TImportTextFailure();
        _m_pVisitingHero->importText(pIStream, version);
    }
    if (_m_events.size() > 0) {
        getline(*pIStream, line);
        if (!line.empty())
            throw TImportTextFailure();
        getline(*pIStream, line);
        if (line != std::string(kTimedEventsStr) + ':')
            throw TImportTextFailure();
        for (std::vector<TTimedEvent>::iterator iter = _m_events.begin(); iter != _m_events.end(); ++iter) {
            getline(*pIStream, line);
            if (!line.empty())
                throw TImportTextFailure();
            try {
                iter->importText(pIStream, version);
            } catch (const ::TTimedEvent::TImportTextFailure&) {
                throw TImportTextFailure();
            }
        }
    }
}

VA(0x004c2a1d, 0x7)
TLinkableObject* TTown::getPContainedObject()
{
    return _m_pVisitingHero;
}

const TLinkableObject* TTown::getPContainedObject() const
{
    return _m_pVisitingHero;
}

VA(0x004c2a24, 0x18)
TTownType TTown::getTownType() const
{
    return getType() != RANDOM_TOWN ? TTownType(getExtra()) : kNumTownTypes;
}

VA(0x004c2a3c, 0x4b)
bool TTown::getBIsBuildingDisabled(TBuilding building) const
{
    TBuilding requiredBuilding = getTownTypeTraits().m_akBuildingTraits[building].m_building;
    return _m_bCustomBuildings
           && (_m_aBuildingState[building].getBDisabled()
               || requiredBuilding != eBuildingNone && getBIsBuildingDisabled(requiredBuilding));
}

VA(0x004c2a87, 0x52)
bool TTown::isCustomized() const
{
    return _m_bCustomName || _m_bCustomGarrison || _m_bCustomBuildings || !_m_aBuildingState[eBuildingFort].getBBuilt()
           || _m_bGroupedFormation || _m_disabledSpellsMask.any() || _m_events.size() != 0 || _m_pVisitingHero != NULL
           || _m_alignment != ePlayerNone;
}

VA(0x004c2ad9, 0x252)
void TTown::write(TRawOStream* pOStream, int version) const
{
    if (version >= GAME_VERSION_AB)
        TLinkableObject::write(pOStream, version);
    TPlayableObject::write(pOStream, version);
    *pOStream << static_cast<signed char>(_m_bCustomName);
    if (_m_bCustomName)
        *pOStream << _m_name;
    *pOStream << static_cast<signed char>(_m_bCustomGarrison);
    if (_m_bCustomGarrison)
        getGarrison().write(pOStream, version);
    *pOStream << static_cast<signed char>(_m_bGroupedFormation);
    *pOStream << static_cast<signed char>(_m_bCustomBuildings);
    if (_m_bCustomBuildings) {
        unsigned char aBuilt[(s_kNumBuildings + 7) / 8];
        unsigned char aDisabled[(s_kNumBuildings + 7) / 8];
        fill_n(aBuilt, sizeof(aBuilt), 0);
        fill_n(aDisabled, sizeof(aDisabled), 0);
        for (unsigned int building = 0; building < s_kNumBuildings; building++) {
            unsigned int index = building / 8;
            unsigned char mask = 1 << building % 8;
            if (_m_aBuildingState[building].getBDisabled())
                aDisabled[index] |= mask;
            else if (_m_aBuildingState[building].getBBuilt())
                aBuilt[index] |= mask;
        }
        *pOStream << aBuilt << aDisabled;
    } else {
        *pOStream << static_cast<signed char>(_m_aBuildingState[eBuildingFort].getBBuilt());
    }
    unsigned char aSpells[(kNumSpells + 7) / 8];
    if (version >= GAME_VERSION_AB) {
        fill_n(aSpells, sizeof(aSpells), 0);
        for (unsigned int spell = 0; spell < kNumSpells; spell++)
            if (_m_obligatorySpellsMask[spell])
                aSpells[spell / 8] |= 1 << spell % 8;
        *pOStream << aSpells;
    }
    fill_n(aSpells, sizeof(aSpells), 0);
    for (unsigned int spell = 0; spell < kNumSpells; spell++)
        if (_m_disabledSpellsMask[spell])
            aSpells[spell / 8] |= 1 << spell % 8;
    *pOStream << aSpells;
    *pOStream << static_cast<long>(_m_events.size());
    for (std::vector<TTimedEvent>::const_iterator iter = _m_events.begin(); iter != _m_events.end(); ++iter)
        iter->write(pOStream, version);
    if (version >= GAME_VERSION_SOD)
        *pOStream << static_cast<signed char>(_m_alignment);
    signed char aReserved[kNumTownReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x004c2d2b, 0x6c)
std::string TTown::getTypeName() const
{
    if (getType() == TOWN)
        return s_akTypeTraits[getExtra()].m_pName;
    return TGameObject::getTypeName();
}

VA(0x004c2d97, 0x45)
bool TTown::hasText() const
{
    return _m_bCustomName || _m_pVisitingHero != NULL && _m_pVisitingHero->hasText() || _m_events.size() > 0;
}

VA(0x004c2ddc, 0x1eb)
void TTown::exportText(std::ostream* pOStream, EGameVersion version) const
{
    if (_m_bCustomName)
        *pOStream << kNameStr << ':' << '\n' << getName() << '\n';
    if (_m_pVisitingHero != NULL && _m_pVisitingHero->hasText()) {
        char buffer[256];
        sprintf(buffer, (std::string(kVisitingHeroFmtStr) + ':').c_str(), _m_pVisitingHero->getTypeName().c_str());
        *pOStream << '\n' << buffer << '\n';
        _m_pVisitingHero->exportText(pOStream, version);
    }
    if (_m_events.size() > 0) {
        *pOStream << '\n' << kTimedEventsStr << ':' << '\n';
        for (std::vector<TTimedEvent>::const_iterator iter = _m_events.begin(); iter != _m_events.end(); ++iter) {
            *pOStream << '\n';
            iter->exportText(pOStream, version);
        }
    }
}
