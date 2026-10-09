// Event.h - a map event: a Pandora's Box that fires for the players it
// names (C:\Dev\Heroes 3 Exp 2\Editor\Event.cpp, by its RTTI; Loki h3maped
// object 11).
//
// RTTI TEvent <- TBlackBox <- TTreasure <- virtual TGameObject. After the
// box's contents (+0x50): the players it fires for (+0x54), whether the
// computer may trigger it (+0x58) and whether it cancels after a visit
// (+0x59).
#ifndef HOMM3_EDITOR_EVENT_H
#define HOMM3_EDITOR_EVENT_H

#include <bitset>

#include "editor/BlackBox.h"
#include "editor/Player.h"

class TEvent : public TBlackBox {
public:
    TEvent(const TObjectType& objType);
    TEvent(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;

    void setBAllowPlayer(TPlayer player, bool bAllow);
    void setBAllowComputer(bool bAllow) { _m_bAllowComputer = bAllow; }
    void setBCancelAfterVisit(bool bCancel) { _m_bCancelAfterVisit = bCancel; }
    bool getBAllowPlayer(TPlayer player) const;
    bool getBAllowComputer() const { return _m_bAllowComputer; }
    bool getBCancelAfterVisit() const { return _m_bCancelAfterVisit; }

private:
    std::bitset<kNumPlayers> _m_bAllowPlayer;
    bool _m_bAllowComputer;
    bool _m_bCancelAfterVisit;
};

#endif  /* HOMM3_EDITOR_EVENT_H */
