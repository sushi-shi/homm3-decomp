// SpellScrollPropsGeneralPage.h - the general page of the spell scroll
// property sheet (SpellScrollPropsGeneralPage.cpp; GOG only, the artifact
// general page plus a spell combo): the scroll's type name, its spell and
// its pickup message. Layout from the image: the spell combo at 0x8c, the
// message edit at 0xc8, the DDX type name and message at 0x104, then the
// scroll, the map's version, the spell and the modified flag (0x11c
// bytes, the sheet's new).
#ifndef HOMM3_EDITOR_SPELLSCROLLPROPSGENERALPAGE_H
#define HOMM3_EDITOR_SPELLSCROLLPROPSGENERALPAGE_H

#include <string>

#include "armygrp.h"
#include "gameversion.h"
#include "va.h"
#include "editor/resource.h"

class TSpellScroll;

class TSpellScrollPropsGeneralPage : public CPropertyPage {
public:
    TSpellScrollPropsGeneralPage(TSpellScroll* pSpellScroll, EGameVersion mapVersion);
    virtual ~TSpellScrollPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }
    VA(0x004ba08e, 0x39)
    std::string getMessage() const { return std::string(_m_message); }
    ESpellId getSpell() const { return _m_spell; }

    enum { IDD = IDD_SPELL_SCROLL_PROPS_GENERAL };
    CComboBox _m_spellCombo;
    CEdit _m_messageEdit;
    CString _m_typeName;
    CString _m_message;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    TSpellScroll* _m_pSpellScroll;
    EGameVersion _m_mapVersion;
    ESpellId _m_spell;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_SPELLSCROLLPROPSGENERALPAGE_H */
