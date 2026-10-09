// ObjectSpecializations.cpp - the specialized map objects (h3maped
// 0x48d4da..0x48fa1b; Loki h3maped object 21): generic, flaggable, mine,
// abandoned mine, garrison, playable, treasure, artifact, spell scroll,
// resource, sign, scholar, grail, shrine and witch hut, and the creature
// bank, monolith, keymaster's tent, mine and garrison name tables, filled
// from the game's text resources at start-up.
//
// The Windows release throws the bare runtime error and allocation
// failure where Loki's port names the file and line, and drops the
// asserts. The creature generators moved to their own object.
#include "editor/stdafx.h"

#include <ctype.h>
#include <string.h>
#include <algorithm>

#include "va.h"
#include "artifact.h"
#include "autoarrayptr.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "retailobjecttype.h"
#include "textresource.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/RawStream.h"

// The reserved byte counts of the map format records this file reads and
// writes.
const unsigned int kNumFlaggableObjectReserved = 3;
const unsigned int kNumAbandonedMineReserved = 3;
const unsigned int kNumGarrisonReserved = 8;
const unsigned int kNumTreasureReserved = 4;
const unsigned int kNumOldAbandonedMineReserved = 4;
const unsigned int kNumSpellScrollReserved = 3;
const unsigned int kNumGameResourceReserved = 4;
const unsigned int kNumSignReserved = 4;
const unsigned int kNumScholarReserved = 6;
const unsigned int kNumHolyGrailReserved = 3;
const unsigned int kNumShrineReserved = 3;

namespace {
DATA(0x005a2180) TCreatureBankTypeTraits aCreatureBankTypeTraitsImp[kNumCreatureBankTypes];
DATA(0x005a2140) TMonolithTypeTraits aMonolithTypeTraitsImp[kNumMonolithTypes];
DATA(0x005a2160) TMonolithTypeTraits aOneWayMonolithTypeTraitsImp[kNumOneWayMonolithTypes];
DATA(0x005a219c) TMine::TTypeTraits aMineTypeTraitsImp[TMine::s_kNumMineTypes];
DATA(0x005a21bc) TGarrison::TTypeTraits aGarrisonTypeTraitsImp[TGarrison::s_kNumTypes];
DATA(0x005a21c4) TKeymasterTentTypeTraits aKeymasterTentTypeTraitsImp[kNumKeymasterTentTypes];
}

DATA(0x0058cff4) const TCreatureBankTypeTraits* akCreatureBankTypeTraits = aCreatureBankTypeTraitsImp;
DATA(0x0058cff8) const TMonolithTypeTraits* akMonolithTypeTraits = aMonolithTypeTraitsImp;
DATA(0x0058cffc) const TMonolithTypeTraits* akOneWayMonolithTypeTraits = aOneWayMonolithTypeTraitsImp;
DATA(0x0058d000) const TKeymasterTentTypeTraits* akKeymasterTentTypeTraits = aKeymasterTentTypeTraitsImp;

VA(0x0048d6ae, 0x151)
void InitializeCreatureBankTypeTraitsTable()
{
    DATA_COMPGEN_GUARD(0x005a2214, creatureBankNamesGuard, apCreatureBankNames)
    VA_COMPGEN(0x0048d7ff, 0x16, STATIC_DTOR, apCreatureBankNames)
    DATA(0x005a21f8) static TAutoArrayPtr<char> apCreatureBankNames;
    TResourcePtr<TSpreadsheetResource> pSpreadsheet(ResourceManager::GetSpreadsheet("crbanks.txt"));
    if (!pSpreadsheet.get())
        throw TRuntimeError();
    int row = 2;
    unsigned int namesSize = 0;
    unsigned int i;
    for (i = 0; i < kNumCreatureBankTypes; ++i) {
        namesSize += strlen(pSpreadsheet->GetRow(row)[0]) + 1;
        row += 4;
    }
    apCreatureBankNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apCreatureBankNames.get())
        throw TAllocationFailure();
    row = 2;
    char* pName = apCreatureBankNames.get();
    for (i = 0; i < kNumCreatureBankTypes; ++i) {
        const char* pText = pSpreadsheet->GetRow(row)[0];
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        aCreatureBankTypeTraitsImp[i].m_name = pName;
        pName += len;
        row += 4;
    }
}

VA(0x0048d815, 0x16e)
void InitializeMonolithTypeTraitsTables()
{
    DATA_COMPGEN_GUARD(0x005a2201, monolithNamesGuard, apMonolithNames)
    VA_COMPGEN(0x0048d983, 0x16, STATIC_DTOR, apMonolithNames)
    DATA(0x005a2138) static TAutoArrayPtr<char> apMonolithNames;
    {
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("monolith.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    unsigned int namesSize = 0;
    unsigned int i;
    for (i = 0; i < kNumMonolithTypes + kNumOneWayMonolithTypes; ++i)
        namesSize += strlen(pTextResource->GetText(i)) + 1;
    apMonolithNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apMonolithNames.get())
        throw TAllocationFailure();
    char* pName = apMonolithNames.get();
    for (i = 0; i < kNumMonolithTypes + kNumOneWayMonolithTypes; ++i) {
        const char* pText = pTextResource->GetText(i);
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        pName += len;
    }
    }
    const char* pNext = apMonolithNames.get();
    unsigned int type;
    for (type = 0; type < kNumMonolithTypes; ++type) {
        aMonolithTypeTraitsImp[type].m_name = pNext;
        pNext += strlen(pNext) + 1;
    }
    for (type = 0; type < kNumOneWayMonolithTypes; ++type) {
        aOneWayMonolithTypeTraitsImp[type].m_name = pNext;
        pNext += strlen(pNext) + 1;
    }
}

VA(0x0048d999, 0x123)
void InitializeKeymasterTentTypeTraitsTable()
{
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("TentColr.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    unsigned int namesSize = 0;
    unsigned int i;
    for (i = 0; i < kNumKeymasterTentTypes; ++i)
        namesSize += strlen(pTextResource->GetText(i)) + 1;
    DATA_COMPGEN_GUARD(0x005a2200, keymasterTentNamesGuard, apTentColorNames)
    VA_COMPGEN(0x0048dabc, 0x16, STATIC_DTOR, apTentColorNames)
    DATA(0x005a21e8) static TAutoArrayPtr<char> apTentColorNames(new char[namesSize]);
    if (!apTentColorNames.get())
        throw TAllocationFailure();
    char* pName = apTentColorNames.get();
    for (i = 0; i < kNumKeymasterTentTypes; ++i) {
        const char* pText = pTextResource->GetText(i);
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        aKeymasterTentTypeTraitsImp[i].m_name = pName;
        for (size_t j = 0; j < len; ++j, ++pName)
            *pName = tolower(*pName);
    }
}

VA(0x0048dad2, 0x41)
TGenericObject::TGenericObject(const TObjectType& objType) : TGameObject(objType)
{
}

VA(0x0048db13, 0x41)
TGenericObject::TGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
}

void TGenericObject::write(TRawOStream* pOStream, int version) const
{
}

VA(0x0048db54, 0x60)
std::string TGenericObject::getTypeName() const
{
    if (getType() == CREATURE_BANK)
        return akCreatureBankTypeTraits[getExtra()].m_name;
    return TGameObject::getTypeName();
}

VA(0x0048dbb4, 0x47)
TFlaggableObject::TFlaggableObject(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), _m_owner(owner)
{
}

VA(0x0048dbfb, 0xa2)
TFlaggableObject::TFlaggableObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
    if ((objType.getType() == GARRISON && version < 4) || (objType.getType() == SHIPYARD && version < 10)) {
        _m_owner = ePlayerNone;
        return;
    }
    signed char owner;
    *pIStream >> owner;
    signed char aReserved[kNumFlaggableObjectReserved];
    *pIStream >> aReserved;
    _m_owner = TPlayer(owner);
}

VA(0x0048dc9d, 0x30)
void TFlaggableObject::write(TRawOStream* pOStream, int version) const
{
    *pOStream << static_cast<signed char>(_m_owner);
    signed char aReserved[kNumFlaggableObjectReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

DATA(0x0058d004) const TMine::TTypeTraits* TMine::s_akMineTypeTraits = aMineTypeTraitsImp;

VA(0x0048dccd, 0x136)
void TMine::initializeTypeTraitsTable()
{
    DATA_COMPGEN_GUARD(0x005a220c, mineNamesGuard, apMineNames)
    VA_COMPGEN(0x0048de03, 0x16, STATIC_DTOR, apMineNames)
    DATA(0x005a2118) static TAutoArrayPtr<char> apMineNames;
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("minename.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    unsigned int namesSize = 0;
    unsigned int i;
    for (i = 0; i < s_kNumMineTypes; ++i)
        namesSize += strlen(pTextResource->GetText(i)) + 1;
    apMineNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apMineNames.get())
        throw TAllocationFailure();
    char* pName = apMineNames.get();
    for (i = 0; i < s_kNumMineTypes; ++i) {
        const char* pText = pTextResource->GetText(i);
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        aMineTypeTraitsImp[i].m_name = pName;
        pName += len;
    }
}

VA(0x0048de19, 0x70)
TMine::TMine(const TObjectType& objType, TPlayer owner) : TGameObject(objType), TFlaggableObject(objType, owner)
{
}

VA(0x0048de89, 0x73)
TMine::TMine(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version)
{
}

VA(0x0048defc, 0x6a)
std::string TMine::getTypeName() const
{
    if (getType() == MINE || getType() == ABANDONED_MINE)
        return s_akMineTypeTraits[getExtra()].m_name;
    return TGameObject::getTypeName();
}

DATA(0x005a2218) const std::bitset<kNumGameResourceTypes> TAbandonedMine::_s_kabDefaultPotentialResource
    = ~std::bitset<kNumGameResourceTypes>(1);

VA(0x0048df90, 0x77)
TAbandonedMine::TAbandonedMine(const TObjectType& objType)
    : TGameObject(objType), TMine(objType, ePlayerNone), _m_abPotentialResource(_s_kabDefaultPotentialResource)
{
}

VA(0x0048e007, 0xd1)
TAbandonedMine::TAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TMine(objType, ePlayerNone)
{
    if (version < 12) {
        _m_abPotentialResource = _s_kabDefaultPotentialResource;
        signed char aReserved[kNumOldAbandonedMineReserved];
        *pIStream >> aReserved;
        return;
    }
    unsigned char potentialResources;
    *pIStream >> potentialResources;
    for (unsigned int type = 0; type < kNumGameResourceTypes; ++type)
        _m_abPotentialResource[type] = (potentialResources & (1 << type)) != 0;
    signed char aReserved[kNumAbandonedMineReserved];
    *pIStream >> aReserved;
}

VA(0x0048e0d8, 0x13)
void TAbandonedMine::setBIsPotentialResource(TGameResourceType type, bool bIsPotential)
{
    _m_abPotentialResource[type] = bIsPotential;
}

VA(0x0048e0eb, 0x30)
bool TAbandonedMine::getBIsPotentialResource(TGameResourceType type) const
{
    return _m_abPotentialResource[type];
}

VA(0x0048e11b, 0x64)
void TAbandonedMine::write(TRawOStream* pOStream, int version) const
{
    unsigned char potentialResources = 0;
    for (unsigned int type = 0; type < kNumGameResourceTypes; ++type)
        if (_m_abPotentialResource[type])
            potentialResources |= 1 << type;
    *pOStream << potentialResources;
    signed char aReserved[kNumAbandonedMineReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

DATA(0x0058d008) const TGarrison::TTypeTraits* TGarrison::s_akTypeTraits = aGarrisonTypeTraitsImp;

VA(0x0048e17f, 0x136)
void TGarrison::initializeTypeTraitsTable()
{
    DATA_COMPGEN_GUARD(0x005a21f4, garrisonNamesGuard, apGarrisonNames)
    VA_COMPGEN(0x0048e2b5, 0x16, STATIC_DTOR, apGarrisonNames)
    DATA(0x005a2128) static TAutoArrayPtr<char> apGarrisonNames;
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("garrison.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    unsigned int namesSize = 0;
    unsigned int i;
    for (i = 0; i < s_kNumTypes; ++i)
        namesSize += strlen(pTextResource->GetText(i)) + 1;
    apGarrisonNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apGarrisonNames.get())
        throw TAllocationFailure();
    char* pName = apGarrisonNames.get();
    for (i = 0; i < s_kNumTypes; ++i) {
        const char* pText = pTextResource->GetText(i);
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        aGarrisonTypeTraitsImp[i].m_name = pName;
        pName += len;
    }
}

VA(0x0048e2cb, 0x7f)
TGarrison::TGarrison(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TFlaggableObject(objType, owner), _m_bRemovableUnits(true)
{
}

VA(0x0048e34a, 0xe7)
TGarrison::TGarrison(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version)
{
    if (version < 4)
        return;
    setArmy(TArmy(pIStream, version));
    if (version >= 20) {
        signed char bRemovableUnits;
        *pIStream >> bRemovableUnits;
        _m_bRemovableUnits = bRemovableUnits != 0;
    } else {
        _m_bRemovableUnits = true;
    }
    signed char aReserved[kNumGarrisonReserved];
    *pIStream >> aReserved;
}

VA(0x0048e431, 0x2c)
void TGarrison::setArmy(const TArmy& newArmy)
{
    _m_army = newArmy;
}

VA(0x0048e45d, 0x2b)
bool TGarrison::isCustomized() const
{
    return !_m_bRemovableUnits
           || find_if(_m_army.begin(), _m_army.end(), bind2nd(not_equal_to<TCreatureStack>(), TCreatureStack()))
              != _m_army.end();
}

VA(0x0048e488, 0x55)
void TGarrison::write(TRawOStream* pOStream, int version) const
{
    TFlaggableObject::write(pOStream, version);
    _m_army.write(pOStream, version);
    if (version >= 1)
        *pOStream << static_cast<signed char>(_m_bRemovableUnits);
    signed char aReserved[kNumGarrisonReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x0048e4dd, 0x49)
std::string TGarrison::getTypeName() const
{
    return s_akTypeTraits[getExtra()].m_name;
}

VA(0x0048e526, 0x47)
TPlayableObject::TPlayableObject(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), _m_owner(owner)
{
}

VA(0x0048e56d, 0x74)
TPlayableObject::TPlayableObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
    signed char owner;
    *pIStream >> owner;
    _m_owner = TPlayer(owner);
}

VA(0x0048e5e1, 0x1a)
void TPlayableObject::write(TRawOStream* pOStream, int version) const
{
    *pOStream << static_cast<signed char>(_m_owner);
}

VA(0x0048e5fb, 0x1e)
void TPlayableObject::read(TRawIStream* pIStream, int version)
{
    signed char owner;
    *pIStream >> owner;
    _m_owner = TPlayer(owner);
}

VA(0x0048e619, 0x7f)
TTreasure::TTreasure(const TObjectType& objType) : TGameObject(objType), _m_bCustomGuardians(false)
{
}

VA(0x0048e698, 0x90)
TTreasure::TTreasure(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), _m_bCustomGuardians(false)
{
    read(pIStream, version);
}

VA(0x0048e728, 0xf8)
void TTreasure::read(TRawIStream* pIStream, int version)
{
    if (version < 5)
        return;
    signed char flag;
    *pIStream >> flag;
    if (!flag)
        return;
    std::string message;
    *pIStream >> message;
    if (version <= akMapFileVersion[GAME_VERSION_ROE] && message.size() > s_kMaxMessageLen)
        message.erase(s_kMaxMessageLen);
    setMessage(message);
    *pIStream >> flag;
    if (flag) {
        _m_bCustomGuardians = true;
        setGuardians(TArmy(pIStream, version));
    }
    signed char aReserved[kNumTreasureReserved];
    *pIStream >> aReserved;
}

// Folded by /OPT:ICF onto TSign::setText (0x48f0c7), which every other
// unit's call reaches.
void TTreasure::setMessage(const std::string& newMessage)
{
    _m_message = newMessage;
}

VA(0x0048e820, 0x2c)
void TTreasure::setGuardians(const TArmy& newGuardians)
{
    _m_guardians = newGuardians;
}

VA(0x0048e84c, 0x174)
void TTreasure::importText(std::istream* pIStream, EGameVersion version)
{
    std::string line;
    getline(*pIStream, line);
    if (line != std::string(kMessageStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    if (version == GAME_VERSION_ROE && line.size() > s_kMaxMessageLen)
        line.erase(s_kMaxMessageLen);
    replace(line.begin(), line.end(), '\t', '\n');
    setMessage(line);
}

VA(0x0048e9c0, 0x10)
bool TTreasure::isCustomized() const
{
    return !getMessage().empty() || getBCustomGuardians();
}

VA(0x0048e9d0, 0x80)
void TTreasure::write(TRawOStream* pOStream, int version) const
{
    if (!getMessage().empty() || getBCustomGuardians()) {
        *pOStream << static_cast<signed char>(1) << _m_message << static_cast<signed char>(_m_bCustomGuardians);
        if (_m_bCustomGuardians)
            _m_guardians.write(pOStream, version);
        signed char aReserved[kNumTreasureReserved];
        fill_n(aReserved, sizeof(aReserved), 0);
        *pOStream << aReserved;
    } else {
        *pOStream << static_cast<signed char>(0);
    }
}

VA(0x0048ea50, 0xca)
void TTreasure::exportText(std::ostream* pOStream, EGameVersion version) const
{
    std::string message = getMessage();
    replace(message.begin(), message.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << message << '\n';
}

VA(0x0048eb1a, 0x6d)
TGameArtifact::TGameArtifact(const TObjectType& objType) : TGameObject(objType), TTreasure(objType)
{
}

VA(0x0048eb87, 0x73)
TGameArtifact::TGameArtifact(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType, pIStream, version)
{
}

VA(0x0048ebfa, 0x6d)
std::string TGameArtifact::getTypeName() const
{
    if (getType() == ARTIFACT)
        return akArtifactTraits[getArtifactType()].m_name;
    return TGameObject::getTypeName();
}

VA(0x0048ec67, 0x74)
TSpellScroll::TSpellScroll(const TObjectType& objType)
    : TGameObject(objType), TTreasure(objType), _m_spell(SPELL_MAGIC_ARROW)
{
}

VA(0x0048ecdb, 0x97)
TSpellScroll::TSpellScroll(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType, pIStream, version)
{
    signed char spell;
    *pIStream >> spell;
    setSpell(ESpellId(spell));
    signed char aReserved[kNumSpellScrollReserved];
    *pIStream >> aReserved;
}

VA(0x0048ed72, 0xa)
void TSpellScroll::setSpell(ESpellId newSpell)
{
    _m_spell = newSpell;
}

VA(0x0048ed7c, 0x1e)
bool TSpellScroll::isCustomized() const
{
    return TTreasure::isCustomized() || _m_spell != SPELL_MAGIC_ARROW;
}

VA(0x0048ed9a, 0x42)
void TSpellScroll::write(TRawOStream* pOStream, int version) const
{
    TTreasure::write(pOStream, version);
    *pOStream << static_cast<signed char>(_m_spell);
    signed char aReserved[kNumSpellScrollReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x0048eddc, 0x70)
TGameResource::TGameResource(const TObjectType& objType)
    : TGameObject(objType), TTreasure(objType), _m_quantity(0)
{
}

VA(0x0048ee4c, 0xa0)
TGameResource::TGameResource(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType, pIStream, version)
{
    if (version >= 5) {
        long quantity;
        *pIStream >> quantity;
        setQuantity(quantity);
        signed char aReserved[kNumGameResourceReserved];
        *pIStream >> aReserved;
    } else {
        _m_quantity = 0;
    }
}

VA(0x0048eeec, 0x1b)
bool TGameResource::isCustomized() const
{
    return TTreasure::isCustomized() || _m_quantity != 0;
}

VA(0x0048ef07, 0x40)
void TGameResource::write(TRawOStream* pOStream, int version) const
{
    TTreasure::write(pOStream, version);
    *pOStream << static_cast<long>(_m_quantity);
    signed char aReserved[kNumGameResourceReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x0048ef47, 0x6a)
std::string TGameResource::getTypeName() const
{
    if (getType() == RESOURCE)
        return akGameResourceTypeTraits[getResourceType()].m_name;
    return TGameObject::getTypeName();
}

VA(0x0048efb1, 0x70)
TSign::TSign(const TObjectType& objType) : TGameObject(objType)
{
}

VA(0x0048f021, 0xa6)
TSign::TSign(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
    *pIStream >> _m_text;
    if (_m_text.size() > s_kMaxTextLen)
        _m_text.erase(s_kMaxTextLen);
    signed char aReserved[kNumSignReserved];
    *pIStream >> aReserved;
}

VA(0x0048f0c7, 0x17)
void TSign::setText(const std::string& newText)
{
    _m_text = newText;
}

VA(0x0048f0de, 0x16f)
void TSign::importText(std::istream* pIStream, EGameVersion version)
{
    std::string line;
    getline(*pIStream, line);
    if (line != std::string(kMessageStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    if (line.size() > s_kMaxTextLen)
        line.erase(s_kMaxTextLen);
    replace(line.begin(), line.end(), '\t', '\n');
    setText(line);
}

VA(0x0048f24d, 0x2a)
void TSign::write(TRawOStream* pOStream, int version) const
{
    *pOStream << _m_text;
    signed char aReserved[kNumSignReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x0048f277, 0xca)
void TSign::exportText(std::ostream* pOStream, EGameVersion version) const
{
    std::string text = getText();
    replace(text.begin(), text.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << text << '\n';
}

VA(0x0048f341, 0x54)
TScholar::TScholar(const TObjectType& objType)
    : TGameObject(objType), _m_rewardType(eRewardRandom), _m_primarySkill(TPrimarySkill(0)),
      _m_secondarySkill(TSecondarySkill(0)), _m_spell(SPELL_MAGIC_ARROW)
{
}

VA(0x0048f395, 0xbd)
TScholar::TScholar(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), _m_primarySkill(TPrimarySkill(0)), _m_secondarySkill(TSecondarySkill(0)),
      _m_spell(SPELL_MAGIC_ARROW)
{
    signed char rewardType;
    signed char reward;
    *pIStream >> rewardType >> reward;
    setRewardType(TRewardType(rewardType));
    switch (_m_rewardType) {
    case eRewardPrimarySkill:
        setPrimarySkill(TPrimarySkill(reward));
        break;
    case eRewardSecondarySkill:
        setSecondarySkill(TSecondarySkill(reward));
        break;
    case eRewardSpell:
        setSpell(ESpellId(reward));
        break;
    }
    signed char aReserved[kNumScholarReserved];
    *pIStream >> aReserved;
}

void TScholar::setRewardType(TRewardType newRewardType)
{
    _m_rewardType = newRewardType;
}

VA(0x0048f452, 0xa)
void TScholar::setPrimarySkill(TPrimarySkill newPrimarySkill)
{
    _m_primarySkill = newPrimarySkill;
}

void TScholar::setSecondarySkill(TSecondarySkill newSecondarySkill)
{
    _m_secondarySkill = newSecondarySkill;
}

VA(0x0048f45c, 0xa)
void TScholar::setSpell(ESpellId newSpell)
{
    _m_spell = newSpell;
}

VA(0x0048f466, 0x5e)
void TScholar::write(TRawOStream* pOStream, int version) const
{
    signed char reward = 0;
    switch (_m_rewardType) {
    case eRewardPrimarySkill:
        reward = _m_primarySkill;
        break;
    case eRewardSecondarySkill:
        reward = _m_secondarySkill;
        break;
    case eRewardSpell:
        reward = _m_spell;
        break;
    }
    *pOStream << static_cast<signed char>(_m_rewardType) << reward;
    signed char aReserved[kNumScholarReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x0048f4c4, 0x45)
THolyGrail::THolyGrail(const TObjectType& objType) : TGameObject(objType), _m_radius(0)
{
}

VA(0x0048f509, 0x8c)
THolyGrail::THolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
    if (version < 3) {
        _m_radius = 0;
    } else {
        signed char radius;
        *pIStream >> radius;
        setRadius(radius);
        signed char aReserved[kNumHolyGrailReserved];
        *pIStream >> aReserved;
    }
}

VA(0x0048f595, 0x30)
void THolyGrail::write(TRawOStream* pOStream, int version) const
{
    *pOStream << static_cast<signed char>(_m_radius);
    signed char aReserved[kNumHolyGrailReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x0048f5c5, 0x45)
TShrine::TShrine(const TObjectType& objType) : TGameObject(objType), _m_spell(SPELL_NONE)
{
}

VA(0x0048f60a, 0xdc)
TShrine::TShrine(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
    if (version < 11) {
        _m_spell = SPELL_NONE;
    } else {
        signed char spell;
        *pIStream >> spell;
        int newSpell = spell;
        if (newSpell != SPELL_NONE
            && (akSpellTraits[newSpell].m_level != getSpellLevel() || (akSpellTraits[newSpell].m_flags & 0x2000)
                || akSpellTraits[newSpell].m_schoolBits == 0))
            newSpell = SPELL_NONE;
        setSpell(ESpellId(newSpell));
        signed char aReserved[kNumShrineReserved];
        *pIStream >> aReserved;
    }
}

VA(0x0048f6e6, 0x26)
int TShrine::getSpellLevel() const
{
    switch (getType()) {
    case SHRINE1:
        return 1;
    case SHRINE2:
        return 2;
    case SHRINE3:
        return 3;
    }
    return 0;
}

VA(0x0048f70c, 0xa)
bool TShrine::isCustomized() const
{
    return _m_spell != SPELL_NONE;
}

VA(0x0048f716, 0x30)
void TShrine::write(TRawOStream* pOStream, int version) const
{
    *pOStream << static_cast<signed char>(_m_spell);
    signed char aReserved[kNumShrineReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

DATA(0x005a2208) const std::bitset<kNumSecSkills> TWitchHut::_s_kabDefaultPotentialSkill
    = ~(std::bitset<kNumSecSkills>(1) << eSecSkillLeadership) & ~(std::bitset<kNumSecSkills>(1) << eSecSkillNecromancy);

VA(0x0048f7bc, 0x49)
TWitchHut::TWitchHut(const TObjectType& objType)
    : TGameObject(objType), _m_abPotentialSkill(_s_kabDefaultPotentialSkill)
{
}

VA(0x0048f805, 0x82)
TWitchHut::TWitchHut(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
    if (version >= 20)
        readBitset(*pIStream, _m_abPotentialSkill);
    else
        _m_abPotentialSkill = _s_kabDefaultPotentialSkill;
}

VA(0x0048f887, 0xc)
void TWitchHut::setPotentialSkills(const std::bitset<kNumSecSkills>& newSkills)
{
    _m_abPotentialSkill = newSkills;
}

VA(0x0048f893, 0xe)
bool TWitchHut::isCustomized() const
{
    return _m_abPotentialSkill != _s_kabDefaultPotentialSkill;
}

VA(0x0048f8a1, 0x62)
void TWitchHut::write(TRawOStream* pOStream, int version) const
{
    if (version >= 1)
        writeBitset(*pOStream, _m_abPotentialSkill);
}
