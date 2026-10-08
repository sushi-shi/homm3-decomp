// Event.cpp - Loki h3maped object 11: the map event, a Pandora's box that
// fires for the players it allows and may cancel itself after a visit.
// Assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <bitset>
#include <string>
#include <vector>

#include "adventureobjecttype.h"
#include "editor/Event.h"
#include "editor/RawStream.h"

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
        signed char aReserved[16];
        *pIStream >> aReserved;
    } else {
        signed char aReserved[4];
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
    signed char aReserved[4];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}
