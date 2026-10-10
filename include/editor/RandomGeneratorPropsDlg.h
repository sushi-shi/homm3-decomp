// RandomGeneratorPropsDlg.h - the properties dialogs of the Armageddon's
// Blade random dwellings (RandomGeneratorPropsDlg.cpp; GOG only, Loki's
// port has no counterpart). Each lays the file's owner, level and
// alignment child dialogs over its frames and edits a copy: of the map
// for the dwellings with a random alignment (the alignment may link the
// dwelling to a random town), of the object for the randomly leveled one.
// DoModal assigns the copy back when OK changed something.
//
// Layout from the image: the map or object at 0x5c, its copy at 0x60, the
// old dwelling (map dialogs) at 0x68, the modified flag at 0x6c (0x68 for
// the leveled dwelling) and the child dialogs after it.
#ifndef HOMM3_EDITOR_RANDOMGENERATORPROPSDLG_H
#define HOMM3_EDITOR_RANDOMGENERATORPROPSDLG_H

#include <memory>

#include "editor/resource.h"

class TGameMap;
class TRandomGenerator;
class TRandomlyAlignedGenerator;
class TRandomlyLeveledGenerator;

class TRandomGeneratorPropsDlg : public CDialog {
public:
    TRandomGeneratorPropsDlg(CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual ~TRandomGeneratorPropsDlg();

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RANDOM_GENERATOR_PROPS };

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    struct _TDialogs;

    TGameMap* _m_pMap;
    std::auto_ptr<TGameMap> _m_pNewMap;
    const TRandomGenerator* _m_pOldGenerator;
    bool _m_bModified;
    std::auto_ptr<_TDialogs> _m_pDialogs;
};

class TRandomlyAlignedGeneratorPropsDlg : public CDialog {
public:
    TRandomlyAlignedGeneratorPropsDlg(CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual ~TRandomlyAlignedGeneratorPropsDlg();

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RANDOMLY_ALIGNED_GENERATOR_PROPS };

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    struct _TDialogs;

    TGameMap* _m_pMap;
    std::auto_ptr<TGameMap> _m_pNewMap;
    const TRandomlyAlignedGenerator* _m_pOldGenerator;
    bool _m_bModified;
    std::auto_ptr<_TDialogs> _m_pDialogs;
};

class TRandomlyLeveledGeneratorPropsDlg : public CDialog {
public:
    TRandomlyLeveledGeneratorPropsDlg(CWnd* pParent, TRandomlyLeveledGenerator* pGenerator);
    virtual ~TRandomlyLeveledGeneratorPropsDlg();

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RANDOMLY_LEVELED_GENERATOR_PROPS };

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    struct _TDialogs;

    TRandomlyLeveledGenerator* _m_pGenerator;
    std::auto_ptr<TRandomlyLeveledGenerator> _m_pNewGenerator;
    bool _m_bModified;
    std::auto_ptr<_TDialogs> _m_pDialogs;
};

#endif  /* HOMM3_EDITOR_RANDOMGENERATORPROPSDLG_H */
