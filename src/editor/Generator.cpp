// Generator.cpp - the creature generators (h3maped 0x43dde6..0x43ee7e;
// name inferred, Complete only): Loki's TGenerator, moved out of
// ObjectSpecializations.cpp, and the Armageddon's Blade random dwellings,
// whose alignment follows a town or a set of town types and whose level is
// drawn from a range. The name tables are filled at start-up: the
// generators' from crgen1.txt and crgen4.txt, the random dwellings' from
// the editor's format strings.
//
// The Windows release throws the bare runtime error and allocation
// failure where Loki's port names the file and line, and drops the
// asserts. Every Complete generator can be flagged.
#include "editor/stdafx.h"

#include <string.h>
#include <algorithm>

#include "va.h"
#include "autoarrayptr.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/BitsetIterator.h"
#include "editor/FormattedStdString.h"
#include "editor/Generator.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"
#include "editor/Town.h"

namespace {
DATA(0x005841c0) TGenerator::TGeneratorTypeTraits aGenerator1TypeTraitsImp[TGenerator::s_kNumGenerator1Types] = {
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 0-7
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 8-15
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 16-23
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 24-31
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 32-39
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 40-47
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 48-55
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 56-63
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },  // 64-71
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true }   // 72-79
};
DATA(0x00584440) TGenerator::TGeneratorTypeTraits aGenerator4TypeTraitsImp[TGenerator::s_kNumGenerator4Types] = {
    { true }, { true }
};
}

DATA(0x00584450) const TGenerator::TGeneratorTypeTraits* TGenerator::s_akGenerator1TypeTraits = aGenerator1TypeTraitsImp;
DATA(0x00584454) const TGenerator::TGeneratorTypeTraits* TGenerator::s_akGenerator4TypeTraits = aGenerator4TypeTraitsImp;

VA(0x0043dfba, 0x26d)
void TGenerator::initializeTypeTraitsTables()
{
    DATA_COMPGEN_GUARD(0x0059e4ac, generatorNamesGuard, apGenerator1Names)
    {
        VA_COMPGEN(0x0043e23d, 0x16, STATIC_DTOR, apGenerator1Names)
        DATA(0x0059e4b8) static TAutoArrayPtr<char> apGenerator1Names;
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("crgen1.txt"));
        if (!pTextResource.get())
            throw TRuntimeError();
        unsigned int namesSize = 0;
        unsigned int i;
        for (i = 0; i < s_kNumGenerator1Types; ++i)
            namesSize += strlen(pTextResource->GetText(i)) + 1;
        apGenerator1Names = TAutoArrayPtr<char>(new char[namesSize]);
        if (!apGenerator1Names.get())
            throw TAllocationFailure();
        char* pName = apGenerator1Names.get();
        for (i = 0; i < s_kNumGenerator1Types; ++i) {
            const char* pText = pTextResource->GetText(i);
            size_t len = strlen(pText) + 1;
            memcpy(pName, pText, len);
            aGenerator1TypeTraitsImp[i].m_name = pName;
            pName += len;
        }
    }
    {
        VA_COMPGEN(0x0043e227, 0x16, STATIC_DTOR, apGenerator4Names)
        DATA(0x0059e4c8) static TAutoArrayPtr<char> apGenerator4Names;
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("crgen4.txt"));
        if (!pTextResource.get())
            throw TRuntimeError();
        unsigned int namesSize = 0;
        unsigned int i;
        for (i = 0; i < s_kNumGenerator4Types; ++i)
            namesSize += strlen(pTextResource->GetText(i)) + 1;
        apGenerator4Names = TAutoArrayPtr<char>(new char[namesSize]);
        if (!apGenerator4Names.get())
            throw TAllocationFailure();
        char* pName = apGenerator4Names.get();
        for (i = 0; i < s_kNumGenerator4Types; ++i) {
            const char* pText = pTextResource->GetText(i);
            size_t len = strlen(pText) + 1;
            memcpy(pName, pText, len);
            aGenerator4TypeTraitsImp[i].m_name = pName;
            pName += len;
        }
    }
}

VA(0x0043e253, 0x83)
TGenerator::TGenerator(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TFlaggableObject(objType, owner)
{
    if (!getGeneratorTypeTraits().m_bFlaggable)
        setOwner(ePlayerNone);
}

VA(0x0043e2d6, 0x86)
TGenerator::TGenerator(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version)
{
    if (!getGeneratorTypeTraits().m_bFlaggable)
        setOwner(ePlayerNone);
}

VA(0x0043e35c, 0x22)
const TGenerator::TGeneratorTypeTraits& TGenerator::getGeneratorTypeTraits() const
{
    if (getType() == CREATURE_GENERATOR_1)
        return s_akGenerator1TypeTraits[getGenerator1Type()];
    return s_akGenerator4TypeTraits[getGenerator4Type()];
}

VA(0x0043e37e, 0x3e)
std::string TGenerator::getTypeName() const
{
    return getGeneratorTypeTraits().m_name;
}

VA(0x0043e3bc, 0x59)
TAbstractRandomlyAlignedGenerator::TAbstractRandomlyAlignedGenerator(const TObjectType& objType)
    : TGameObject(objType), _m_townLinkID(TLinkableObject::s_kNoLinkID),
      _m_alignments(std::bitset<kNumTownTypes>().set())
{
}

VA(0x0043e415, 0xd8)
TAbstractRandomlyAlignedGenerator::TAbstractRandomlyAlignedGenerator(const TObjectType& objType,
                                                                     TRawIStream* pIStream, int version)
    : TGameObject(objType), _m_alignments(std::bitset<kNumTownTypes>().set())
{
    unsigned int townLinkID;
    *pIStream >> townLinkID;
    _m_townLinkID = townLinkID;
    if (_m_townLinkID != TLinkableObject::s_kNoLinkID) {
        _m_alignments.set();
    } else if (version >= 16) {
        readBitset(*pIStream, _m_alignments);
    } else {
        std::bitset<8> alignments;
        readBitset(*pIStream, alignments);
        std::copy(TBitsetIterator<8>(alignments, 0), TBitsetIterator<8>(alignments, 8),
                  TBitsetIterator<kNumTownTypes>(_m_alignments, 0));
    }
}

VA(0x0043e4ed, 0xc)
void TAbstractRandomlyAlignedGenerator::setAlignments(const std::bitset<kNumTownTypes>& newAlignments)
{
    _m_alignments = newAlignments;
}

VA(0x0043e4f9, 0x2f)
void TAbstractRandomlyAlignedGenerator::write(TRawOStream* pOStream, int version) const
{
    *pOStream << _m_townLinkID;
    if (_m_townLinkID == TLinkableObject::s_kNoLinkID)
        writeBitset(*pOStream, _m_alignments);
}

VA(0x0043e528, 0x48)
bool TAbstractRandomlyAlignedGenerator::isCustomized() const
{
    return _m_townLinkID != TLinkableObject::s_kNoLinkID || _m_alignments != ~std::bitset<kNumTownTypes>();
}

namespace {
DATA(0x0059e4dc) TRandomlyAlignedGenerator::TTypeTraits aRandomlyAlignedTypeTraitsImp[TRandomlyAlignedGenerator::s_kNumTypes];
}

DATA(0x00584458) const TRandomlyAlignedGenerator::TTypeTraits* TRandomlyAlignedGenerator::s_akTypeTraits
    = aRandomlyAlignedTypeTraitsImp;

VA(0x0043e570, 0x11e)
void TRandomlyAlignedGenerator::initializeTypeTraitsTable()
{
    DATA_COMPGEN_GUARD(0x0059e4c4, randomlyAlignedNamesGuard, apAlignedNames)
    VA_COMPGEN(0x0043e6cb, 0x16, STATIC_DTOR, apAlignedNames)
    DATA(0x0059e488) static TAutoArrayPtr<char> apAlignedNames;
    unsigned int namesSize = 0;
    int level;
    for (level = 0; level < s_kNumTypes; ++level)
        namesSize += TFormattedStdString(kRandomDwellingFmtStr, level + 1).length() + 1;
    apAlignedNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apAlignedNames.get())
        throw TAllocationFailure();
    char* pName = apAlignedNames.get();
    for (level = 0; level < s_kNumTypes; ++level) {
        TFormattedStdString name(kRandomDwellingFmtStr, level + 1);
        memcpy(pName, name.c_str(), name.length() + 1);
        aRandomlyAlignedTypeTraitsImp[level].m_name = pName;
        pName += name.length() + 1;
    }
}

VA(0x0043e6e1, 0x93)
TRandomlyAlignedGenerator::TRandomlyAlignedGenerator(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TFlaggableObject(objType, owner), TAbstractRandomlyAlignedGenerator(objType)
{
}

VA(0x0043e774, 0xc)
TFlaggableObject* TRandomlyAlignedGenerator::getPFlaggableObject()
{
    return this;
}

const TFlaggableObject* TRandomlyAlignedGenerator::getPFlaggableObject() const
{
    return this;
}

VA(0x0043e780, 0x9c)
TRandomlyAlignedGenerator::TRandomlyAlignedGenerator(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version),
      TAbstractRandomlyAlignedGenerator(objType, pIStream, version)
{
}

VA(0x0043e81c, 0x3c)
void TRandomlyAlignedGenerator::write(TRawOStream* pOStream, int version) const
{
    TFlaggableObject::write(pOStream, version);
    TAbstractRandomlyAlignedGenerator::write(pOStream, version);
}

VA(0x0043e858, 0x55)
TAbstractRandomlyLeveledGenerator::TAbstractRandomlyLeveledGenerator(const TObjectType& objType)
    : TGameObject(objType), _m_minLevel(0), _m_maxLevel(s_kNumLevels - 1)
{
}

VA(0x0043e8ad, 0x8f)
TAbstractRandomlyLeveledGenerator::TAbstractRandomlyLeveledGenerator(const TObjectType& objType,
                                                                     TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
    signed char minLevel;
    signed char maxLevel;
    *pIStream >> minLevel >> maxLevel;
    _m_minLevel = minLevel;
    _m_maxLevel = maxLevel;
}

VA(0x0043e93c, 0x2b)
void TAbstractRandomlyLeveledGenerator::write(TRawOStream* pOStream, int version) const
{
    *pOStream << static_cast<signed char>(_m_minLevel) << static_cast<signed char>(_m_maxLevel);
}

namespace {
DATA(0x0059e4f8) TRandomlyLeveledGenerator::TTypeTraits aRandomlyLeveledTypeTraitsImp[kNumTownTypes];
}

DATA(0x0058445c) const TRandomlyLeveledGenerator::TTypeTraits* TRandomlyLeveledGenerator::s_akTypeTraits
    = aRandomlyLeveledTypeTraitsImp;

VA(0x0043e967, 0x128)
void TRandomlyLeveledGenerator::initializeTypeTraitsTable()
{
    DATA_COMPGEN_GUARD(0x0059e4b4, randomlyLeveledNamesGuard, apLeveledNames)
    VA_COMPGEN(0x0043ea8f, 0x16, STATIC_DTOR, apLeveledNames)
    DATA(0x0059e4a0) static TAutoArrayPtr<char> apLeveledNames;
    unsigned int namesSize = 0;
    int type;
    for (type = 0; type < kNumTownTypes; ++type)
        namesSize += TFormattedStdString(kRandomTownDwellingFmtStr, TTown::s_akTypeTraits[type].m_pName).length() + 1;
    apLeveledNames = TAutoArrayPtr<char>(new char[namesSize]);
    if (!apLeveledNames.get())
        throw TAllocationFailure();
    char* pName = apLeveledNames.get();
    for (type = 0; type < kNumTownTypes; ++type) {
        TFormattedStdString name(kRandomTownDwellingFmtStr, TTown::s_akTypeTraits[type].m_pName);
        memcpy(pName, name.c_str(), name.length() + 1);
        aRandomlyLeveledTypeTraitsImp[type].m_name = pName;
        pName += name.length() + 1;
    }
}

VA(0x0043eaa5, 0x93)
TRandomlyLeveledGenerator::TRandomlyLeveledGenerator(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TFlaggableObject(objType, owner), TAbstractRandomlyLeveledGenerator(objType)
{
}

TFlaggableObject* TRandomlyLeveledGenerator::getPFlaggableObject()
{
    return this;
}

const TFlaggableObject* TRandomlyLeveledGenerator::getPFlaggableObject() const
{
    return this;
}

VA(0x0043eb38, 0x9c)
TRandomlyLeveledGenerator::TRandomlyLeveledGenerator(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version),
      TAbstractRandomlyLeveledGenerator(objType, pIStream, version)
{
}

VA(0x0043ebd4, 0x3a)
void TRandomlyLeveledGenerator::write(TRawOStream* pOStream, int version) const
{
    TFlaggableObject::write(pOStream, version);
    TAbstractRandomlyLeveledGenerator::write(pOStream, version);
}

VA(0x0043ec0e, 0xb3)
TRandomGenerator::TRandomGenerator(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TFlaggableObject(objType, owner), TAbstractRandomlyAlignedGenerator(objType),
      TAbstractRandomlyLeveledGenerator(objType)
{
}

VA(0x0043ecc1, 0xc)
TFlaggableObject* TRandomGenerator::getPFlaggableObject()
{
    return this;
}

const TFlaggableObject* TRandomGenerator::getPFlaggableObject() const
{
    return this;
}

VA(0x0043eccd, 0xc2)
TRandomGenerator::TRandomGenerator(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TFlaggableObject(objType, pIStream, version),
      TAbstractRandomlyAlignedGenerator(objType, pIStream, version),
      TAbstractRandomlyLeveledGenerator(objType, pIStream, version)
{
}

VA(0x0043ed8f, 0x5e)
void TRandomGenerator::write(TRawOStream* pOStream, int version) const
{
    TFlaggableObject::write(pOStream, version);
    TAbstractRandomlyAlignedGenerator::write(pOStream, version);
    TAbstractRandomlyLeveledGenerator::write(pOStream, version);
}

VA(0x0043eded, 0x21)
bool TRandomGenerator::isCustomized() const
{
    return TAbstractRandomlyAlignedGenerator::isCustomized() || TAbstractRandomlyLeveledGenerator::isCustomized();
}
