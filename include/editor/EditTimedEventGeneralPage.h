// EditTimedEventGeneralPage.h - the general page of the timed and town
// event sheets (EditTimedEventGeneralPage.cpp; Loki h3maped object 80):
// the name, message, players, human and computer flags, first day and
// repeat interval. The sheet implements its parent interface, which the
// page tells when the name turns blank or not. Layout from the image: the
// human check, interval combo, day spin and edit and message edit from
// 0x8c, the DDX name, message and flags at 0x1b8, the parent sheet, event,
// present players, map version and modified flag at 0x1c8, the player
// checks at 0x1dc, then the blank-name flag, players, day and interval
// (0x3cc bytes, the sheets' new).
#ifndef HOMM3_EDITOR_EDITTIMEDEVENTGENERALPAGE_H
#define HOMM3_EDITOR_EDITTIMEDEVENTGENERALPAGE_H

#include <string>

#include "gameversion.h"
#include "editor/Player.h"
#include "editor/resource.h"

class TTimedEvent;

class TEditTimedEventGeneralPageParentSheet {
public:
    virtual void onEnableOK() = 0;
    virtual void onDisableOK() = 0;
};

class TEditTimedEventGeneralPage : public CPropertyPage {
public:
    TEditTimedEventGeneralPage(TEditTimedEventGeneralPageParentSheet* pParentSheet, const TTimedEvent& event,
                               const TPlayerMask& playersPresent, EGameVersion mapVersion);
    virtual ~TEditTimedEventGeneralPage();

    bool wasModified() const { return _m_bModified; }
    std::string getName() const { return std::string(_m_name); }
    std::string getMessage() const { return std::string(_m_message); }
    bool getBApplyToPlayer(TPlayer player) const;
    bool getBApplyToHuman() const { return _m_bApplyToHuman != FALSE; }
    bool getBApplyToComputer() const { return _m_bApplyToComputer != FALSE; }
    unsigned int getFirstOccurence() const { return _m_firstOccurence; }
    unsigned int getSubsequentInterval() const { return _m_subsequentInterval; }

    enum { IDD = IDD_EDIT_TIMED_EVENT_GENERAL };
    CButton _m_humanCheck;
    CComboBox _m_subsequentCombo;
    CSpinButtonCtrl _m_firstOccurenceSpin;
    CEdit _m_firstOccurenceEdit;
    CEdit _m_messageEdit;
    CString _m_name;
    CString _m_message;
    BOOL _m_bApplyToComputer;
    BOOL _m_bApplyToHuman;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnChangeNameEdit();
    afx_msg void OnKillFocusFirstOccurenceEdit();
    afx_msg void OnComputerCheck();
    afx_msg void OnHumanCheck();
    DECLARE_MESSAGE_MAP()

private:
    TEditTimedEventGeneralPageParentSheet* _m_pParentSheet;
    const TTimedEvent& _m_event;
    TPlayerMask _m_playersPresent;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    CButton _m_aPlayerChecks[kNumPlayers];
    bool _m_bBlankName;
    TPlayerMask _m_bApplyToPlayer;
    unsigned int _m_firstOccurence;
    unsigned int _m_subsequentInterval;
};

#endif  /* HOMM3_EDITOR_EDITTIMEDEVENTGENERALPAGE_H */
