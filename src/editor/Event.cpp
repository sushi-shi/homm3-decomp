// Event.cpp - the map event (h3maped 0x418c73..0x41948b; Loki h3maped
// object 11): a Pandora's box that fires for the players it allows and may
// cancel itself after a visit. Before map version 6 an event carried its own
// message, resources and one artifact. The release drops Loki's asserts.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "editor/Event.h"
#include "editor/RawStream.h"

// The reserved byte counts of the map format records this file reads and
// writes.
const unsigned int kNumOldEventReserved = 16;
const unsigned int kNumEventReserved = 4;

VA(0x00418e47, 0x7b)
TEvent::TEvent(const TObjectType& objType)
    : TGameObject(objType), TBlackBox(objType), _m_bAllowPlayer(~std::bitset<kNumPlayers>(0)),
      _m_bAllowComputer(false), _m_bCancelAfterVisit(true)
{
}

VA(0x00418ec2, 0x209)
TEvent::TEvent(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TBlackBox(objType, pIStream, version)
{
    if (version < 6) {
        std::string message;
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
            std::vector<TArtifact> aArtifact(1, TArtifact(artifact));
            getPContents()->setArtifacts(aArtifact);
        }
    }
    unsigned char players;
    *pIStream >> players;
    for (unsigned int player = 0; player < kNumPlayers; ++player)
        setBAllowPlayer(TPlayer(player), (players & (1 << player)) != 0);
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

VA(0x00419182, 0x13)
void TEvent::setBAllowPlayer(TPlayer player, bool bAllow)
{
    _m_bAllowPlayer[player] = bAllow;
}

VA(0x00419195, 0x30)
bool TEvent::getBAllowPlayer(TPlayer player) const
{
    return _m_bAllowPlayer.test(player);
}

VA(0x004191c5, 0x58)
bool TEvent::isCustomized() const
{
    return TBlackBox::isCustomized() || _m_bAllowPlayer != ~std::bitset<kNumPlayers>(0) || _m_bAllowComputer
           || !_m_bCancelAfterVisit;
}

VA(0x0041921d, 0xa3)
void TEvent::write(TRawOStream* pOStream, int version) const
{
    TBlackBox::write(pOStream, version);
    unsigned char players = 0;
    for (unsigned int player = 0; player < kNumPlayers; ++player)
        if (_m_bAllowPlayer.test(player))
            players |= 1 << player;
    *pOStream << players;
    *pOStream << static_cast<signed char>(_m_bAllowComputer) << static_cast<signed char>(_m_bCancelAfterVisit);
    signed char aReserved[kNumEventReserved];
    std::fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}
