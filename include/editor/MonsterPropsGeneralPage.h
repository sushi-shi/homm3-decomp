// MonsterPropsGeneralPage.h - the general page of the monster property
// sheet (MonsterPropsGeneralPage.cpp; GOG only): the creature's name, a
// random or a custom quantity, the disposition, the flee and growth flags
// and the message. Layout from the image: the quantity spin at 0x8c, the
// quantity edit at 0xc8, the message edit at 0x104, the DDX type name,
// disposition, message, quantity radio and the two flags from 0x140, then
// the monster, the map's version, the modified flag and the quantity
// (0x168 bytes, the sheet's new).
#ifndef HOMM3_EDITOR_MONSTERPROPSGENERALPAGE_H
#define HOMM3_EDITOR_MONSTERPROPSGENERALPAGE_H

#include <string>

#include "gameversion.h"
#include "va.h"
#include "editor/resource.h"

class TMonster;

class TMonsterPropsGeneralPage : public CPropertyPage {
public:
    TMonsterPropsGeneralPage(TMonster* pMonster, EGameVersion mapVersion);
    virtual ~TMonsterPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }
    unsigned int getQuantity() const { return _m_quantity; }
    int getDisposition() const { return _m_disposition; }
    bool getBNeverFlees() const { return _m_bNeverFlees != FALSE; }
    bool getBNeverGrows() const { return _m_bNeverGrows != FALSE; }
    VA(0x00489e24, 0x39)
    std::string getMessage() const { return std::string(_m_message); }

    enum { IDD = IDD_MONSTER_PROPS_GENERAL };
    CSpinButtonCtrl _m_quantitySpin;
    CEdit _m_quantityEdit;
    CEdit _m_messageEdit;
    CString _m_typeName;
    int _m_disposition;
    CString _m_message;
    int _m_quantityChoice;
    BOOL _m_bNeverFlees;
    BOOL _m_bNeverGrows;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnKillFocusQuantityEdit();
    afx_msg void OnRandomQtyRadio();
    afx_msg void OnCustomQtyRadio();
    DECLARE_MESSAGE_MAP()

private:
    TMonster* _m_pMonster;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    unsigned int _m_quantity;
};

#endif  /* HOMM3_EDITOR_MONSTERPROPSGENERALPAGE_H */
