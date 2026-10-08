// ObjectSpecializations.cpp - Loki h3maped object 21: the specialized map
// objects (generic, flaggable, playable, treasure, artifact, spell scroll,
// resource, sign, scholar, garrison, mine, abandoned mine, generator,
// grail, shrine) and the creature bank, monolith, mine, generator and
// garrison name tables, filled from the game's text resources at start-up.
// Assert and throw lines come from the retail immediates.
#include <assert.h>
#include <string.h>
#include <algorithm>
#include <functional>
#include <iostream.h>
#include <string>

#include "adventureobjecttype.h"
#include "autoarrayptr.h"
#include "exceptions.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/RawStream.h"
#include "textresource.h"

namespace {
TCreatureBankTypeTraits aCreatureBankTypeTraitsImp[kNumCreatureBankTypes];
TMonolithTypeTraits aMonolithTypeTraitsImp[kNumMonolithTypes];
TMonolithTypeTraits aOneWayMonolithTypeTraitsImp[kNumOneWayMonolithTypes];
TMine::TTypeTraits aMineTypeTraitsImp[TMine::s_kNumMineTypes];
TGarrison::TTypeTraits aGarrisonTypeTraitsImp[TGarrison::s_kNumTypes];
}

const TCreatureBankTypeTraits* akCreatureBankTypeTraits = aCreatureBankTypeTraitsImp;
const TMonolithTypeTraits* akMonolithTypeTraits = aMonolithTypeTraitsImp;
const TMonolithTypeTraits* akOneWayMonolithTypeTraits = aOneWayMonolithTypeTraitsImp;

void InitializeCreatureBankTypeTraitsTable()
{
    static TAutoArrayPtr<char> apNames;
    TResourcePtr<TSpreadsheetResource> pSpreadsheet(ResourceManager::GetSpreadsheet("crbanks.txt"));
    if (!pSpreadsheet.get())
#line 79
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"crbanks.txt\".");
    assert(pSpreadsheet->GetNumberOfRows() >= kNumCreatureBankTypes * 4 + 2);
    int row = 2;
    unsigned int namesSize = 0;
    for (unsigned int i = 0; i < kNumCreatureBankTypes; ++i) {
        namesSize += strlen(pSpreadsheet->GetRow(row)[0]) + 1;
        row += 4;
    }
    apNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apNames.get())
#line 95
        throw TAllocationFailure(__FILE__, __LINE__);
    row = 2;
    char* pName = apNames.get();
    for (i = 0; i < kNumCreatureBankTypes; ++i) {
        const char* pText = pSpreadsheet->GetRow(row)[0];
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        aCreatureBankTypeTraitsImp[i].m_name = pName;
        pName += len;
        row += 4;
    }
}

void InitializeMonolithTypeTraitsTables()
{
    static TAutoArrayPtr<char> apNames;
    {
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("monolith.txt"));
    if (!pTextResource.get())
#line 126
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"monolith.txt\".");
    assert(pTextResource->GetNumberOfStrings() >= kNumMonolithTypes + kNumOneWayMonolithTypes);
    unsigned int namesSize = 0;
    for (unsigned int i = 0; i < kNumMonolithTypes + kNumOneWayMonolithTypes; ++i)
        namesSize += strlen(pTextResource->GetText(i)) + 1;
    apNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apNames.get())
#line 138
        throw TAllocationFailure(__FILE__, __LINE__);
    char* pName = apNames.get();
    for (i = 0; i < kNumMonolithTypes + kNumOneWayMonolithTypes; ++i) {
        const char* pText = pTextResource->GetText(i);
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        pName += len;
    }
    }
    const char* pNext = apNames.get();
    for (unsigned int type = 0; type < kNumMonolithTypes; ++type) {
        aMonolithTypeTraitsImp[type].m_name = pNext;
        pNext += strlen(pNext) + 1;
    }
    for (type = 0; type < kNumOneWayMonolithTypes; ++type) {
        aOneWayMonolithTypeTraitsImp[type].m_name = pNext;
        pNext += strlen(pNext) + 1;
    }
}

TGenericObject::TGenericObject(const TObjectType& objType) : TGameObject(objType)
{
}

TGenericObject::TGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
#line 180
    assert(pIStream != NULL);
}

void TGenericObject::write(TRawOStream* pOStream) const
{
#line 186
    assert(pOStream != NULL);
}

string TGenericObject::getTypeName() const
{
    if (getType() == CREATURE_BANK) {
#line 194
        assert(getExtra() >= 0 && getExtra() < kNumCreatureBankTypes);
        return akCreatureBankTypeTraits[getExtra()].m_name;
    }
    return TGameObject::getTypeName();
}

TFlaggableObject::TFlaggableObject(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), _m_owner(owner)
{
#line 209
    assert(_m_owner >= ePlayerNone && _m_owner < kNumPlayers);
}

TFlaggableObject::TFlaggableObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
#line 216
    assert(pIStream != NULL);
    if ((objType.getType() == GARRISON && version < 4) || (objType.getType() == SHIPYARD && version < 10)) {
        _m_owner = ePlayerNone;
        return;
    }
    signed char owner;
    *pIStream >> owner;
    signed char aReserved[3];
    *pIStream >> aReserved;
    _m_owner = TPlayer(owner);
#line 233
    assert(_m_owner >= ePlayerNone && _m_owner < kNumPlayers);
}

void TFlaggableObject::write(TRawOStream* pOStream) const
{
#line 239
    assert(pOStream != NULL);
    *pOStream << (signed char) _m_owner;
    signed char aReserved[3];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

const TMine::TTypeTraits* TMine::s_akMineTypeTraits = aMineTypeTraitsImp;

void TMine::initializeTypeTraitsTable()
{
    static TAutoArrayPtr<char> apNames;
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("minename.txt"));
    if (!pTextResource.get())
#line 272
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"minename.txt\".");
    assert(pTextResource->GetNumberOfStrings() >= s_kNumMineTypes);
    unsigned int namesSize = 0;
    for (unsigned int i = 0; i < s_kNumMineTypes; ++i)
        namesSize += strlen(pTextResource->GetText(i)) + 1;
    apNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apNames.get())
#line 284
        throw TAllocationFailure(__FILE__, __LINE__);
    char* pName = apNames.get();
    for (i = 0; i < s_kNumMineTypes; ++i) {
        const char* pText = pTextResource->GetText(i);
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        aMineTypeTraitsImp[i].m_name = pName;
        pName += len;
    }
}

TMine::TMine(const TObjectType& objType, TPlayer owner) : TGameObject(objType), TFlaggableObject(objType, owner)
{
#line 303
    assert(objType.getType() == MINE || objType.getType() == LIGHTHOUSE);
}

TMine::TMine(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version)
{
#line 311
    assert(objType.getType() == MINE || objType.getType() == LIGHTHOUSE);
}

string TMine::getTypeName() const
{
    if (getType() == MINE) {
#line 319
        assert(getExtra() >= 0 && getExtra() < s_kNumMineTypes);
        return s_akMineTypeTraits[getExtra()].m_name;
    }
    return TGameObject::getTypeName();
}

const bitset<kNumGameResourceTypes> TAbandonedMine::_s_kabDefaultPotentialResource
    = ~bitset<kNumGameResourceTypes>(1);

TAbandonedMine::TAbandonedMine(const TObjectType& objType)
    : TGameObject(objType), TMine(objType, ePlayerNone), _m_abPotentialResource(_s_kabDefaultPotentialResource)
{
#line 338
    assert(objType.getType() == MINE && objType.getExtra() == kNumGameResourceTypes);
}

TAbandonedMine::TAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TMine(objType, ePlayerNone)
{
#line 346
    assert(objType.getType() == MINE && objType.getExtra() == kNumGameResourceTypes);
    if (version < 12) {
        _m_abPotentialResource = _s_kabDefaultPotentialResource;
        signed char aReserved[4];
        *pIStream >> aReserved;
        return;
    }
    unsigned char potentialResources;
    *pIStream >> potentialResources;
    for (unsigned int type = 0; type < kNumGameResourceTypes; ++type)
        _m_abPotentialResource[type] = (potentialResources >> type) & 1;
#line 363
    assert(!_m_abPotentialResource[ eResourceWood ]);
    assert(_m_abPotentialResource.count() > 0);
    signed char aReserved[3];
    *pIStream >> aReserved;
}

void TAbandonedMine::setBIsPotentialResource(TGameResourceType type, bool bIsPotential)
{
#line 373
    assert(type >= 0 && type < kNumGameResourceTypes && type != eResourceWood);
    _m_abPotentialResource[type] = bIsPotential;
    assert(_m_abPotentialResource.count() > 0);
}

bool TAbandonedMine::getBIsPotentialResource(TGameResourceType type) const
{
#line 381
    assert(type >= 0 && type < kNumGameResourceTypes && type != eResourceWood);
    return _m_abPotentialResource[type];
}

void TAbandonedMine::write(TRawOStream* pOStream) const
{
#line 388
    assert(pOStream != NULL);
    unsigned char potentialResources = 0;
    for (unsigned int type = 0; type < kNumGameResourceTypes; ++type)
        if (_m_abPotentialResource[type])
            potentialResources |= 1 << type;
    *pOStream << potentialResources;
    signed char aReserved[3];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

// Whether a dwelling of each kind can be flagged is compiled in; the names
// come from crgen1.txt and crgen4.txt at startup.
namespace {
TGenerator::TGeneratorTypeTraits aGenerator1TypeTraitsImp[TGenerator::s_kNumGenerator1Types] = {
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { false },  // 0-7
    { true }, { true }, { true }, { true }, { true }, { false }, { true }, { true },  // 8-15
    { false }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 16-23
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 24-31
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 32-39
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { false },  // 40-47
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 48-55
    { true }, { true }, { true },  // 56-58
};
TGenerator::TGeneratorTypeTraits aGenerator4TypeTraitsImp[TGenerator::s_kNumGenerator4Types] = {
    { false }, { true }
};
}

const TGenerator::TGeneratorTypeTraits* TGenerator::s_akGenerator1TypeTraits = aGenerator1TypeTraitsImp;
const TGenerator::TGeneratorTypeTraits* TGenerator::s_akGenerator4TypeTraits = aGenerator4TypeTraitsImp;

void TGenerator::initializeTypeTraitsTables()
{
    {
        static TAutoArrayPtr<char> apNames;
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("crgen1.txt"));
        if (!pTextResource.get())
#line 499
            throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"crgen1.txt\".");
        assert(pTextResource->GetNumberOfStrings() >= s_kNumGenerator1Types);
        unsigned int namesSize = 0;
        for (unsigned int i = 0; i < s_kNumGenerator1Types; ++i)
            namesSize += strlen(pTextResource->GetText(i)) + 1;
        apNames = TAutoArrayPtr<char>(new char[namesSize]);
        if (!apNames.get())
#line 511
            throw TAllocationFailure(__FILE__, __LINE__);
        char* pName = apNames.get();
        for (i = 0; i < s_kNumGenerator1Types; ++i) {
            const char* pText = pTextResource->GetText(i);
            size_t len = strlen(pText) + 1;
            memcpy(pName, pText, len);
            aGenerator1TypeTraitsImp[i].m_name = pName;
            pName += len;
        }
    }
    {
        static TAutoArrayPtr<char> apNames;
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("crgen4.txt"));
        if (!pTextResource.get())
#line 531
            throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"crgen4.txt\".");
        assert(pTextResource->GetNumberOfStrings() >= s_kNumGenerator4Types);
        unsigned int namesSize = 0;
        for (unsigned int i = 0; i < s_kNumGenerator4Types; ++i)
            namesSize += strlen(pTextResource->GetText(i)) + 1;
        apNames = TAutoArrayPtr<char>(new char[namesSize]);
        if (!apNames.get())
#line 543
            throw TAllocationFailure(__FILE__, __LINE__);
        char* pName = apNames.get();
        for (i = 0; i < s_kNumGenerator4Types; ++i) {
            const char* pText = pTextResource->GetText(i);
            size_t len = strlen(pText) + 1;
            memcpy(pName, pText, len);
            aGenerator4TypeTraitsImp[i].m_name = pName;
            pName += len;
        }
    }
}

TGenerator::TGenerator(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TFlaggableObject(objType, owner)
{
#line 565
    assert(objType.getExtra() >= 0 && ( ( objType.getType() == CREATURE_GENERATOR_1 && objType.getExtra() < s_kNumGenerator1Types ) || ( objType.getType() == CREATURE_GENERATOR_4 && objType.getExtra() < s_kNumGenerator4Types ) ));
    if (!getGeneratorTypeTraits().m_bFlaggable)
        setOwner(ePlayerNone);
}

TGenerator::TGenerator(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version)
{
#line 578
    assert(objType.getExtra() >= 0 && ( ( objType.getType() == CREATURE_GENERATOR_1 && objType.getExtra() < s_kNumGenerator1Types ) || ( objType.getType() == CREATURE_GENERATOR_4 && objType.getExtra() < s_kNumGenerator4Types ) ));
    if (!getGeneratorTypeTraits().m_bFlaggable)
        setOwner(ePlayerNone);
}

const TGenerator::TGeneratorTypeTraits& TGenerator::getGeneratorTypeTraits() const
{
    if (getType() == CREATURE_GENERATOR_1)
        return s_akGenerator1TypeTraits[getGenerator1Type()];
#line 590
    assert(getType() == CREATURE_GENERATOR_4);
    return s_akGenerator4TypeTraits[getGenerator4Type()];
}

TGenerator::TGenerator1Type TGenerator::getGenerator1Type() const
{
#line 597
    assert(getType() == CREATURE_GENERATOR_1);
    assert(getExtra() >= 0 && getExtra() < s_kNumGenerator1Types);
    return TGenerator1Type(getExtra());
}

TGenerator::TGenerator4Type TGenerator::getGenerator4Type() const
{
#line 605
    assert(getType() == CREATURE_GENERATOR_4);
    assert(getExtra() >= 0 && getExtra() < s_kNumGenerator4Types);
    return TGenerator4Type(getExtra());
}

string TGenerator::getTypeName() const
{
    return getGeneratorTypeTraits().m_name;
}

const TGarrison::TTypeTraits* TGarrison::s_akTypeTraits = aGarrisonTypeTraitsImp;

void TGarrison::initializeTypeTraitsTable()
{
    static TAutoArrayPtr<char> apNames;
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("garrison.txt"));
    if (!pTextResource.get())
#line 637
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"garrison.txt\".");
    assert(pTextResource->GetNumberOfStrings() >= s_kNumTypes);
    unsigned int namesSize = 0;
    for (unsigned int i = 0; i < s_kNumTypes; ++i)
        namesSize += strlen(pTextResource->GetText(i)) + 1;
    apNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apNames.get())
#line 649
        throw TAllocationFailure(__FILE__, __LINE__);
    char* pName = apNames.get();
    for (i = 0; i < s_kNumTypes; ++i) {
        const char* pText = pTextResource->GetText(i);
        size_t len = strlen(pText) + 1;
        memcpy(pName, pText, len);
        aGarrisonTypeTraitsImp[i].m_name = pName;
        pName += len;
    }
}

TGarrison::TGarrison(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TFlaggableObject(objType, owner)
{
#line 668
    assert(objType.getType() == GARRISON);
}

TGarrison::TGarrison(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version)
{
#line 676
    assert(objType.getType() == GARRISON);
    if (version < 4)
        return;
    TArmy army;
    *pIStream >> army;
    setArmy(army);
    signed char aReserved[8];
    *pIStream >> aReserved;
}

void TGarrison::setArmy(const TArmy& newArmy)
{
    _m_army = newArmy;
}

bool TGarrison::isCustomized() const
{
    return find_if(_m_army.begin(), _m_army.end(), bind2nd(not_equal_to<TCreatureStack>(), TCreatureStack()))
           != _m_army.end();
}

void TGarrison::write(TRawOStream* pOStream) const
{
    TFlaggableObject::write(pOStream);
    *pOStream << _m_army;
    signed char aReserved[8];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

string TGarrison::getTypeName() const
{
    return getTypeTraits().m_name;
}

TPlayableObject::TPlayableObject(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), _m_owner(owner)
{
#line 736
    assert(_m_owner >= ePlayerNone && _m_owner < kNumPlayers);
}

TPlayableObject::TPlayableObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
#line 743
    assert(pIStream != NULL);
    signed char owner;
    *pIStream >> owner;
    _m_owner = TPlayer(owner);
#line 750
    assert(_m_owner >= ePlayerNone && _m_owner < kNumPlayers);
}

void TPlayableObject::write(TRawOStream* pOStream) const
{
#line 756
    assert(pOStream != NULL);
    *pOStream << (signed char) _m_owner;
}

TTreasure::TTreasure(const TObjectType& objType) : TGameObject(objType), _m_bCustomGuardians(false)
{
}

TTreasure::TTreasure(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), _m_bCustomGuardians(false)
{
    read(pIStream, version);
}

void TTreasure::read(TRawIStream* pIStream, int version)
{
#line 782
    assert(pIStream != NULL);
    if (version > 4) {
        signed char flag;
        *pIStream >> flag;
        if (flag) {
            string message;
            *pIStream >> message;
            if (message.size() > s_kMaxMessageLen)
                message.erase(s_kMaxMessageLen);
            setMessage(message);
            *pIStream >> flag;
            if (flag) {
                _m_bCustomGuardians = true;
                TArmy guardians;
                *pIStream >> guardians;
                setGuardians(guardians);
            } else {
#line 807
                assert(!_m_message.empty());
            }
            signed char aReserved[4];
            *pIStream >> aReserved;
        }
    }
}

void TTreasure::setMessage(const string& newMessage)
{
#line 818
    assert(newMessage.size() <= s_kMaxMessageLen);
    assert(newMessage.find( '\t' ) == std::string::npos);
    _m_message = newMessage;
}

void TTreasure::setGuardians(const TArmy& newGuardians)
{
    _m_guardians = newGuardians;
}

void TTreasure::importText(istream* pIStream)
{
#line 842
    assert(pIStream != NULL);
    string line;
    getline(*pIStream, line);
    if (line != string(kMessageStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    if (line.size() > s_kMaxMessageLen)
        line.erase(s_kMaxMessageLen);
    replace(line.begin(), line.end(), '\t', '\n');
    setMessage(line);
}

bool TTreasure::isCustomized() const
{
    return !getMessage().empty() || getBCustomGuardians();
}

void TTreasure::write(TRawOStream* pOStream) const
{
#line 866
    assert(pOStream != NULL);
    if (!getMessage().empty() || getBCustomGuardians()) {
        *pOStream << (signed char) 1 << _m_message << (signed char) _m_bCustomGuardians;
        if (_m_bCustomGuardians)
            *pOStream << _m_guardians;
        signed char aReserved[4];
        fill_n(aReserved, sizeof(aReserved), 0);
        *pOStream << aReserved;
    } else {
        *pOStream << (signed char) 0;
    }
}

void TTreasure::exportText(ostream* pOStream) const
{
#line 888
    assert(pOStream != NULL);
    string message = getMessage();
    replace(message.begin(), message.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << message << '\n';
}

TGameArtifact::TGameArtifact(const TObjectType& objType) : TGameObject(objType), TTreasure(objType)
{
#line 908
    assert(objType.getType() == ARTIFACT || objType.getType() == RANDOM_ARTIFACT || objType.getType() == RANDOM_ARTIFACT_1 || objType.getType() == RANDOM_ARTIFACT_2 || objType.getType() == RANDOM_ARTIFACT_3 || objType.getType() == RANDOM_ARTIFACT_4);
}

TGameArtifact::TGameArtifact(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType, pIStream, version)
{
#line 916
    assert(pIStream != NULL);
#line 922
    assert(objType.getType() == ARTIFACT || objType.getType() == RANDOM_ARTIFACT || objType.getType() == RANDOM_ARTIFACT_1 || objType.getType() == RANDOM_ARTIFACT_2 || objType.getType() == RANDOM_ARTIFACT_3 || objType.getType() == RANDOM_ARTIFACT_4);
}

string TGameArtifact::getTypeName() const
{
    if (getType() == ARTIFACT) {
#line 930
        assert(getArtifactType() >= 0 && getArtifactType() < kNumArtifacts);
        return akArtifactTraits[getArtifactType()].m_name;
    }
    return TGameObject::getTypeName();
}

TSpellScroll::TSpellScroll(const TObjectType& objType) : TGameObject(objType), TTreasure(objType), _m_spell(eSpellMagicBolt)
{
#line 946
    assert(objType.getType() == SPELL_SCROLL);
}

TSpellScroll::TSpellScroll(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType, pIStream, version)
{
#line 954
    assert(objType.getType() == SPELL_SCROLL);
    assert(pIStream != NULL);
    signed char spell;
    *pIStream >> spell;
    setSpell(SpellID(spell));
    signed char aReserved[3];
    *pIStream >> aReserved;
}

void TSpellScroll::setSpell(SpellID newSpellID)
{
#line 968
    assert(newSpellID >= 0 && newSpellID < kNumSpells);
    _m_spell = newSpellID;
}

bool TSpellScroll::isCustomized() const
{
    return TTreasure::isCustomized() || _m_spell != eSpellMagicBolt;
}

void TSpellScroll::write(TRawOStream* pOStream) const
{
#line 981
    assert(pOStream != NULL);
    TTreasure::write(pOStream);
    *pOStream << (signed char) _m_spell;
    signed char aReserved[3];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

TGameResource::TGameResource(const TObjectType& objType) : TGameObject(objType), TTreasure(objType), _m_quantity(0)
{
#line 1001
    assert(objType.getType() == RESOURCE || objType.getType() == RANDOM_RESOURCE);
}

TGameResource::TGameResource(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTreasure(objType, pIStream, version)
{
#line 1009
    assert(pIStream != NULL);
    assert(objType.getType() == RESOURCE || objType.getType() == RANDOM_RESOURCE);
    if (version > 4) {
        long quantity;
        *pIStream >> quantity;
        setQuantity(quantity);
        signed char aReserved[4];
        *pIStream >> aReserved;
    } else {
        _m_quantity = 0;
    }
}

void TGameResource::setQuantity(unsigned int newQuantity)
{
#line 1028
    assert(newQuantity <= s_kMaxQuantity);
    _m_quantity = newQuantity;
}

bool TGameResource::isCustomized() const
{
    return TTreasure::isCustomized() || _m_quantity != 0;
}

void TGameResource::write(TRawOStream* pOStream) const
{
#line 1042
    assert(pOStream != NULL);
    TTreasure::write(pOStream);
    *pOStream << (long) _m_quantity;
    signed char aReserved[4];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

string TGameResource::getTypeName() const
{
    if (getType() == RESOURCE) {
#line 1058
        assert(getResourceType() >= 0 && getResourceType() < kNumGameResourceTypes);
        return akGameResourceTypeTraits[getResourceType()].m_name;
    }
    return TGameObject::getTypeName();
}

TSign::TSign(const TObjectType& objType) : TGameObject(objType)
{
#line 1072
    assert(objType.getType() == SIGN || objType.getType() == OCEAN_BOTTLE);
}

TSign::TSign(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
#line 1079
    assert(objType.getType() == SIGN || objType.getType() == OCEAN_BOTTLE);
    assert(pIStream != NULL);
    *pIStream >> _m_text;
    if (_m_text.size() > s_kMaxTextLen)
        _m_text.erase(s_kMaxTextLen);
    signed char aReserved[4];
    *pIStream >> aReserved;
}

void TSign::setText(const string& newText)
{
#line 1093
    assert(newText.size() <= s_kMaxTextLen);
    assert(newText.find( '\t' ) == std::string::npos);
    _m_text = newText;
}

void TSign::importText(istream* pIStream)
{
#line 1102
    assert(pIStream != NULL);
    string line;
    getline(*pIStream, line);
    if (line != string(kMessageStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    if (line.size() > s_kMaxTextLen)
        line.erase(s_kMaxTextLen);
    replace(line.begin(), line.end(), '\t', '\n');
    setText(line);
}

void TSign::write(TRawOStream* pOStream) const
{
#line 1120
    assert(pOStream != NULL);
    *pOStream << _m_text;
    signed char aReserved[4];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

void TSign::exportText(ostream* pOStream) const
{
#line 1132
    assert(pOStream != NULL);
    string text = getText();
    replace(text.begin(), text.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << text << '\n';
}

TScholar::TScholar(const TObjectType& objType)
    : TGameObject(objType), _m_rewardType(eRewardRandom), _m_primarySkill(TPrimarySkill(0)),
      _m_secondarySkill(TSecondarySkill(0)), _m_spell(eSpellMagicBolt)
{
#line 1150
    assert(objType.getType() == SCHOLAR);
}

TScholar::TScholar(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), _m_primarySkill(TPrimarySkill(0)), _m_secondarySkill(TSecondarySkill(0)),
      _m_spell(eSpellMagicBolt)
{
#line 1160
    assert(objType.getType() == SCHOLAR);
    assert(pIStream != NULL);
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
        setSpell(SpellID(reward));
        break;
    }
    signed char aReserved[6];
    *pIStream >> aReserved;
}

void TScholar::setRewardType(TRewardType newRewardType)
{
#line 1190
    assert(( newRewardType >= 0 && newRewardType < s_kNumRewardTypes ) || newRewardType == eRewardRandom);
    _m_rewardType = newRewardType;
}

void TScholar::setPrimarySkill(TPrimarySkill newPrimarySkill)
{
#line 1197
    assert(newPrimarySkill >= 0 && newPrimarySkill < kNumPrimarySkills);
    _m_primarySkill = newPrimarySkill;
}

void TScholar::setSecondarySkill(TSecondarySkill newSecondarySkill)
{
#line 1204
    assert(newSecondarySkill >= 0 && newSecondarySkill < kNumSecSkills);
    _m_secondarySkill = newSecondarySkill;
}

void TScholar::setSpell(SpellID newSpellID)
{
#line 1211
    assert(newSpellID >= 0 && newSpellID < kNumSpells);
    _m_spell = newSpellID;
}

void TScholar::write(TRawOStream* pOStream) const
{
#line 1218
    assert(pOStream != NULL);
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
    *pOStream << (signed char) _m_rewardType << reward;
    signed char aReserved[6];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

THolyGrail::THolyGrail(const TObjectType& objType) : TGameObject(objType), _m_radius(0)
{
#line 1250
    assert(objType.getType() == HOLY_GRAIL);
}

THolyGrail::THolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
#line 1257
    assert(objType.getType() == HOLY_GRAIL);
    assert(pIStream != NULL);
    if (version < 3) {
        _m_radius = 0;
    } else {
        signed char radius;
        *pIStream >> radius;
        setRadius(radius);
        signed char aReserved[3];
        *pIStream >> aReserved;
    }
}

void THolyGrail::setRadius(unsigned int newRadius)
{
#line 1277
    assert(newRadius <= s_kMaxRadius);
    _m_radius = newRadius;
}

void THolyGrail::write(TRawOStream* pOStream) const
{
#line 1284
    assert(pOStream != NULL);
    *pOStream << (signed char) _m_radius;
    signed char aReserved[3];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

TShrine::TShrine(const TObjectType& objType) : TGameObject(objType), _m_spell(eSpellNone)
{
#line 1303
    assert(objType.getType() == SHRINE1 || objType.getType() == SHRINE2 || objType.getType() == SHRINE3);
}

TShrine::TShrine(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
#line 1312
    assert(objType.getType() == SHRINE1 || objType.getType() == SHRINE2 || objType.getType() == SHRINE3);
    assert(pIStream != NULL);
    if (version < 11) {
        _m_spell = eSpellNone;
    } else {
        signed char spell;
        *pIStream >> spell;
#line 1322
        assert(spell >= eSpellNone && spell < kNumSpells);
        if (spell != eSpellNone
            && (akSpellTraits[spell].m_level != getSpellLevel() || akSpellTraits[spell].m_school == 0))
            spell = eSpellNone;
        setSpell(SpellID(spell));
        signed char aReserved[3];
        *pIStream >> aReserved;
    }
}

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
#line 1350
    assert(false);
    return 0;
}

void TShrine::setSpell(SpellID newSpell)
{
#line 1357
    assert(newSpell >= eSpellNone && newSpell < kNumSpells);
#line 1360
    assert(newSpell == eSpellNone || ( akSpellTraits[ newSpell ].m_level == getSpellLevel() && akSpellTraits[ newSpell ].m_school != 0 ));
    _m_spell = newSpell;
}

bool TShrine::isCustomized() const
{
    return _m_spell != eSpellNone;
}

void TShrine::write(TRawOStream* pOStream) const
{
#line 1374
    assert(pOStream != NULL);
    *pOStream << (signed char) _m_spell;
    signed char aReserved[3];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}
