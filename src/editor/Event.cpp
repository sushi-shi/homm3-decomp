// Event.cpp - Loki h3maped object 11: the map event, a Pandora's box that
// fires for the players it allows and may cancel itself after a visit.
// Assert lines come from the retail immediates.
#include <assert.h>
#include <algorithm>

#include "adventureobjecttype.h"
#include "exceptions.h"
#include "editor/Event.h"
#include "editor/RawStream.h"

// The reserved byte counts of the map format records this file reads and
// writes. The object emits them at the end of its .rodata in this order
// (values proven there); the names are inferred.
const unsigned int kNumOldEventReserved = 16;
const unsigned int kNumEventReserved = 4;

TEvent::TEvent(const TObjectType& objType)
    : TGameObject(objType), TBlackBox(objType), _m_bAllowPlayer(~bitset<kNumPlayers>(0)), _m_bAllowComputer(false),
      _m_bCancelAfterVisit(true)
{
#line 38
    assert(objType.getType() == EVENT);
}

TEvent::TEvent(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TBlackBox(objType, pIStream, version)
{
#line 46
    assert(objType.getType() == EVENT);
    assert(pIStream != NULL);
    if (version < 6) {
        string message;
        *pIStream >> message;
        if (message.size() > s_kMaxMessageLen)
            message.erase(s_kMaxMessageLen);
        setMessage(message);
        TResourceQuantities resourceQuantities;
        *pIStream >> resourceQuantities;
        getPContents()->setResourceQuantities(resourceQuantities);
        signed char artifact;
        *pIStream >> artifact;
        if (artifact != -1) {
            vector<TArtifact> aArtifact(1, TArtifact(artifact));
            getPContents()->setArtifacts(aArtifact);
        }
    }
    unsigned char players;
    *pIStream >> players;
    for (unsigned int player = 0; player < kNumPlayers; ++player)
        setBAllowPlayer(TPlayer(player), (players >> player) & 1);
    signed char bAllowComputer;
    signed char bCancelAfterVisit;
    *pIStream >> bAllowComputer >> bCancelAfterVisit;
    setBAllowComputer(bAllowComputer != 0);
    setBCancelAfterVisit(bCancelAfterVisit != 0);
    if (version < 6) {
        signed char aReserved[kNumOldEventReserved];
        *pIStream >> aReserved;
    } else {
        signed char aReserved[kNumEventReserved];
        *pIStream >> aReserved;
    }
}

void TEvent::setBAllowPlayer(TPlayer player, bool bAllow)
{
#line 98
    assert(player >= 0 && player < kNumPlayers);
    _m_bAllowPlayer[player] = bAllow;
}

bool TEvent::getBAllowPlayer(TPlayer player) const
{
#line 106
    assert(player >= 0 && player < kNumPlayers);
    return _m_bAllowPlayer[player];
}

bool TEvent::isCustomized() const
{
    return TBlackBox::isCustomized() || _m_bAllowPlayer != ~bitset<kNumPlayers>(0) || _m_bAllowComputer
           || !_m_bCancelAfterVisit;
}

void TEvent::write(TRawOStream* pOStream) const
{
#line 123
    assert(pOStream != NULL);
    TBlackBox::write(pOStream);
    unsigned char players = 0;
    for (unsigned int player = 0; player < kNumPlayers; ++player)
        if (_m_bAllowPlayer[player])
            players |= 1 << player;
    *pOStream << players;
    *pOStream << (signed char) _m_bAllowComputer << (signed char) _m_bCancelAfterVisit;
    signed char aReserved[kNumEventReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}
