// HeroPropsBiographyPage.h - the biography page of the hero property
// sheets (GOG HeroPropsBiographyPage.cpp, not in Loki): a customize check
// over the biography edit, which shows the custom biography, or the
// hero's default one while uncustomized. Before Armageddon's Blade a map
// keeps no custom biography. Layout from the image: the check and edit at
// 0x8c, the original and edited heroes, the map's version, the modified
// flag, the default and custom biographies and the custom flag (0x120
// bytes, the sheets' new).
#ifndef HOMM3_EDITOR_HEROPROPSBIOGRAPHYPAGE_H
#define HOMM3_EDITOR_HEROPROPSBIOGRAPHYPAGE_H

#include <string>

#include "gameversion.h"
#include "editor/resource.h"

class THero;

class THeroPropsBiographyPage : public CPropertyPage {
public:
    THeroPropsBiographyPage(const THero* pOldHero, THero* pNewHero, EGameVersion mapVersion);
    virtual ~THeroPropsBiographyPage();

    bool wasModified() const { return _m_bModified; }
    // The biography shown while uncustomized (the hero's identity's).
    void setDefaultBiography(const std::string& biography);

    enum { IDD = IDD_HERO_PROPS_BIOGRAPHY };
    CButton _m_customizeCheck;
    CEdit _m_biographyEdit;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCustomizeCheck();
    DECLARE_MESSAGE_MAP()

private:
    const THero* _m_pOldHero;
    THero* _m_pNewHero;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    CString _m_defaultBiography;
    CString _m_customBiography;
    bool _m_bCustomBiography;
};

#endif  /* HOMM3_EDITOR_HEROPROPSBIOGRAPHYPAGE_H */
