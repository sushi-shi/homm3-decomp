// Monster.cpp - Loki h3maped object 19: a wandering monster, its size,
// disposition and flags, and the message, resources and artifact it may
// guard. Assert lines come from the retail immediates.
#include <assert.h>
#include <algorithm>
#include <iostream.h>
#include <string>

#include "adventureobjecttype.h"
#include "creaturetype.h"
#include "artifact.h"
#include "editor/Clamp.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/RawStream.h"

TMonster::TMonster(const TObjectType& objType)
    : TGameObject(objType), _m_quantity(0), _m_disposition(eDispositionAggressive), _m_bNeverFlees(false),
      _m_bNeverGrows(false), _m_resourceQuantities(0), _m_artifact(eArtifactNone)
{
#line 52
    assert(objType.getType() == MONSTER || objType.getType() == RANDOM_MONSTER || objType.getType() == RANDOM_MONSTER_1 || objType.getType() == RANDOM_MONSTER_2 || objType.getType() == RANDOM_MONSTER_3 || objType.getType() == RANDOM_MONSTER_4 || objType.getType() == RANDOM_MONSTER_5 || objType.getType() == RANDOM_MONSTER_6 || objType.getType() == RANDOM_MONSTER_7);
    assert(getCreatureType() >= 0 && getCreatureType() < kNumCreatureTypes);
}

TMonster::TMonster(const TObjectType& objType, TRawIStream* pIStream, int version) : TGameObject(objType)
{
#line 68
    assert(objType.getType() == MONSTER || objType.getType() == RANDOM_MONSTER || objType.getType() == RANDOM_MONSTER_1 || objType.getType() == RANDOM_MONSTER_2 || objType.getType() == RANDOM_MONSTER_3 || objType.getType() == RANDOM_MONSTER_4 || objType.getType() == RANDOM_MONSTER_5 || objType.getType() == RANDOM_MONSTER_6 || objType.getType() == RANDOM_MONSTER_7);
    assert(objType.getType() != MONSTER || ( getCreatureType() >= 0 && getCreatureType() < kNumCreatureTypes ));
    assert(pIStream != NULL);
    short quantity;
    signed char disposition;
    *pIStream >> quantity >> disposition;
    setQuantity(quantity <= s_kMaxQuantity ? quantity : s_kMaxQuantity);
    setDisposition(TDisposition(disposition));
    signed char bHasMessage;
    *pIStream >> bHasMessage;
    if (!bHasMessage) {
        fill(_m_resourceQuantities.begin(), _m_resourceQuantities.end(), 0u);
        _m_artifact = eArtifactNone;
    } else {
        string message;
        *pIStream >> message;
        if (message.size() > s_kMaxMessageLen)
            message.erase(s_kMaxMessageLen);
        setMessage(message);
        for (unsigned int type = 0; type < kNumGameResourceTypes; ++type) {
            long resourceQuantity;
            *pIStream >> resourceQuantity;
            setResourceQuantity(TGameResourceType(type),
                                clamp(0, (int) resourceQuantity, (int) s_kMaxResourceQuantity));
        }
        signed char artifact;
        *pIStream >> artifact;
        setArtifact(TArtifact(artifact));
    }
    signed char bNeverFlees;
    *pIStream >> bNeverFlees;
    setBNeverFlees(bNeverFlees != 0);
    signed char bNeverGrows;
    *pIStream >> bNeverGrows;
    setBNeverGrows(bNeverGrows != 0);
    signed char aReserved[2];
    *pIStream >> aReserved;
}

void TMonster::setQuantity(unsigned int newQuantity)
{
#line 121
    assert(newQuantity <= s_kMaxQuantity);
    _m_quantity = newQuantity;
}

void TMonster::setDisposition(TDisposition newDisposition)
{
#line 128
    assert(newDisposition >= 0 && newDisposition < s_kNumDispositions);
    _m_disposition = newDisposition;
}

void TMonster::setMessage(const string& newMessage)
{
#line 135
    assert(newMessage.size() <= s_kMaxMessageLen);
    assert(newMessage.find( '\t' ) == std::string::npos);
    _m_message = newMessage;
}

void TMonster::setResourceQuantity(TGameResourceType type, unsigned int newQuantity)
{
#line 144
    assert(type >= 0 && type < kNumGameResourceTypes);
    assert(newQuantity <= s_kMaxResourceQuantity);
    _m_resourceQuantities[type] = newQuantity;
}

void TMonster::setArtifact(TArtifact newArtifact)
{
#line 153
    assert(newArtifact >= eArtifactNone && newArtifact < kNumArtifacts);
    assert(newArtifact == eArtifactNone || ( akArtifactTraits[ newArtifact ].m_class & ArtifactClassSpecial ) == 0);
    _m_artifact = newArtifact;
}

void TMonster::importText(istream* pIStream)
{
#line 162
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

bool TMonster::isCustomized() const
{
    bool bResources = false;
    for (unsigned int type = 0; type < kNumGameResourceTypes; ++type)
        if (_m_resourceQuantities[type] == 0)
            bResources = true;
    return _m_quantity != 0 || _m_disposition != eDispositionAggressive || _m_bNeverFlees || _m_bNeverGrows
           || !_m_message.empty() || bResources || _m_artifact != eArtifactNone;
}

unsigned int TMonster::getResourceQuantity(TGameResourceType type) const
{
#line 201
    assert(type >= 0 && type < kNumGameResourceTypes);
    return _m_resourceQuantities[type];
}

void TMonster::write(TRawOStream* pOStream) const
{
#line 209
    assert(pOStream != NULL);
    *pOStream << (short) _m_quantity << (signed char) _m_disposition;
    if (_m_message == string() && _m_resourceQuantities == TArray<unsigned int, kNumGameResourceTypes>(0)
        && _m_artifact == eArtifactNone) {
        *pOStream << (signed char) 0;
    } else {
        *pOStream << (signed char) 1 << _m_message;
        for (unsigned int type = 0; type < kNumGameResourceTypes; ++type)
            *pOStream << (long) _m_resourceQuantities[type];
        *pOStream << (signed char) _m_artifact;
    }
    *pOStream << (signed char) _m_bNeverFlees << (signed char) _m_bNeverGrows;
    signed char aReserved[2];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

string TMonster::getTypeName() const
{
    if (getType() == MONSTER)
        return akCreatureTypeTraits[getCreatureType()].m_name;
    return TGameObject::getTypeName();
}

void TMonster::exportText(ostream* pOStream) const
{
#line 252
    assert(pOStream != NULL);
    string message = getMessage();
    replace(message.begin(), message.end(), '\n', '\t');
    *pOStream << kMessageStr << ':' << '\n' << message << '\n';
}
