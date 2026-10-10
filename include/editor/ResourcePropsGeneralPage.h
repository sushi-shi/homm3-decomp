// ResourcePropsGeneralPage.h - the general page of the resource property
// sheet (ResourcePropsGeneralPage.cpp; GOG only): the resource's type
// name, a random or a custom quantity and its pickup message. Layout from
// the image: the quantity spin at 0x8c, the quantity edit at 0xc8, the
// message edit at 0x104, the DDX message at 0x140, the DDX radio index
// (0 random, 1 custom) at 0x144, the DDX type name at 0x148, then the
// resource, the map's version, the quantity and the modified flag (0x15c
// bytes, the sheet's new).
#ifndef HOMM3_EDITOR_RESOURCEPROPSGENERALPAGE_H
#define HOMM3_EDITOR_RESOURCEPROPSGENERALPAGE_H

#include <string>

#include "gameversion.h"
#include "va.h"
#include "editor/resource.h"

class TGameResource;

class TResourcePropsGeneralPage : public CPropertyPage {
public:
    TResourcePropsGeneralPage(TGameResource* pResource, EGameVersion mapVersion);
    virtual ~TResourcePropsGeneralPage();

    bool wasModified() const { return _m_bModified; }
    unsigned int getQuantity() const { return _m_quantity; }
    VA(0x004b339b, 0x39)
    std::string getMessage() const { return std::string(_m_message); }

    enum { IDD = IDD_RESOURCE_PROPS_GENERAL };
    CSpinButtonCtrl _m_quantitySpin;
    CEdit _m_quantityEdit;
    CEdit _m_messageEdit;
    CString _m_message;
    int _m_quantityChoice;
    CString _m_typeName;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnKillFocusQuantityEdit();
    afx_msg void OnRandomQtyRadio();
    afx_msg void OnCustomQtyRadio();
    DECLARE_MESSAGE_MAP()

private:
    TGameResource* _m_pResource;
    EGameVersion _m_mapVersion;
    unsigned int _m_quantity;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_RESOURCEPROPSGENERALPAGE_H */
