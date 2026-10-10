// Monster.cpp - a wandering monster (h3maped 0x488545..0x488f0e; Loki
// h3maped object 19): its size, disposition and flags, and the message,
// resources and artifact it may guard. The Windows monster is linkable;
// a Restoration of Erathia map keeps 300 characters of its message, and
// the artifact is a short from map version 21 (written for Armageddon's
// Blade on). The release drops Loki's asserts.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "adventureobjecttype.h"
#include "creaturetype.h"
#include "editor/Clamp.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/RawStream.h"

// The reserved byte count of the map format record.
const unsigned int kNumMonsterReserved = 2;

VA(0x00488719, 0xb2)
TMonster::TMonster(const TObjectType& objType)
    : TGameObject(objType), TLinkableObject(objType), _m_quantity(0), _m_disposition(eDispositionAggressive),
      _m_bNeverFlees(false), _m_bNeverGrows(false), _m_resourceQuantities(0), _m_artifact(ARTIFACT_NONE)
{
}

VA(0x004887cb, 0x220)
TMonster::TMonster(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TLinkableObject(objType, pIStream, version)
{
    short quantity;
    signed char disposition;
    *pIStream >> quantity >> disposition;
    setQuantity(quantity <= s_kMaxQuantity ? quantity : s_kMaxQuantity);
    setDisposition(TDisposition(disposition));
    signed char bHasMessage;
    *pIStream >> bHasMessage;
    if (!bHasMessage) {
        fill(_m_resourceQuantities.begin(), _m_resourceQuantities.end(), 0u);
        _m_artifact = ARTIFACT_NONE;
    } else {
        std::string message;
        *pIStream >> message;
        if (version <= akMapFileVersion[GAME_VERSION_ROE] && message.size() > s_kMaxMessageLen)
            message.erase(s_kMaxMessageLen);
        setMessage(message);
        for (unsigned int type = 0; type < kNumGameResourceTypes; ++type) {
            long resourceQuantity;
            *pIStream >> resourceQuantity;
            setResourceQuantity(TGameResourceType(type),
                                clamp(0, static_cast<int>(resourceQuantity), static_cast<int>(s_kMaxResourceQuantity)));
        }
        if (version >= 21) {
            short artifact;
            *pIStream >> artifact;
            setArtifact(TArtifact(artifact));
        } else {
            signed char artifact;
            *pIStream >> artifact;
            setArtifact(TArtifact(artifact));
        }
    }
    signed char bNeverFlees;
    *pIStream >> bNeverFlees;
    setBNeverFlees(bNeverFlees != 0);
    signed char bNeverGrows;
    *pIStream >> bNeverGrows;
    setBNeverGrows(bNeverGrows != 0);
    signed char aReserved[kNumMonsterReserved];
    *pIStream >> aReserved;
}

void TMonster::setQuantity(unsigned int newQuantity)
{
    _m_quantity = newQuantity;
}

void TMonster::setDisposition(TDisposition newDisposition)
{
    _m_disposition = newDisposition;
}

void TMonster::setMessage(const std::string& newMessage)
{
    _m_message = newMessage;
}

void TMonster::setArtifact(TArtifact newArtifact)
{
    _m_artifact = newArtifact;
}

VA(0x004889eb, 0xf)
void TMonster::setResourceQuantity(TGameResourceType type, unsigned int newQuantity)
{
    _m_resourceQuantities[type] = newQuantity;
}

VA(0x004889fa, 0x174)
void TMonster::importText(std::istream* pIStream, EGameVersion version)
{
    std::string line;
    getline(*pIStream, line);
    if (line != std::string(kMessageStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    replace(line.begin(), line.end(), '\t', '\n');
    if (version == GAME_VERSION_ROE && line.size() > s_kMaxMessageLen)
        line.erase(s_kMaxMessageLen);
    setMessage(line);
}

VA(0x00488b6e, 0x6f)
bool TMonster::isCustomized() const
{
    return _m_quantity != 0 || _m_disposition != eDispositionAggressive || _m_bNeverFlees || _m_bNeverGrows
           || !_m_message.empty() || !(_m_resourceQuantities == TArray<unsigned int, kNumGameResourceTypes>(0))
           || _m_artifact != ARTIFACT_NONE;
}

VA(0x00488bdd, 0xb)
unsigned int TMonster::getResourceQuantity(TGameResourceType type) const
{
    return _m_resourceQuantities[type];
}

VA(0x00488be8, 0x18d)
void TMonster::write(TRawOStream* pOStream, int version) const
{
    TLinkableObject::write(pOStream, version);
    *pOStream << static_cast<short>(_m_quantity) << static_cast<signed char>(_m_disposition);
    if (_m_message == std::string() && _m_resourceQuantities == TArray<unsigned int, kNumGameResourceTypes>(0)
        && _m_artifact == ARTIFACT_NONE) {
        *pOStream << static_cast<signed char>(0);
    } else {
        *pOStream << static_cast<signed char>(1) << _m_message;
        for (unsigned int type = 0; type < kNumGameResourceTypes; ++type)
            *pOStream << static_cast<long>(_m_resourceQuantities[type]);
        if (version >= GAME_VERSION_AB)
            *pOStream << static_cast<short>(_m_artifact);
        else
            *pOStream << static_cast<signed char>(_m_artifact);
    }
    *pOStream << static_cast<signed char>(_m_bNeverFlees) << static_cast<signed char>(_m_bNeverGrows);
    signed char aReserved[kNumMonsterReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x00488d75, 0x6e)
std::string TMonster::getTypeName() const
{
    if (getType() == MONSTER)
        return akCreatureTypeTraits[getCreatureType()].m_name;
    return TGameObject::getTypeName();
}

VA(0x00488de3, 0xca)
void TMonster::exportText(std::ostream* pOStream, EGameVersion version) const
{
    std::string message = getMessage();
    replace(message.begin(), message.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << message << '\n';
}
