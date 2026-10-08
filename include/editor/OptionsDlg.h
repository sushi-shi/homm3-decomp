// OptionsDlg.h - the options dialog (OptionsDlg.cpp; GOG only): the
// special tile frequency slider, the repaint check and the autosave
// period. Layout from the image: the slider, spin, edit and check controls
// at 0x5c, 0x98, 0xd4 and 0x110, then the DDX repaint flag, the autosave
// period and the frequency.
#ifndef HOMM3_EDITOR_OPTIONSDLG_H
#define HOMM3_EDITOR_OPTIONSDLG_H

#include "editor/resource.h"

class TOptionsDlg : public CDialog {
public:
    TOptionsDlg(CWnd* pParent);

    BOOL getBRepaint() const { return _m_bRepaint; }
    unsigned int getAutosaveMinutes() const { return _m_autosaveMinutes; }
    unsigned int getSpecialTileFrequency() const { return _m_specialTileFrequency; }

    enum { IDD = IDD_OPTIONS };
    CSliderCtrl _m_frequencySlider;
    CSpinButtonCtrl _m_minutesSpin;
    CEdit _m_minutesEdit;
    CButton _m_autosaveCheck;
    BOOL _m_bRepaint;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnAutosaveCheck();
    afx_msg void OnKillFocusMinutesEdit();
    DECLARE_MESSAGE_MAP()

private:
    unsigned int _m_autosaveMinutes;
    unsigned int _m_specialTileFrequency;
};

#endif  /* HOMM3_EDITOR_OPTIONSDLG_H */
