// MonsterPropsSheet.h - the monster property sheet (MonsterPropsSheet.cpp;
// GOG only): the general and treasure pages over the monster itself,
// which DoModal updates from the pages that changed. Layout from the
// image: the monster at 0x88 and the two pages' auto_ptrs (0xa0 bytes).
#ifndef HOMM3_EDITOR_MONSTERPROPSSHEET_H
#define HOMM3_EDITOR_MONSTERPROPSSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/MonsterPropsGeneralPage.h"
#include "editor/MonsterPropsTreasurePage.h"

class TMonster;

class TMonsterPropsSheet : public CPropertySheet {
public:
    TMonsterPropsSheet(CWnd* pParentWnd, TMonster* pMonster, EGameVersion mapVersion);
    virtual ~TMonsterPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

protected:
    DECLARE_MESSAGE_MAP()

private:
    TMonster* _m_pMonster;
    std::auto_ptr<TMonsterPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TMonsterPropsTreasurePage> _m_pTreasurePage;
};

#endif  /* HOMM3_EDITOR_MONSTERPROPSSHEET_H */
