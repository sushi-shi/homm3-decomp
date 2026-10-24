// QuestLocationPropsDlg.cpp - the properties dialog a Seer's Hut and a
// Quest Guard share (h3maped 0x49717b..0x499bda; GOG only). The quest's
// description and its default messages are visitors of the quest: the
// description names the target and its location, the messages fill
// seerhut.txt's texts of the quest's kind.
#include "editor/stdafx.h"

#include <stdio.h>

#include <string>

#include "adventureobjecttype.h"
#include "artifact.h"
#include "exceptions.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "retailobjecttype.h"
#include "textresource.h"
#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/EditQuestDlg.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/MapObjectRef.h"
#include "editor/Monster.h"
#include "editor/Player.h"
#include "editor/Quest.h"
#include "editor/QuestLocationPropsDlg.h"
#include "editor/StringUtil.h"
#include "editor/Town.h"
#include "editor/WorldDirections.h"

// seerhut.txt's texts: two heading rows, then five rows per quest kind,
// the proposal, progress and completion messages first; one column per
// quest location kind.
enum TQuestTexts {
    kExperienceLevelQuestTexts,
    kPrimarySkillLevelQuestTexts,
    kDefeatHeroQuestTexts,
    kDefeatMonsterQuestTexts,
    kBringArtifactsQuestTexts,
    kBringCreaturesQuestTexts,
    kBringResourcesQuestTexts,
    kBeASpecificHeroQuestTexts,
    kBelongToASpecificPlayerQuestTexts
};

enum { kFirstQuestTextRow = 2, kNumQuestTextRows = 5 };

namespace {

// One line of the description: what the quest asks for.
class TGetQuestDescFunc : public TQuest::TVisitor {
public:
    TGetQuestDescFunc(const TGameMap* pMap) : _m_pMap(pMap) {}

    CString operator()(const TQuest* pQuest);

    virtual void visit(const TQuestAchieveExperienceLevel& quest);
    virtual void visit(const TQuestAchievePrimarySkillLevel& quest);
    virtual void visit(const TQuestDefeatHero& quest);
    virtual void visit(const TQuestDefeatMonster& quest);
    virtual void visit(const TQuestBringArtifacts& quest);
    virtual void visit(const TQuestBringCreatures& quest);
    virtual void visit(const TQuestBringResources& quest);
    virtual void visit(const TQuestBeASpecificHero& quest);
    virtual void visit(const TQuestBelongToASpecificPlayer& quest);

private:
    const TGameMap* _m_pMap;
    CString _m_desc;
};

// A message of the quest as the game words it when not customized.
class TGetDefaultQuestMessageFunc : public TQuest::TVisitor {
public:
    TGetDefaultQuestMessageFunc() : _m_pMap(NULL) {}

    CString operator()(const TQuest& quest, const TGameMap* pMap, unsigned int message, int column);

    virtual void visit(const TQuestAchieveExperienceLevel& quest);
    virtual void visit(const TQuestAchievePrimarySkillLevel& quest);
    virtual void visit(const TQuestDefeatHero& quest);
    virtual void visit(const TQuestDefeatMonster& quest);
    virtual void visit(const TQuestBringArtifacts& quest);
    virtual void visit(const TQuestBringCreatures& quest);
    virtual void visit(const TQuestBringResources& quest);
    virtual void visit(const TQuestBeASpecificHero& quest);
    virtual void visit(const TQuestBelongToASpecificPlayer& quest);

private:
    const TGameMap* _m_pMap;
    unsigned int _m_message;
    int _m_column;
    CString _m_text;
};

const char* getQuestText(TQuestTexts texts, unsigned int message, int column);

}

VA(0x00497382, 0x94)
CString TGetQuestDescFunc::operator()(const TQuest* pQuest)
{
    if (pQuest != NULL)
        pQuest->accept(*this);
    else
        _m_desc = kNoQuestStr;
    CString desc(_m_desc);
    _m_desc = CString();
    return desc;
}

VA(0x00497416, 0x1d)
void TGetQuestDescFunc::visit(const TQuestAchieveExperienceLevel& quest)
{
    _m_desc.Format(kQuestExperienceLevelFmtStr, quest.getLevel());
}

VA(0x00497433, 0xfc)
void TGetQuestDescFunc::visit(const TQuestAchievePrimarySkillLevel& quest)
{
    const TArray<int, kNumPrimarySkills>& aLevel = quest.getLevels();
    unsigned int numSkills = 0;
    unsigned int skill;
    for (skill = 0; skill < kNumPrimarySkills; skill++) {
        if (aLevel[skill] > 0)
            numSkills++;
    }
    if (numSkills == 1) {
        for (skill = 0; aLevel[skill] <= 0; skill++)
            ;
        TFormattedString level(kQuestPrimarySkillLevelFmtStr, THero::s_akPrimarySkillTraits[skill].m_name,
                               aLevel[skill]);
        _m_desc.Format(kQuestAchieveFmtStr, (LPCTSTR)level);
    } else {
        _m_desc = kQuestAchieveTheFollowingStr;
        for (skill = 0; skill < kNumPrimarySkills; skill++) {
            if (aLevel[skill] > 0) {
                TFormattedString line(kQuestPrimarySkillLevelFmtStr, THero::s_akPrimarySkillTraits[skill].m_name,
                                      aLevel[skill]);
                _m_desc += "\r\n\t";
                _m_desc += line;
            }
        }
    }
}

VA(0x0049752f, 0x1e8)
void TGetQuestDescFunc::visit(const TQuestDefeatHero& quest)
{
    TMapObjectRef heroRef = _m_pMap->getLinkableObjectRef(quest.getHeroLinkID());
    const TGameMap::TLayer& layer = _m_pMap->getLayer(heroRef.getBSecondLayer());
    const TGameObject* pObject = layer.getPObject(heroRef.getObjectID());
    TTilePoint loc = layer.getObjectLoc(heroRef.getObjectID());
    const THero* pHero = dynamic_cast<const THero*>(pObject);
    if (pHero == NULL) {
        const TTown* pTown = dynamic_cast<const TTown*>(pObject);
        pHero = pTown->getPVisitingHero();
        loc -= pTown->getTriggerLoc();
    } else
        loc -= pHero->getTriggerLoc();
    CString heroName;
    const TIdentifiedHero* pIdentifiedHero = dynamic_cast<const TIdentifiedHero*>(pHero);
    if (pIdentifiedHero != NULL) {
        const std::string& name = pIdentifiedHero->getBCustomName()
                                      ? pIdentifiedHero->getName()
                                      : _m_pMap->getHeroPrototype(pIdentifiedHero->getHeroID()).getName();
        heroName.Format(kSpecificHeroAndClassFmtStr, name.c_str(),
                        THero::s_akClassTraits[pIdentifiedHero->getHeroClass()].m_name);
    } else if (pHero->getBCustomName())
        heroName.Format(kSpecificHeroAndClassFmtStr, pHero->getName().c_str(),
                        akAdvObjectTypeTraits[RANDOM_HERO].m_name);
    else
        heroName = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
    TFormattedString location(kObjectAtLocationFmtStr, (LPCTSTR)heroName, loc.x(), loc.y(),
                              heroRef.getBSecondLayer());
    _m_desc.Format(kQuestDefeatHeroFmtStr, (LPCTSTR)location);
}

VA(0x00497717, 0x13c)
void TGetQuestDescFunc::visit(const TQuestDefeatMonster& quest)
{
    TMapObjectRef monsterRef = _m_pMap->getLinkableObjectRef(quest.getMonsterLinkID());
    const TGameMap::TLayer& layer = _m_pMap->getLayer(monsterRef.getBSecondLayer());
    const TMonster* pMonster = dynamic_cast<const TMonster*>(layer.getPObject(monsterRef.getObjectID()));
    TTilePoint loc = layer.getObjectLoc(monsterRef.getObjectID()) - pMonster->getTriggerLoc();
    const char* monsterName;
    if (pMonster->getType() == MONSTER)
        monsterName = getArmyName(pMonster->getCreatureType(), pMonster->getQuantity());
    else
        monsterName = pMonster->getQuantity() == 1 ? kRandomMonsterStr : kRandomMonstersStr;
    TFormattedString location(kObjectAtLocationFmtStr, monsterName, loc.x(), loc.y(), monsterRef.getBSecondLayer());
    _m_desc.Format(kQuestDefeatMonsterFmtStr, (LPCTSTR)location);
}

VA(0x00497853, 0x8f)
void TGetQuestDescFunc::visit(const TQuestBringArtifacts& quest)
{
    const std::multiset<TArtifact>& artifacts = quest.getArtifacts();
    if (artifacts.size() == 1)
        _m_desc.Format(kQuestReturnWithArtifactFmtStr, akArtifactTraits[*artifacts.begin()].m_name);
    else {
        _m_desc = kQuestReturnWithTheFollowingStr;
        for (std::multiset<TArtifact>::const_iterator pArtifact = artifacts.begin(); pArtifact != artifacts.end();
             ++pArtifact) {
            _m_desc += "\r\n\t";
            _m_desc += akArtifactTraits[*pArtifact].m_name;
        }
    }
}

VA(0x004978e2, 0x14c)
void TGetQuestDescFunc::visit(const TQuestBringCreatures& quest)
{
    const TQuestBringCreatures::TCreatures& creatures = quest.getCreatures();
    if (creatures.size() == 1) {
        TFormattedString creature("%d %s", creatures.begin()->second,
                                  getArmyName(creatures.begin()->first, creatures.begin()->second));
        _m_desc.Format(kQuestReturnWithCreaturesFmtStr, (LPCTSTR)creature);
    } else {
        _m_desc = kQuestReturnWithTheFollowingStr;
        for (TQuestBringCreatures::TCreatures::const_iterator pCreature = creatures.begin();
             pCreature != creatures.end(); ++pCreature) {
            TFormattedString line("%d %s", pCreature->second, getArmyName(pCreature->first, pCreature->second));
            _m_desc += "\r\n\t";
            _m_desc += line;
        }
    }
}

VA(0x00497a2e, 0xf9)
void TGetQuestDescFunc::visit(const TQuestBringResources& quest)
{
    const TArray<int, kNumGameResourceTypes>& aQuantity = quest.getQuantities();
    unsigned int numTypes = 0;
    unsigned int type;
    for (type = 0; type < kNumGameResourceTypes; type++) {
        if (unsigned(aQuantity[type]) > 0)
            numTypes++;
    }
    if (numTypes == 1) {
        for (type = 0; unsigned(aQuantity[type]) <= 0; type++)
            ;
        TFormattedString resource("%d %s", aQuantity[type], akGameResourceTypeTraits[type].m_name);
        _m_desc.Format(kQuestReturnWithResourcesFmtStr, (LPCTSTR)resource);
    } else {
        _m_desc = kQuestReturnWithTheFollowingStr;
        for (type = 0; type < kNumGameResourceTypes; type++) {
            if (unsigned(aQuantity[type]) > 0) {
                TFormattedString line("%d %s", aQuantity[type], akGameResourceTypeTraits[type].m_name);
                _m_desc += "\r\n\t";
                _m_desc += line;
            }
        }
    }
}

VA(0x00497b27, 0x91)
void TGetQuestDescFunc::visit(const TQuestBeASpecificHero& quest)
{
    THeroID heroID = quest.getHeroID();
    THeroClass heroClass = THero::s_akTraits[heroID].m_class;
    const char* className = THero::s_akClassTraits[heroClass].m_name;
    _m_desc.Format(SSeersHutPropsDlgText::kBeHeroFmtStr,
                   (LPCTSTR)TFormattedString(kSpecificHeroAndClassFmtStr,
                                             _m_pMap->getHeroPrototype(heroID).getName().c_str(), className));
}

VA(0x00497bb8, 0x26)
void TGetQuestDescFunc::visit(const TQuestBelongToASpecificPlayer& quest)
{
    _m_desc.Format(SSeersHutPropsDlgText::kBelongToPlayerFmtStr, akPlayerTraits[quest.getPlayer()].m_pName);
}

namespace {

VA(0x00497bde, 0xbb)
const char* getQuestText(TQuestTexts texts, unsigned int message, int column)
{
    DATA_COMPGEN_GUARD(0x005a2620, questTextsGuard, pQuestTexts)
    VA_COMPGEN(0x00497c99, 0xa, STATIC_DTOR, pQuestTexts)
    DATA(0x005a2618) static TResourcePtr<TSpreadsheetResource> pQuestTexts;
    if (pQuestTexts.get() == NULL) {
        pQuestTexts = TResourcePtr<TSpreadsheetResource>(ResourceManager::GetSpreadsheet("SeerHut.txt"));
        if (pQuestTexts.get() == NULL)
            throw TRuntimeError();
    }
    return pQuestTexts->GetSpreadsheet(kFirstQuestTextRow + texts * kNumQuestTextRows + message, column);
}

}

VA(0x00497ca3, 0x94)
CString TGetDefaultQuestMessageFunc::operator()(const TQuest& quest, const TGameMap* pMap, unsigned int message,
                                                int column)
{
    _m_pMap = pMap;
    _m_message = message;
    _m_column = column;
    quest.accept(*this);
    _m_pMap = NULL;
    CString text(_m_text);
    _m_text = CString();
    return text;
}

VA(0x00497d37, 0x2b)
void TGetDefaultQuestMessageFunc::visit(const TQuestAchieveExperienceLevel& quest)
{
    _m_text.Format(getQuestText(kExperienceLevelQuestTexts, _m_message, _m_column), quest.getLevel());
}

VA(0x00497d62, 0x110)
void TGetDefaultQuestMessageFunc::visit(const TQuestAchievePrimarySkillLevel& quest)
{
    unsigned int numSkills = 0;
    unsigned int skill;
    for (skill = 0; skill < kNumPrimarySkills; skill++) {
        if (quest.getLevel(TPrimarySkill(skill)) > 0)
            numSkills++;
    }
    CString skills;
    skill = 0;
    while (quest.getLevel(TPrimarySkill(skill)) <= 0)
        skill++;
    for (;;) {
        skills += TFormattedString("%s %d", THero::s_akPrimarySkillTraits[skill].m_name,
                                   quest.getLevel(TPrimarySkill(skill)));
        if (--numSkills == 0)
            break;
        if (numSkills == 1) {
            skills += ' ';
            skills += kAndStr;
            skills += ' ';
        } else
            skills += ", ";
        while (quest.getLevel(TPrimarySkill(++skill)) <= 0)
            ;
    }
    _m_text.Format(getQuestText(kPrimarySkillLevelQuestTexts, _m_message, _m_column), (LPCTSTR)skills);
}

VA(0x00497e72, 0xf0)
void TGetDefaultQuestMessageFunc::visit(const TQuestDefeatHero& quest)
{
    CString heroName;
    const TGameObject* pObject = _m_pMap->getPLinkableObject(quest.getHeroLinkID());
    if (dynamic_cast<const TRandomHero*>(pObject) != NULL)
        heroName = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
    else {
        const TIdentifiedHero* pHero = dynamic_cast<const TIdentifiedHero*>(pObject);
        heroName = pHero->getBCustomName() ? pHero->getName().c_str()
                                           : _m_pMap->getHeroPrototype(pHero->getHeroID()).getName().c_str();
    }
    _m_text.Format(getQuestText(kDefeatHeroQuestTexts, _m_message, _m_column), (LPCTSTR)heroName);
}

VA(0x00497f62, 0x16e)
void TGetDefaultQuestMessageFunc::visit(const TQuestDefeatMonster& quest)
{
    TMapObjectRef monsterRef = _m_pMap->getLinkableObjectRef(quest.getMonsterLinkID());
    const TGameMap::TLayer& layer = _m_pMap->getLayer(monsterRef.getBSecondLayer());
    CString monsterName;
    const TMonster* pMonster = dynamic_cast<const TMonster*>(layer.getPObject(monsterRef.getObjectID()));
    if (pMonster->getType() == MONSTER)
        monsterName = getArmyName(pMonster->getCreatureType(), 0);
    else
        monsterName = akAdvObjectTypeTraits[pMonster->getType()].m_name;
    TTilePoint loc = layer.getObjectLoc(monsterRef.getObjectID());
    CString location(
        akWorldDirectionNames[getWorldDirection(loc, TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight()))]);
    if (monsterRef.getBSecondLayer()) {
        location += ' ';
        location += kUndergroundStr;
    }
    _m_text.Format(getQuestText(kDefeatMonsterQuestTexts, _m_message, _m_column), (LPCTSTR)monsterName,
                   (LPCTSTR)location);
}

VA(0x004980d0, 0xea)
void TGetDefaultQuestMessageFunc::visit(const TQuestBringArtifacts& quest)
{
    std::multiset<TArtifact>::const_iterator pArtifact = quest.getArtifacts().begin();
    CString artifacts(akArtifactTraits[*pArtifact++].m_name);
    while (pArtifact != quest.getArtifacts().end()) {
        TArtifact artifact = *pArtifact;
        if (++pArtifact == quest.getArtifacts().end()) {
            artifacts += ' ';
            artifacts += kAndStr;
            artifacts += ' ';
        } else
            artifacts += ", ";
        artifacts += akArtifactTraits[artifact].m_name;
    }
    _m_text.Format(getQuestText(kBringArtifactsQuestTexts, _m_message, _m_column), (LPCTSTR)artifacts);
}

VA(0x004981ba, 0x18c)
void TGetDefaultQuestMessageFunc::visit(const TQuestBringCreatures& quest)
{
    TQuestBringCreatures::TCreatures::const_iterator pCreature = quest.getCreatures().begin();
    CString creatures = TFormattedString("%d %s", pCreature->second, getArmyName(pCreature->first, pCreature->second));
    ++pCreature;
    while (pCreature != quest.getCreatures().end()) {
        TCreatureType type = pCreature->first;
        int quantity = pCreature->second;
        if (++pCreature == quest.getCreatures().end()) {
            creatures += ' ';
            creatures += kAndStr;
            creatures += ' ';
        } else
            creatures += ", ";
        creatures += TFormattedString("%d %s", quantity, getArmyName(type, quantity));
    }
    _m_text.Format(getQuestText(kBringCreaturesQuestTexts, _m_message, _m_column), (LPCTSTR)creatures);
}

VA(0x00498346, 0x10a)
void TGetDefaultQuestMessageFunc::visit(const TQuestBringResources& quest)
{
    const TArray<int, kNumGameResourceTypes>& aQuantity = quest.getQuantities();
    unsigned int numTypes = 0;
    unsigned int type;
    for (type = 0; type < kNumGameResourceTypes; type++) {
        if (unsigned(aQuantity[type]) > 0)
            numTypes++;
    }
    CString resources;
    type = 0;
    while (unsigned(aQuantity[type]) <= 0)
        type++;
    for (;;) {
        resources += TFormattedString("%d %s", aQuantity[type], akGameResourceTypeTraits[type].m_name);
        if (--numTypes == 0)
            break;
        if (numTypes == 1) {
            resources += ' ';
            resources += kAndStr;
            resources += ' ';
        } else
            resources += ", ";
        while (unsigned(aQuantity[++type]) <= 0)
            ;
    }
    _m_text.Format(getQuestText(kBringResourcesQuestTexts, _m_message, _m_column), (LPCTSTR)resources);
}

VA(0x00498450, 0x45)
void TGetDefaultQuestMessageFunc::visit(const TQuestBeASpecificHero& quest)
{
    _m_text.Format(getQuestText(kBeASpecificHeroQuestTexts, _m_message, _m_column),
                   _m_pMap->getHeroPrototype(quest.getHeroID()).getName().c_str());
}

VA(0x00498495, 0x35)
void TGetDefaultQuestMessageFunc::visit(const TQuestBelongToASpecificPlayer& quest)
{
    _m_text.Format(getQuestText(kBelongToASpecificPlayerQuestTexts, _m_message, _m_column),
                   akPlayerTraits[quest.getPlayer()].m_pColorName);
}

VA(0x004984ca, 0x6)
BEGIN_MESSAGE_MAP(TQuestLocationPropsDlg, CDialog)
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_EDIT_QUEST_BUTTON, OnEditQuestButton)
    ON_BN_CLICKED(IDC_DEADLINE_CHECK, OnDeadlineCheck)
    ON_EN_KILLFOCUS(IDC_MONTH_EDIT, OnKillFocusMonthEdit)
    ON_EN_KILLFOCUS(IDC_WEEK_EDIT, OnKillFocusWeekEdit)
    ON_EN_KILLFOCUS(IDC_DAY_EDIT, OnKillFocusDayEdit)
    ON_NOTIFY(TCN_SELCHANGE, IDC_MESSAGE_TAB, OnSelChangeMessageTab)
    ON_BN_CLICKED(IDC_CUSTOMIZE_MESSAGE_CHECK, OnCustomizeMessageCheck)
    ON_EN_CHANGE(IDC_MESSAGE_EDIT, OnChangeMessageEdit)
END_MESSAGE_MAP()

VA(0x004984d0, 0x1df)
TQuestLocationPropsDlg::TQuestLocationPropsDlg(UINT nIDTemplate, CWnd* pParent, TGameMap* pMap, bool bSecondLayer,
                                               unsigned int objID, int textColumn)
    : CDialog(nIDTemplate, pParent),
      _m_pMap(pMap),
      _m_bSecondLayer(bSecondLayer),
      _m_objID(objID),
      _m_textColumn(textColumn),
      _m_bModified(false),
      _m_pOldLocation(_getOldLocation()),
      _m_pNewMap(NULL),
      _m_pNewLocation(NULL)
{
}

VA_COMPGEN(0x004986af, 0x5, IMPLICIT_DTOR, _TMessage)

VA(0x004986b4, 0x138)
TQuestLocationPropsDlg::~TQuestLocationPropsDlg()
{
}

VA(0x004987ec, 0x39)
const TQuestLocation* TQuestLocationPropsDlg::_getOldLocation() const
{
    const TGameMap* pMap = _m_pMap;
    return dynamic_cast<const TQuestLocation*>(pMap->getLayer(_m_bSecondLayer).getPObject(_m_objID));
}

VA(0x00498825, 0x42)
void TQuestLocationPropsDlg::_setNewMap(TGameMap* pNewMap)
{
    _m_pNewMap = pNewMap;
    _m_pNewLocation = dynamic_cast<TQuestLocation*>(pNewMap->getLayer(_m_bSecondLayer).getPObject(_m_objID));
}

VA(0x00498867, 0x237)
void TQuestLocationPropsDlg::_onQuestChanged(const TQuest* pNewQuest)
{
    if (pNewQuest == NULL) {
        if (_m_deadlineCheck.GetCheck()) {
            _m_deadlineCheck.SetCheck(0);
            OnDeadlineCheck();
        }
        _m_deadlineCheck.EnableWindow(FALSE);
        for (unsigned int i = 0; i < TQuestLocation::s_kNumMessages; i++) {
            _TMessage& message = _m_aMessage[i];
            if (message.m_bCustomized && message.m_bBlank)
                _removeBlankMessage();
            message.m_text = CString();
            message.m_bCustomized = false;
            message.m_bBlank = false;
        }
        _m_messageTab.SetCurSel(0);
        TCITEM item;
        item.mask = TCIF_PARAM;
        _m_messageTab.GetItem(0, &item);
        _m_curMessage = item.lParam;
        _m_messageTab.EnableWindow(FALSE);
        _m_customizeMessageCheck.SetCheck(0);
        _m_customizeMessageCheck.EnableWindow(FALSE);
        _m_messageEdit.SetWindowText("");
        _m_messageEdit.EnableWindow(FALSE);
    } else if (_m_pQuest.get() != NULL) {
        for (unsigned int i = 0; i < TQuestLocation::s_kNumMessages; i++) {
            if (!_m_aMessage[i].m_bCustomized)
                _m_aMessage[i].m_text = _getDefaultMessage(pNewQuest, i);
        }
        if (!_m_aMessage[_m_curMessage].m_bCustomized)
            _m_messageEdit.SetWindowText(_m_aMessage[_m_curMessage].m_text);
    } else {
        if (_m_pNewMap->getVersion() >= GAME_VERSION_AB)
            _m_deadlineCheck.EnableWindow(TRUE);
        for (unsigned int i = 0; i < TQuestLocation::s_kNumMessages; i++)
            _m_aMessage[i].m_text = _getDefaultMessage(pNewQuest, i);
        _m_messageTab.EnableWindow(TRUE);
        if (_m_pNewMap->getVersion() >= GAME_VERSION_AB)
            _m_customizeMessageCheck.EnableWindow(TRUE);
        _m_messageEdit.SetWindowText(_m_aMessage[_m_curMessage].m_text);
    }
}

VA(0x00498a9e, 0x24)
void TQuestLocationPropsDlg::_addBlankMessage()
{
    if (_m_numBlankMessages++ == 0)
        _m_okButton.EnableWindow(FALSE);
}

VA(0x00498ac2, 0x16)
void TQuestLocationPropsDlg::_removeBlankMessage()
{
    if (--_m_numBlankMessages == 0)
        _m_okButton.EnableWindow(TRUE);
}

VA(0x00498ad8, 0x63)
CString TQuestLocationPropsDlg::_getDefaultMessage(const TQuest* pQuest, unsigned int message) const
{
    return TGetDefaultQuestMessageFunc()(*pQuest, _m_pNewMap, message, _m_textColumn);
}

VA_COMPGEN(0x00498b3b, 0x8, IMPLICIT_DTOR, TGetDefaultQuestMessageFunc)

VA(0x00498b43, 0x90)
int TQuestLocationPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(TFormattedString(kObjectPropertiesCaptionFmtStr, _m_pNewLocation->getTypeName().c_str()));
    return 0;
}

VA(0x00498bd3, 0x118)
void TQuestLocationPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_QUEST_EDIT, _m_questEdit);
    DDX_Control(pDX, IDC_DEADLINE_CHECK, _m_deadlineCheck);
    DDX_Control(pDX, IDC_MONTH_STATIC, _m_monthStatic);
    DDX_Control(pDX, IDC_MONTH_EDIT, _m_monthEdit);
    DDX_Control(pDX, IDC_MONTH_SPIN, _m_monthSpin);
    DDX_Control(pDX, IDC_WEEK_STATIC, _m_weekStatic);
    DDX_Control(pDX, IDC_WEEK_EDIT, _m_weekEdit);
    DDX_Control(pDX, IDC_WEEK_SPIN, _m_weekSpin);
    DDX_Control(pDX, IDC_DAY_STATIC, _m_dayStatic);
    DDX_Control(pDX, IDC_DAY_EDIT, _m_dayEdit);
    DDX_Control(pDX, IDC_DAY_SPIN, _m_daySpin);
    DDX_Control(pDX, IDC_MESSAGE_TAB, _m_messageTab);
    DDX_Control(pDX, IDC_CUSTOMIZE_MESSAGE_CHECK, _m_customizeMessageCheck);
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Control(pDX, IDOK, _m_okButton);
}

VA(0x00498ceb, 0x57c)
BOOL TQuestLocationPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_QUEST_GROUP)->SetWindowText(SSeersHutPropsDlgText::kQuestStaticStr);
    GetDlgItem(IDC_EDIT_QUEST_BUTTON)->SetWindowText(SSeersHutPropsDlgText::kEditQuestButtonStr);
    GetDlgItem(IDC_DEADLINE_CHECK)->SetWindowText(SSeersHutPropsDlgText::kHasDeadlineCheckStr);
    GetDlgItem(IDC_MONTH_STATIC)->SetWindowText(SSeersHutPropsDlgText::kMonthStaticStr);
    GetDlgItem(IDC_WEEK_STATIC)->SetWindowText(SSeersHutPropsDlgText::kWeekStaticStr);
    GetDlgItem(IDC_DAY_STATIC)->SetWindowText(SSeersHutPropsDlgText::kDayStaticStr);
    GetDlgItem(IDC_CUSTOMIZE_MESSAGE_CHECK)->SetWindowText(SSeersHutPropsDlgText::kCustomizeCheckStr);
    _m_bModified = false;
    _m_numBlankMessages = 0;
    if (_m_pNewLocation->getPQuest() != NULL) {
        _m_pQuest = _m_pNewLocation->getPQuest()->clone();
        if (_m_pQuest.get() == NULL)
            throw TAllocationFailure();
    }
    unsigned int i;
    for (i = 0; i < TQuestLocation::s_kNumMessages; i++) {
        if (_m_pQuest.get() != NULL) {
            const std::string& message = _m_pNewLocation->getMessage(i);
            if (message.empty()) {
                _m_aMessage[i].m_text = _getDefaultMessage(_m_pQuest.get(), i);
                _m_aMessage[i].m_bCustomized = false;
            } else {
                _m_aMessage[i].m_text = message.c_str();
                _m_aMessage[i].m_text.Replace("\n", "\r\n");
                _m_aMessage[i].m_bCustomized = true;
            }
        } else
            _m_aMessage[i].m_bCustomized = false;
        _m_aMessage[i].m_bBlank = false;
    }
    CDialog::OnInitDialog();
    TGetQuestDescFunc getQuestDesc(_m_pNewMap);
    _m_questEdit.SetWindowText(getQuestDesc(_m_pQuest.get()));
    _m_monthEdit.LimitText(TDigits<99>::getDigits());
    _m_monthSpin.SetRange(1, 99);
    _m_weekEdit.LimitText(TDigits<4>::getDigits());
    _m_weekSpin.SetRange(1, 4);
    _m_dayEdit.LimitText(TDigits<7>::getDigits());
    _m_daySpin.SetRange(1, 7);
    unsigned int deadline = _m_pNewLocation->getDeadline();
    if (deadline != -1) {
        _m_deadlineCheck.SetCheck(1);
        unsigned int day = deadline % 7 + 1;
        unsigned int week = deadline / 7 % 4 + 1;
        unsigned int month = deadline / 28 + 1;
        _m_monthEdit.SetWindowText(TFormattedString("%d", month));
        _m_weekEdit.SetWindowText(TFormattedString("%d", week));
        _m_dayEdit.SetWindowText(TFormattedString("%d", day));
    } else {
        if (_m_pQuest.get() == NULL || _m_pNewMap->getVersion() < GAME_VERSION_AB)
            _m_deadlineCheck.EnableWindow(FALSE);
        _m_monthStatic.EnableWindow(FALSE);
        _m_monthEdit.EnableWindow(FALSE);
        _m_monthSpin.EnableWindow(FALSE);
        _m_weekStatic.EnableWindow(FALSE);
        _m_weekEdit.EnableWindow(FALSE);
        _m_weekSpin.EnableWindow(FALSE);
        _m_dayStatic.EnableWindow(FALSE);
        _m_dayEdit.EnableWindow(FALSE);
        _m_daySpin.EnableWindow(FALSE);
    }
    const char* const aMessageName[TQuestLocation::s_kNumMessages] = {
        kProposalMessageStr,
        kProgressMessageStr,
        kCompletionMessageStr
    };
    for (i = 0; i < TQuestLocation::s_kNumMessages; i++)
        _m_messageTab.InsertItem(TCIF_TEXT | TCIF_PARAM, _m_messageTab.GetItemCount(), aMessageName[i], 0, i);
    _m_messageTab.SetCurSel(0);
    TCITEM item;
    item.mask = TCIF_PARAM;
    _m_messageTab.GetItem(0, &item);
    _m_curMessage = item.lParam;
    if (_m_pQuest.get() != NULL) {
        if (_m_pNewMap->getVersion() >= GAME_VERSION_AB) {
            _m_customizeMessageCheck.SetCheck(_m_aMessage[_m_curMessage].m_bCustomized);
            _m_messageEdit.EnableWindow(_m_aMessage[_m_curMessage].m_bCustomized);
        } else {
            _m_customizeMessageCheck.EnableWindow(FALSE);
            _m_messageEdit.EnableWindow(FALSE);
        }
        _m_messageEdit.SetWindowText(_m_aMessage[_m_curMessage].m_text);
    } else {
        _m_messageTab.EnableWindow(FALSE);
        _m_customizeMessageCheck.EnableWindow(FALSE);
        _m_messageEdit.EnableWindow(FALSE);
    }
    return TRUE;
}

VA_COMPGEN(0x00499267, 0x8, IMPLICIT_DTOR, TGetQuestDescFunc)

VA(0x0049926f, 0x2eb)
void TQuestLocationPropsDlg::OnOK()
{
    CDialog::OnOK();
    if (_m_pQuest.get() != NULL) {
        _m_pNewMap->setQuest(TMapObjectRef(_m_bSecondLayer, _m_objID), _m_pQuest);
        if (_m_deadlineCheck.GetCheck()) {
            CString text;
            int month = 0;
            _m_monthEdit.GetWindowText(text);
            sscanf(text, "%d", &month);
            month = clamp(1, month, 99);
            int week = 0;
            _m_weekEdit.GetWindowText(text);
            sscanf(text, "%d", &week);
            week = clamp(1, week, 4);
            int day = 0;
            _m_dayEdit.GetWindowText(text);
            sscanf(text, "%d", &day);
            day = clamp(1, day, 7);
            _m_pNewLocation->setDeadline((month - 1) * 28 + (week - 1) * 7 + day - 1);
        } else
            _m_pNewLocation->setDeadline(-1);
        for (unsigned int i = 0; i < TQuestLocation::s_kNumMessages; i++) {
            if (_m_aMessage[i].m_bCustomized) {
                _m_aMessage[i].m_text.Replace("\r\n", "\n");
                _m_pNewLocation->setMessage(i, std::string(_m_aMessage[i].m_text));
            } else
                _m_pNewLocation->setMessage(i, std::string());
        }
    } else if (_m_pNewLocation->getPQuest() != NULL)
        _m_pNewMap->clearQuest(TMapObjectRef(_m_bSecondLayer, _m_objID));
    _m_bModified = _m_bModified
                   || (_m_pNewLocation->getPQuest() != NULL) != (_m_pOldLocation->getPQuest() != NULL)
                   || (_m_pNewLocation->getPQuest() != NULL
                       && !TQuest::equivalent(*_m_pNewLocation->getPQuest(), *_m_pOldLocation->getPQuest()))
                   || _m_pNewLocation->getDeadline() != _m_pOldLocation->getDeadline();
    if (!_m_bModified) {
        for (int i = 0; i < TQuestLocation::s_kNumMessages; i++) {
            if (_m_pNewLocation->getMessage(i) != _m_pOldLocation->getMessage(i)) {
                _m_bModified = true;
                break;
            }
        }
    }
}

VA(0x0049956c, 0x117)
void TQuestLocationPropsDlg::OnEditQuestButton()
{
    TEditQuestDlg dlg(this, _m_pQuest.get(), _m_pNewMap);
    if (dlg.DoModal() == IDOK) {
        std::auto_ptr<TQuest> pNewQuest = dlg.getQuest();
        if ((pNewQuest.get() != NULL) != (_m_pQuest.get() != NULL)
            || (pNewQuest.get() != NULL && !TQuest::equivalent(*pNewQuest, *_m_pQuest))) {
            _onQuestChanged(pNewQuest.get());
            _m_pQuest = pNewQuest;
            TGetQuestDescFunc getQuestDesc(_m_pNewMap);
            _m_questEdit.SetWindowText(getQuestDesc(_m_pQuest.get()));
        }
    }
}

VA(0x00499683, 0x15c)
void TQuestLocationPropsDlg::OnDeadlineCheck()
{
    if (_m_deadlineCheck.GetCheck()) {
        _m_monthStatic.EnableWindow(TRUE);
        _m_monthEdit.EnableWindow(TRUE);
        _m_monthSpin.EnableWindow(TRUE);
        _m_weekStatic.EnableWindow(TRUE);
        _m_weekEdit.EnableWindow(TRUE);
        _m_weekSpin.EnableWindow(TRUE);
        _m_dayStatic.EnableWindow(TRUE);
        _m_dayEdit.EnableWindow(TRUE);
        _m_daySpin.EnableWindow(TRUE);
        _m_monthEdit.SetWindowText("1");
        _m_weekEdit.SetWindowText("1");
        _m_dayEdit.SetWindowText("1");
    } else {
        _m_monthEdit.SetWindowText("");
        _m_weekEdit.SetWindowText("");
        _m_dayEdit.SetWindowText("");
        _m_monthStatic.EnableWindow(FALSE);
        _m_monthEdit.EnableWindow(FALSE);
        _m_monthSpin.EnableWindow(FALSE);
        _m_weekStatic.EnableWindow(FALSE);
        _m_weekEdit.EnableWindow(FALSE);
        _m_weekSpin.EnableWindow(FALSE);
        _m_dayStatic.EnableWindow(FALSE);
        _m_dayEdit.EnableWindow(FALSE);
        _m_daySpin.EnableWindow(FALSE);
    }
}

VA(0x004997df, 0xa6)
void TQuestLocationPropsDlg::OnKillFocusMonthEdit()
{
    CString text;
    int month = 0;
    _m_monthEdit.GetWindowText(text);
    sscanf(text, "%d", &month);
    if (month < 1 || month > 99U) {
        month = clamp(1, month, 99);
        text.Format("%d", month);
        _m_monthEdit.SetWindowText(text);
    }
}

VA(0x00499885, 0xa6)
void TQuestLocationPropsDlg::OnKillFocusWeekEdit()
{
    CString text;
    int week = 0;
    _m_weekEdit.GetWindowText(text);
    sscanf(text, "%d", &week);
    if (week < 1 || week > 4U) {
        week = clamp(1, week, 4);
        text.Format("%d", week);
        _m_weekEdit.SetWindowText(text);
    }
}

VA(0x0049992b, 0xa6)
void TQuestLocationPropsDlg::OnKillFocusDayEdit()
{
    CString text;
    int day = 0;
    _m_dayEdit.GetWindowText(text);
    sscanf(text, "%d", &day);
    if (day < 1 || day > 7U) {
        day = clamp(1, day, 7);
        text.Format("%d", day);
        _m_dayEdit.SetWindowText(text);
    }
}

VA(0x004999d1, 0xa9)
void TQuestLocationPropsDlg::OnSelChangeMessageTab(NMHDR* pNMHDR, LRESULT* pResult)
{
    TCITEM item;
    item.mask = TCIF_PARAM;
    _m_messageTab.GetItem(_m_messageTab.GetCurSel(), &item);
    _m_curMessage = item.lParam;
    const _TMessage& message = _m_aMessage[_m_curMessage];
    if (_m_pNewMap->getVersion() >= GAME_VERSION_AB) {
        _m_customizeMessageCheck.SetCheck(message.m_bCustomized);
        _m_messageEdit.EnableWindow(message.m_bCustomized);
    } else {
        _m_customizeMessageCheck.EnableWindow(FALSE);
        _m_messageEdit.EnableWindow(FALSE);
    }
    _m_messageEdit.SetWindowText(message.m_text);
    *pResult = 0;
}

VA(0x00499a7a, 0xb5)
void TQuestLocationPropsDlg::OnCustomizeMessageCheck()
{
    _TMessage& message = _m_aMessage[_m_curMessage];
    if (_m_customizeMessageCheck.GetCheck()) {
        if (!message.m_bCustomized) {
            message.m_bCustomized = true;
            _m_messageEdit.EnableWindow(TRUE);
        }
    } else if (message.m_bCustomized) {
        message.m_bCustomized = false;
        if (message.m_bBlank)
            _removeBlankMessage();
        message.m_text = _getDefaultMessage(_m_pQuest.get(), _m_curMessage);
        message.m_bBlank = false;
        _m_messageEdit.SetWindowText(message.m_text);
        _m_messageEdit.EnableWindow(FALSE);
    }
}

VA(0x00499b2f, 0x4f)
void TQuestLocationPropsDlg::OnChangeMessageEdit()
{
    _TMessage& message = _m_aMessage[_m_curMessage];
    _m_messageEdit.GetWindowText(message.m_text);
    if (_isspace(message.m_text)) {
        if (!message.m_bBlank) {
            message.m_bBlank = true;
            _addBlankMessage();
        }
    } else if (message.m_bBlank) {
        message.m_bBlank = false;
        _removeBlankMessage();
    }
}
