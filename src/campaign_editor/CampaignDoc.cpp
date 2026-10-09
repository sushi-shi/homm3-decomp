// CampaignDoc.cpp - the campaign document: loading and saving campaign files.
#include "campaign_editor/stdafx.h"

#include <strstream>

#include "va.h"
#include "gzinflatebuf.h"
#include "editor/RawStream.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignDoc.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/NewCampaignDlg.h"

namespace {

class TFileContent {
public:
    TFileContent(int size, auto_ptr<char> pData);
    virtual ~TFileContent();

    auto_ptr<streambuf> createInflateBuf() const;

private:
    int m_size;
    auto_ptr<char> m_pData;
};

class TStrGzInflateBufBase {
public:
    TStrGzInflateBufBase(const char* pData, int size) : m_strBuf(pData, size) {}
    virtual ~TStrGzInflateBufBase() {}

protected:
    strstreambuf m_strBuf;
};

class TStrGzInflateBuf : private TStrGzInflateBufBase, public TGzInflateBuf {
public:
    TStrGzInflateBuf(const char* pData, int size)
        : TStrGzInflateBufBase(pData, size), TGzInflateBuf(&m_strBuf) {}
};

class TScenarioStartingBonusWriter : public TScenarioStartingBonus::TVisitor {
public:
    void write(const TScenarioStartingBonus& bonus, TRawOStream& oStream, int version);

    virtual void visit(const TScenarioBonusSpell& bonus);
    virtual void visit(const TScenarioBonusCreature& bonus);
    virtual void visit(const TScenarioBonusBuilding& bonus);
    virtual void visit(const TScenarioBonusArtifact& bonus);
    virtual void visit(const TScenarioBonusSpellScroll& bonus);
    virtual void visit(const TScenarioBonusPrimarySkill& bonus);
    virtual void visit(const TScenarioBonusSecondarySkill& bonus);
    virtual void visit(const TScenarioBonusResource& bonus);

private:
    TRawOStream* m_pOStream;
    int m_version;
};

class TScenarioStartingOptionsWriter : public TScenarioStartingOptions::TVisitor {
public:
    virtual void visit(const TScenarioOptionsBonus& options);
    virtual void visit(const TScenarioOptionsCrossoverScenario& options);
    virtual void visit(const TScenarioOptionsStartingHero& options);

private:
    TRawOStream* m_pOStream;
    int m_version;
};

DATA(0x004c6f40) TScenarioStartingBonusWriter bonusWriter;
DATA(0x004c6f30) TScenarioStartingOptionsWriter optionsWriter;

VA(0x00410bd0, 0x20)
TFileContent::TFileContent(int size, auto_ptr<char> pData)
    : m_size(size), m_pData(pData)
{
}

VA(0x00410c10, 0x18)
TFileContent::~TFileContent()
{
}

VA(0x00410d70, 0xed)
auto_ptr<streambuf> TFileContent::createInflateBuf() const
{
    auto_ptr<streambuf> pBuf(new TStrGzInflateBuf(m_pData.get(), m_size));
    if (pBuf.get() == NULL)
        throw TAllocationFailure();
    return pBuf;
}

VA(0x00410f30, 0x1d)
void TScenarioStartingBonusWriter::write(const TScenarioStartingBonus& bonus,
                                         TRawOStream& oStream, int version)
{
    m_pOStream = &oStream;
    m_version = version;
    bonus.accept(*this);
}

VA(0x00410f50, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusSpell& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusSpell);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<ubyte>(bonus.m_spell);
}

VA(0x00411010, 0xe9)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusCreature& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusCreature);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<short>(bonus.m_creature);
    *m_pOStream << static_cast<short>(bonus.m_count);
}

VA(0x00411100, 0x7b)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusBuilding& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusBuilding);
    *m_pOStream << static_cast<ubyte>(bonus.m_building);
}

VA(0x00411180, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusArtifact& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusArtifact);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<short>(bonus.m_artifact);
}

VA(0x00411240, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusSpellScroll& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusSpellScroll);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<ubyte>(bonus.m_spell);
}

VA(0x00411300, 0xd0)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusPrimarySkill& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusPrimarySkill);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    for (unsigned int i = 0; i < kNumPrimarySkills; i++)
        *m_pOStream << static_cast<ubyte>(bonus.m_skills[i]);
}

VA(0x004113d0, 0xe9)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusSecondarySkill& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusSecondarySkill);
    *m_pOStream << static_cast<short>(bonus.m_hero);
    *m_pOStream << static_cast<ubyte>(bonus.m_skill);
    *m_pOStream << static_cast<ubyte>(bonus.m_level);
}

VA(0x004114c0, 0xb3)
void TScenarioStartingBonusWriter::visit(const TScenarioBonusResource& bonus)
{
    *m_pOStream << static_cast<ubyte>(eBonusResource);
    *m_pOStream << static_cast<ubyte>(bonus.m_resource);
    *m_pOStream << static_cast<long>(bonus.m_amount);
}

VA(0x004115a0, 0x540)
auto_ptr<TScenarioStartingBonus> readStartingBonus(TRawIStream& iStream, int version)
{
    signed char type;
    iStream >> type;
    auto_ptr<TScenarioStartingBonus> pBonus;
    switch (type) {
    case eBonusSpell: {
        short hero;
        iStream >> hero;
        signed char spell;
        iStream >> spell;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusSpell(hero, spell));
        break;
    }
    case eBonusCreature: {
        short hero;
        iStream >> hero;
        short creature;
        iStream >> creature;
        short count;
        iStream >> count;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusCreature(hero, creature, count));
        break;
    }
    case eBonusBuilding: {
        signed char building;
        iStream >> building;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusBuilding(building));
        break;
    }
    case eBonusArtifact: {
        short hero;
        iStream >> hero;
        int artifact;
        if (version >= 3) {
            short wideArtifact;
            iStream >> wideArtifact;
            artifact = wideArtifact;
        } else {
            signed char narrowArtifact;
            iStream >> narrowArtifact;
            artifact = narrowArtifact;
        }
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusArtifact(hero, artifact));
        break;
    }
    case eBonusSpellScroll: {
        short hero;
        iStream >> hero;
        signed char spell;
        iStream >> spell;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusSpellScroll(hero, spell));
        break;
    }
    case eBonusPrimarySkill: {
        short hero;
        iStream >> hero;
        int aSkills[kNumPrimarySkills];
        for (int i = 0; i < kNumPrimarySkills; i++) {
            signed char skill;
            iStream >> skill;
            aSkills[i] = skill;
        }
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusPrimarySkill(hero, aSkills));
        break;
    }
    case eBonusSecondarySkill: {
        short hero;
        iStream >> hero;
        signed char skill;
        iStream >> skill;
        signed char level;
        iStream >> level;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusSecondarySkill(hero, skill, level));
        break;
    }
    default: {
        signed char resource;
        iStream >> resource;
        long amount;
        iStream >> amount;
        pBonus = auto_ptr<TScenarioStartingBonus>(new TScenarioBonusResource(resource, amount));
        break;
    }
    }
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x00411b80, 0xf7)
void TScenarioStartingOptionsWriter::visit(const TScenarioOptionsBonus& options)
{
    *m_pOStream << static_cast<ubyte>(eOptionsBonus);
    *m_pOStream << static_cast<ubyte>(options.m_player);
    *m_pOStream << static_cast<ubyte>(options.m_bonuses.size());
    for (unsigned int i = 0; i < options.m_bonuses.size(); i++)
        bonusWriter.write(*options.m_bonuses[i], *m_pOStream, m_version);
}

VA(0x00411c80, 0x11e)
void TScenarioStartingOptionsWriter::visit(const TScenarioOptionsCrossoverScenario& options)
{
    *m_pOStream << static_cast<ubyte>(eOptionsCrossoverScenario);
    *m_pOStream << static_cast<ubyte>(options.m_choices.size());
    for (unsigned int i = 0; i < options.m_choices.size(); i++) {
        const TScenarioOptionsCrossoverScenario::TChoice& choice = options.m_choices[i];
        *m_pOStream << static_cast<ubyte>(choice.m_player);
        *m_pOStream << static_cast<ubyte>(choice.m_scenario);
    }
}

VA(0x00411da0, 0x11e)
void TScenarioStartingOptionsWriter::visit(const TScenarioOptionsStartingHero& options)
{
    *m_pOStream << static_cast<ubyte>(eOptionsStartingHero);
    *m_pOStream << static_cast<ubyte>(options.m_choices.size());
    for (unsigned int i = 0; i < options.m_choices.size(); i++) {
        const TScenarioOptionsStartingHero::TChoice& choice = options.m_choices[i];
        *m_pOStream << static_cast<ubyte>(choice.m_player);
        *m_pOStream << static_cast<short>(choice.m_hero);
    }
}


// What a campaign file holds: the campaign and the file version it was
// written with.
struct TLoadedCampaign {
    auto_ptr<TCampaign> m_pCampaign;
    int m_version;
};

void saveCampaign(const TCampaign& campaign, int version, CFile* pFile);
TLoadedCampaign loadCampaign(CFile* pFile);

}

// The supported campaign file versions; the last is the one written.
enum { kCurrentFileVersion = 6 };

// A new campaign is made for this campaign map unless the new campaign
// dialog chose another.
enum { kDefaultNewCampaignType = 1 };

DATA(0x00487ee0)
IMPLEMENT_DYNAMIC(TCampaignDocLoadFailure, CException)

DATA(0x00487ef8)
IMPLEMENT_DYNAMIC(TCampaignDocInvalidFileVersion, TCampaignDocLoadFailure)

VA(0x00414020, 0x57)
IMPLEMENT_DYNCREATE(TCampaignDoc, CDocument)

VA(0x00414090, 0x6)
BEGIN_MESSAGE_MAP(TCampaignDoc, CDocument)
    ON_COMMAND(ID_FILE_REFRESH_SCENARIO_MAPS, OnRefreshScenarioMaps)
    ON_COMMAND(ID_FILE_EXPORT_SCENARIO_MAPS, OnExportScenarioMaps)
    ON_COMMAND(ID_FILE_EXPORT_TEXT, OnExportText)
    ON_COMMAND(ID_FILE_IMPORT_TEXT, OnImportText)
END_MESSAGE_MAP()

VA(0x004140a0, 0x2b)
TCampaignDoc::TCampaignDoc()
    : _m_bNewCampaign(false), _m_newVersion(eCampaignVersionShadowOfDeath), _m_newType(kDefaultNewCampaignType)
{
}

VA(0x004140f0, 0x6a)
TCampaignDoc::~TCampaignDoc()
{
}

VA(0x00414160, 0x12f)
BOOL TCampaignDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;
    _m_pCampaign = auto_ptr<TCampaign>(new TCampaign(_m_newType));
    if (_m_pCampaign.get() == NULL)
        throw TAllocationFailure();
    _m_version = _m_newVersion;
    _m_newVersion = eCampaignVersionShadowOfDeath;
    _m_newType = kDefaultNewCampaignType;
    return TRUE;
}

VA(0x00414290, 0xfc)
void TCampaignDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring()) {
        saveCampaign(*_m_pCampaign, _m_version, ar.GetFile());
    } else {
        TLoadedCampaign loaded = loadCampaign(ar.GetFile());
        _m_pCampaign = loaded.m_pCampaign;
        _m_version = loaded.m_version;
    }
}

VA(0x00414a50, 0xa)
void TCampaignDoc::setVersion(int newVersion)
{
    _m_version = newVersion;
}

VA(0x00414a60, 0x7d)
void TCampaignDoc::DeleteContents()
{
    _m_pCampaign = auto_ptr<TCampaign>();
    CDocument::DeleteContents();
}

VA(0x00414ae0, 0x13)
BOOL TCampaignDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (!CDocument::OnOpenDocument(lpszPathName))
        return FALSE;
    return TRUE;
}

VA(0x00414b00, 0xda)
void TCampaignDoc::ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault)
{
    CString prompt;
    if (e->IsKindOf(RUNTIME_CLASS(TCampaignDocInvalidFileVersion))) {
        TCampaignDocInvalidFileVersion* pVersionFailure = static_cast<TCampaignDocInvalidFileVersion*>(e);
        prompt.Format(kInvalidCampaignVersionFmtStr, pVersionFailure->m_currentVersion, pVersionFailure->m_fileVersion);
        AfxMessageBox(prompt, MB_ICONEXCLAMATION);
    } else if (e->IsKindOf(RUNTIME_CLASS(TCampaignDocLoadFailure))) {
        prompt = kInvalidCampaignFileStr;
        AfxMessageBox(prompt, MB_ICONEXCLAMATION);
    } else {
        CDocument::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
    }
}

VA(0x00414f70, 0x122)
BOOL TCampaignDoc::SaveModified()
{
    bool bNewCampaign = _m_bNewCampaign;
    _m_bNewCampaign = false;
    if (!CDocument::SaveModified())
        return FALSE;
    if (bNewCampaign) {
        TNewCampaignDlg dlg(AfxGetMainWnd(), eCampaignVersionShadowOfDeath, kDefaultNewCampaignType);
        if (dlg.DoModal() != IDOK)
            return FALSE;
        _m_newVersion = dlg.getVersion();
        _m_newType = dlg.getType();
    }
    return TRUE;
}
