// BlackBoxPropsGeneralPage.h - the general page of the black box property
// sheet (BlackBoxPropsGeneralPage.cpp; GOG only): the box's message.
// Layout from the image: the message edit at 0x8c, the DDX message at
// 0xc8, then the box, the map's version and the modified flag (0xd8 bytes,
// the sheet's new).
#ifndef HOMM3_EDITOR_BLACKBOXPROPSGENERALPAGE_H
#define HOMM3_EDITOR_BLACKBOXPROPSGENERALPAGE_H

#include <string>

#include "gameversion.h"
#include "va.h"
#include "editor/resource.h"

class TBlackBox;

class TBlackBoxPropsGeneralPage : public CPropertyPage {
public:
    TBlackBoxPropsGeneralPage(TBlackBox* pBlackBox, EGameVersion mapVersion);
    virtual ~TBlackBoxPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }
    VA(0x0040c977, 0x39)
    std::string getMessage() const { return std::string(_m_message); }

    enum { IDD = IDD_BLACK_BOX_PROPS_GENERAL };
    CEdit _m_messageEdit;
    CString _m_message;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    TBlackBox* _m_pBlackBox;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_BLACKBOXPROPSGENERALPAGE_H */
