// EventPropsGeneralPage.h - the general page of the event property sheet
// (EventPropsGeneralPage.cpp; GOG only): the event's message, the players
// it fires for and whether the computer triggers it and it cancels after a
// visit. Layout from the image: the message edit at 0x8c, the DDX computer
// and cancel flags at 0xc8 and the message at 0xd0, then the event, the
// players present, the map's version, the modified flag, the allowed
// players at 0xe4 and the player checks at 0xe8 (0x2c8 bytes, the sheet's
// new).
#ifndef HOMM3_EDITOR_EVENTPROPSGENERALPAGE_H
#define HOMM3_EDITOR_EVENTPROPSGENERALPAGE_H

#include <string>

#include "gameversion.h"
#include "va.h"
#include "editor/Player.h"
#include "editor/resource.h"

class TEvent;

class TEventPropsGeneralPage : public CPropertyPage {
public:
    TEventPropsGeneralPage(TEvent* pEvent, const TPlayerMask& playersPresent, EGameVersion mapVersion);
    virtual ~TEventPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }
    VA(0x0041a5ab, 0x39)
    std::string getMessage() const { return std::string(_m_message); }
    bool getBAllowPlayer(TPlayer player) const;
    bool getBAllowComputer() const { return _m_bAllowComputer != FALSE; }
    bool getBCancelAfterVisit() const { return _m_bCancelAfterVisit != FALSE; }

    enum { IDD = IDD_EVENT_PROPS_GENERAL };
    CEdit _m_messageEdit;
    BOOL _m_bAllowComputer;
    BOOL _m_bCancelAfterVisit;
    CString _m_message;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    TEvent* _m_pEvent;
    TPlayerMask _m_playersPresent;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    TPlayerMask _m_bAllowPlayer;
    CButton _m_aPlayerChecks[kNumPlayers];
};

#endif  /* HOMM3_EDITOR_EVENTPROPSGENERALPAGE_H */
