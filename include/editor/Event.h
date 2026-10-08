// Event.h - a map event (Loki h3maped Event.cpp, object 11): a Pandora's
// box that fires for the players in its mask (+0x50), optionally for the
// computer (+0x54), once or every visit (+0x55).
#ifndef HOMM3_EDITOR_EVENT_H
#define HOMM3_EDITOR_EVENT_H

#include <bitset>

#include "editor/BlackBox.h"
#include "editor/Player.h"

class TEvent : public TBlackBox {
public:
    TEvent(const TObjectType& objType);
    TEvent(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;

    bool getBAllowPlayer(TPlayer player) const;
    void setBAllowPlayer(TPlayer player, bool bAllow);
    bool getBAllowComputer() const { return _m_bAllowComputer; }
    void setBAllowComputer(bool bAllow) { _m_bAllowComputer = bAllow; }
    bool getBCancelAfterVisit() const { return _m_bCancelAfterVisit; }
    void setBCancelAfterVisit(bool bCancel) { _m_bCancelAfterVisit = bCancel; }

private:
    bitset<kNumPlayers> _m_bAllowPlayer;
    bool _m_bAllowComputer;
    bool _m_bCancelAfterVisit;
};

#endif  /* HOMM3_EDITOR_EVENT_H */
