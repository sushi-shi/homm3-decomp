// EditTimedEventGeneralPage.h - the general page of the timed event sheet
// (Loki EditTimedEventGeneralPage.cpp): name, message, the players and the
// computer the event applies to, the first day and the repeat interval,
// over the timed_event glade widgets. Layout from the image: the MFC-era
// DDX members (_m_name, _m_message, a BOOL for the computer flag), the
// event, the players present on the map, the modified and blank-name
// flags, the player bits, the days, the interval of each combo entry, then
// the vtable pointer (OnOK, OnInitDialog). Member names other than the
// ones the asserts show are not proven.
#ifndef HOMM3_EDITOR_EDITTIMEDEVENTGENERALPAGE_H
#define HOMM3_EDITOR_EDITTIMEDEVENTGENERALPAGE_H

#include "editor/stdafx.h"

#include <bitset>
#include <string>

#include "editor/Player.h"

class TTimedEvent;

class TEditTimedEventGeneralPage {
public:
    TEditTimedEventGeneralPage(void* pParentSheet, const TTimedEvent& event, const TPlayerMask& playersPresent);
    ~TEditTimedEventGeneralPage();

    void UpdateData(bool bSaveAndValidate);

    bool wasModified() const { return _m_bModified; }
    string getName() const { return _m_name; }
    string getMessage() const { return _m_message; }
    bool getBApplyToPlayer(TPlayer player) const;
    bool getBApplyToComputer() const { return _m_bApplyToComputer != FALSE; }
    unsigned int getFirstOccurence() const { return _m_firstOccurence; }
    unsigned int getSubsequentInterval() const { return _m_subsequentInterval; }

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnChangeEventNameEdit();
    void OnKillFocusFirstOccurenceEdit();

private:
    string _m_name;
    string _m_message;
    BOOL _m_bApplyToComputer;
    const TTimedEvent& _m_event;
    TPlayerMask _m_playersPresent;
    bool _m_bModified;
    bool _m_bBlankName;
    TPlayerMask _m_bApplyToPlayer;
    unsigned int _m_firstOccurence;
    unsigned int _m_subsequentInterval;
    unsigned int _m_aComboIntervals[512];
};

#endif  /* HOMM3_EDITOR_EDITTIMEDEVENTGENERALPAGE_H */
