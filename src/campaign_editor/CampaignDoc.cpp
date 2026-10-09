// CampaignDoc.cpp - the campaign document: loading and saving campaign files.
#include "campaign_editor/stdafx.h"

#include <strstream>

#include "va.h"
#include "gzinflatebuf.h"
#include "editor/MFCFileBuf.h"
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

    const char* getData() const { return m_pData.get(); }
    int getSize() const { return m_size; }

private:
    int m_size;
    auto_ptr<char> m_pData;
};

// A scenario map as a campaign file carries it: the deflated map file, and
// the header read out of it.
class TGzDeflatedScenarioMap : public TFileContent, public TCampaignScenarioMap {
public:
    TGzDeflatedScenarioMap(const string& fileName, int size, auto_ptr<char> pData, int campaignVersion,
                           bool bAnyVersion);
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
    void write(const TScenarioStartingOptions* pOptions, TRawOStream& oStream, int version);

    virtual void visit(const TScenarioOptionsBonus& options);
    virtual void visit(const TScenarioOptionsCrossoverScenario& options);
    virtual void visit(const TScenarioOptionsStartingHero& options);

private:
    TRawOStream* m_pOStream;
    int m_version;
};

// A scenario as a campaign file stores it: the map file name and the
// length of its deflated data, then what the campaign says about the
// scenario. Records are read before the maps that follow them in the file.
struct TScenarioRecord {
    explicit TScenarioRecord(int numScenarios)
        : m_mapSize(0), m_prerequisites(numScenarios, false), m_regionColor(0), m_difficulty(1) {}

    void read(TRawIStream& iStream, int version);
    void write(TRawOStream& oStream, int version) const;

    string m_mapFileName;
    long m_mapSize;
    vector<bool> m_prerequisites;
    int m_regionColor;
    int m_difficulty;
    string m_regionDesc;
    auto_ptr<TScenarioPrologue> m_pPrologue;
    auto_ptr<TScenarioPrologue> m_pEpilogue;
    TScenarioCrossover m_crossover;
    auto_ptr<TScenarioStartingOptions> m_pStartingOptions;
};

// The campaign file versions, by campaign version; the last is written.
DATA(0x00487ed0) const int kFileVersions[] = { 3, 5, 6 };

enum {
    kNumFileVersions = 3,
    kCurrentFileVersion = 6
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

VA(0x00410c30, 0xbe)
TGzDeflatedScenarioMap::TGzDeflatedScenarioMap(const string& fileName, int size, auto_ptr<char> pData,
                                               int campaignVersion, bool bAnyVersion)
    : TFileContent(size, pData),
      TCampaignScenarioMap(fileName, createInflateBuf().get(), campaignVersion, bAnyVersion)
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

VA(0x00411b20, 0x60)
void TScenarioStartingOptionsWriter::write(const TScenarioStartingOptions* pOptions, TRawOStream& oStream,
                                           int version)
{
    if (pOptions != NULL) {
        m_pOStream = &oStream;
        m_version = version;
        pOptions->accept(*this);
    } else {
        oStream << static_cast<ubyte>(eOptionsNone);
    }
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


VA(0x00411ee0, 0x518)
auto_ptr<TScenarioStartingOptions> readStartingOptions(TRawIStream& iStream, int version)
{
    auto_ptr<TScenarioStartingOptions> pOptions;
    signed char type;
    iStream >> type;
    if (type == eOptionsNone)
        return pOptions;
    switch (type) {
    case eOptionsBonus: {
        signed char player;
        iStream >> player;
        signed char numBonuses;
        iStream >> numBonuses;
        vector<auto_ptr<TScenarioStartingBonus> > bonuses;
        bonuses.reserve(numBonuses);
        for (unsigned int i = 0; i < numBonuses; i++)
            bonuses.push_back(readStartingBonus(iStream, version));
        pOptions = auto_ptr<TScenarioStartingOptions>(new TScenarioOptionsBonus(player, bonuses));
        break;
    }
    case eOptionsCrossoverScenario: {
        signed char numChoices;
        iStream >> numChoices;
        vector<TScenarioOptionsCrossoverScenario::TChoice> choices;
        choices.reserve(numChoices);
        for (unsigned int i = 0; i < numChoices; i++) {
            signed char player;
            iStream >> player;
            signed char scenario;
            iStream >> scenario;
            TScenarioOptionsCrossoverScenario::TChoice choice;
            choice.m_scenario = scenario;
            choice.m_player = player;
            choices.push_back(choice);
        }
        pOptions = auto_ptr<TScenarioStartingOptions>(new TScenarioOptionsCrossoverScenario(choices));
        break;
    }
    default: {
        signed char numChoices;
        iStream >> numChoices;
        vector<TScenarioOptionsStartingHero::TChoice> choices;
        choices.reserve(numChoices);
        for (unsigned int i = 0; i < numChoices; i++) {
            signed char player;
            iStream >> player;
            short hero;
            iStream >> hero;
            TScenarioOptionsStartingHero::TChoice choice;
            choice.m_hero = hero;
            choice.m_player = player;
            choices.push_back(choice);
        }
        pOptions = auto_ptr<TScenarioStartingOptions>(new TScenarioOptionsStartingHero(choices));
        break;
    }
    }
    if (pOptions.get() == NULL)
        throw TAllocationFailure();
    return pOptions;
}

VA(0x00412400, 0x5e3)
void TScenarioRecord::read(TRawIStream& iStream, int version)
{
    iStream >> m_mapFileName;
    iStream >> m_mapSize;
    vector<unsigned char> prerequisiteBytes((m_prerequisites.size() + 7) / 8, 0);
    for (vector<unsigned char>::iterator it = prerequisiteBytes.begin(); it != prerequisiteBytes.end(); ++it)
        iStream >> *it;
    if (m_mapFileName.length() != 0) {
        for (unsigned int i = 0; i < m_prerequisites.size(); i++)
            m_prerequisites[i] = (prerequisiteBytes[i / 8] & (1 << (i % 8))) != 0;
    } else {
        fill(m_prerequisites.begin(), m_prerequisites.end(), false);
    }
    if (version >= 2) {
        signed char regionColor;
        iStream >> regionColor;
        m_regionColor = regionColor;
        signed char difficulty;
        iStream >> difficulty;
        m_difficulty = difficulty;
        iStream >> m_regionDesc;
        signed char bHasPrologue;
        iStream >> bHasPrologue;
        if (bHasPrologue != 0) {
            m_pPrologue = auto_ptr<TScenarioPrologue>(new TScenarioPrologue);
            if (m_pPrologue.get() == NULL)
                throw TAllocationFailure();
            iStream >> *m_pPrologue;
        }
        signed char bHasEpilogue;
        iStream >> bHasEpilogue;
        if (bHasEpilogue != 0) {
            m_pEpilogue = auto_ptr<TScenarioPrologue>(new TScenarioPrologue);
            if (m_pEpilogue.get() == NULL)
                throw TAllocationFailure();
            iStream >> *m_pEpilogue;
        }
        m_crossover.read(iStream, version);
        m_pStartingOptions = readStartingOptions(iStream, version);
    } else {
        m_regionColor = 0;
        m_difficulty = 1;
        m_regionDesc = string();
        m_pPrologue = auto_ptr<TScenarioPrologue>();
        m_pEpilogue = auto_ptr<TScenarioPrologue>();
        m_crossover = TScenarioCrossover();
        m_pStartingOptions = auto_ptr<TScenarioStartingOptions>();
    }
}

VA(0x004129f0, 0x2b0)
void TScenarioRecord::write(TRawOStream& oStream, int version) const
{
    oStream << m_mapFileName;
    oStream << m_mapSize;
    vector<unsigned char> prerequisiteBytes((m_prerequisites.size() + 7) / 8, 0);
    if (m_mapFileName.length() != 0) {
        for (unsigned int i = 0; i < m_prerequisites.size(); i++) {
            if (m_prerequisites[i])
                prerequisiteBytes[i / 8] |= 1 << (i % 8);
        }
    }
    for (vector<unsigned char>::const_iterator it = prerequisiteBytes.begin(); it != prerequisiteBytes.end(); ++it)
        oStream << *it;
    oStream << static_cast<ubyte>(m_regionColor);
    oStream << static_cast<ubyte>(m_difficulty);
    oStream << m_regionDesc;
    if (m_pPrologue.get() != NULL) {
        oStream << static_cast<ubyte>(true);
        oStream << *m_pPrologue;
    } else {
        oStream << static_cast<ubyte>(false);
    }
    if (m_pEpilogue.get() != NULL) {
        oStream << static_cast<ubyte>(true);
        oStream << *m_pEpilogue;
    } else {
        oStream << static_cast<ubyte>(false);
    }
    m_crossover.write(oStream, version);
    optionsWriter.write(m_pStartingOptions.get(), oStream, version);
}

VA(0x00412ca0, 0x138)
void reportCreateFailure(const string& fileName, const TCampaignScenarioMap::TCreateFailure& failure, CWnd* pWnd)
{
    CString prompt;
    if (dynamic_cast<const TCampaignScenarioMap::TMapFileIsInvalid*>(&failure) != NULL) {
        prompt.Format(kMapInvalidFmtStr, fileName.c_str());
    } else if (dynamic_cast<const TCampaignScenarioMap::TMapIsUnplayable*>(&failure) != NULL) {
        prompt.Format(kMapUnplayableFmtStr, fileName.c_str());
    } else {
        const TCampaignScenarioMap::TMapFileIsIncorrectVersion& versionFailure =
            static_cast<const TCampaignScenarioMap::TMapFileIsIncorrectVersion&>(failure);
        if (versionFailure.m_version <= versionFailure.m_maxVersion
            && versionFailure.m_version >= versionFailure.m_maxVersion - 1)
            prompt.Format(kMapOldVersionFmtStr, fileName.c_str());
        else
            prompt.Format(kMapInvalidVersionFmtStr, fileName.c_str());
    }
    if (pWnd != NULL)
        pWnd->MessageBox(prompt);
    else
        AfxMessageBox(prompt);
}


}

// A new campaign is made for this campaign map unless the new campaign
// dialog chose another.
enum { kDefaultNewCampaignType = 1 };

DATA(0x00487ee0)
IMPLEMENT_DYNAMIC(TCampaignDocLoadFailure, CException)

DATA(0x00487ef8)
IMPLEMENT_DYNAMIC(TCampaignDocInvalidFileVersion, TCampaignDocLoadFailure)

namespace {

VA(0x00412e00, 0x6f0)
void saveCampaign(const TCampaign& campaign, int version, CFile* pFile)
{
    unsigned int scenario;
    {
        TMFCFileBuf fileBuf(pFile);
        TGzDeflateBuf deflateBuf(&fileBuf);
        TRawOStream oStream(&deflateBuf);
        oStream << kFileVersions[version];
        oStream << static_cast<ubyte>(campaign.getType());
        oStream << campaign.getName();
        oStream << campaign.getDescription();
        oStream << static_cast<ubyte>(campaign.getBDifficultyChoice());
        oStream << static_cast<ubyte>(campaign.getMusic());
        for (scenario = 0; scenario < campaign.getMapTraits().m_numRegions; scenario++) {
            TScenarioRecord record(campaign.getMapTraits().m_numRegions);
            const TScenario& rScenario = campaign.getScenario(scenario);
            const TCampaignScenarioMap* pMap = rScenario.getMap();
            if (pMap != NULL) {
                record.m_mapFileName = pMap->m_fileName;
                record.m_mapSize = static_cast<const TGzDeflatedScenarioMap*>(pMap)->getSize();
                for (unsigned int other = 0; other < campaign.getMapTraits().m_numRegions; other++) {
                    if (other != scenario && campaign.getScenario(other).getMap() != NULL
                        && campaign.getBDirectPrerequisite(scenario, other))
                        record.m_prerequisites[other] = true;
                }
                record.m_regionColor = rScenario.getRegionColor();
                record.m_difficulty = rScenario.getDifficulty();
                record.m_regionDesc = rScenario.getRegionDesc();
                if (rScenario.getPrologue() != NULL) {
                    record.m_pPrologue = auto_ptr<TScenarioPrologue>(new TScenarioPrologue(*rScenario.getPrologue()));
                    if (record.m_pPrologue.get() == NULL)
                        throw TAllocationFailure();
                }
                if (rScenario.getEpilogue() != NULL) {
                    record.m_pEpilogue = auto_ptr<TScenarioPrologue>(new TScenarioPrologue(*rScenario.getEpilogue()));
                    if (record.m_pEpilogue.get() == NULL)
                        throw TAllocationFailure();
                }
                record.m_crossover = rScenario.getCrossover();
                record.m_pStartingOptions = rScenario.getStartingOptions()->clone();
                if (record.m_pStartingOptions.get() == NULL)
                    throw TAllocationFailure();
            }
            record.write(oStream, version);
        }
    }
    for (scenario = 0; scenario < campaign.getMapTraits().m_numRegions; scenario++) {
        const TCampaignScenarioMap* pMap = campaign.getScenario(scenario).getMap();
        if (pMap != NULL) {
            const TGzDeflatedScenarioMap* pDeflatedMap = static_cast<const TGzDeflatedScenarioMap*>(pMap);
            pFile->Write(pDeflatedMap->getData(), pDeflatedMap->getSize());
        }
    }
}

VA(0x00413600, 0x735)
pair<auto_ptr<TCampaign>, int> loadCampaign(CFile* pFile)
{
    auto_ptr<TCampaign> pCampaign;
    int campaignVersion;
    vector<TScenarioRecord> records;
    unsigned int scenario;
    try {
        {
            TMFCFileBuf fileBuf(pFile);
            TGzInflateBuf inflateBuf(&fileBuf);
            TRawIStream iStream(&inflateBuf);
            int fileVersion;
            iStream >> fileVersion;
            for (campaignVersion = 0;; campaignVersion++) {
                if (campaignVersion >= kNumFileVersions)
                    throw new TCampaignDocInvalidFileVersion(fileVersion, kCurrentFileVersion);
                if (fileVersion == kFileVersions[campaignVersion])
                    break;
            }
            signed char type;
            iStream >> type;
            pCampaign = auto_ptr<TCampaign>(new TCampaign(type));
            if (pCampaign.get() == NULL)
                throw TAllocationFailure();
            string name;
            string description;
            iStream >> name;
            iStream >> description;
            signed char bDifficultyChoice;
            iStream >> bDifficultyChoice;
            pCampaign->setName(name);
            pCampaign->setDescription(description);
            pCampaign->setBDifficultyChoice(bDifficultyChoice != 0);
            if (fileVersion >= 5) {
                signed char music;
                iStream >> music;
                pCampaign->setMusic(music);
            }
            records.resize(pCampaign->getMapTraits().m_numRegions,
                           TScenarioRecord(pCampaign->getMapTraits().m_numRegions));
            for (scenario = 0; scenario < pCampaign->getMapTraits().m_numRegions; scenario++)
                records[scenario].read(iStream, fileVersion);
        }
        for (scenario = 0; scenario < pCampaign->getMapTraits().m_numRegions; scenario++) {
            TScenarioRecord& record = records[scenario];
            if (record.m_mapFileName.length() == 0)
                continue;
            try {
                auto_ptr<char> pData(new char[record.m_mapSize]);
                if (pData.get() == NULL)
                    throw TAllocationFailure();
                if (pFile->Read(pData.get(), record.m_mapSize) < record.m_mapSize)
                    throw new TCampaignDocLoadFailure;
                auto_ptr<TCampaignScenarioMap> pMap(
                    new TGzDeflatedScenarioMap(record.m_mapFileName, record.m_mapSize, pData, campaignVersion, true));
                if (pMap.get() == NULL)
                    throw TAllocationFailure();
                pCampaign->setScenarioMap(scenario, pMap);
            } catch (TCampaignScenarioMap::TCreateFailure& failure) {
                reportCreateFailure(record.m_mapFileName, failure, NULL);
                throw new TCampaignDocLoadFailure;
            }
        }
        for (scenario = 0; scenario < pCampaign->getMapTraits().m_numRegions; scenario++) {
            TScenarioRecord& record = records[scenario];
            pCampaign->getScenario(scenario).setRegionColor(record.m_regionColor);
            if (record.m_mapFileName.length() == 0)
                continue;
            for (unsigned int other = 0; other < pCampaign->getMapTraits().m_numRegions; other++) {
                if (other != scenario && records[other].m_mapFileName.length() != 0 && record.m_prerequisites[other])
                    pCampaign->setBPrerequisite(scenario, other, true);
            }
            TScenario& rScenario = pCampaign->getScenario(scenario);
            rScenario.setDifficulty(record.m_difficulty);
            rScenario.setRegionDesc(record.m_regionDesc);
            if (record.m_pPrologue.get() != NULL)
                rScenario.setPrologue(record.m_pPrologue);
            if (record.m_pEpilogue.get() != NULL)
                rScenario.setEpilogue(record.m_pEpilogue);
            rScenario.setCrossover(record.m_crossover);
            if (record.m_pStartingOptions.get() != NULL)
                pCampaign->setScenarioStartingOptions(scenario, record.m_pStartingOptions);
        }
    } catch (TRawIStream::TReadFailure&) {
        throw new TCampaignDocLoadFailure;
    } catch (TGzInflateBuf::TDataError&) {
        throw new TCampaignDocLoadFailure;
    }
    return make_pair(pCampaign, campaignVersion);
}

}

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
        pair<auto_ptr<TCampaign>, int> loaded = loadCampaign(ar.GetFile());
        _m_pCampaign = loaded.first;
        _m_version = loaded.second;
    }
}

VA(0x00414390, 0x24b)
auto_ptr<TCampaignScenarioMap> TCampaignDoc::loadScenarioMap(const CString& pathName)
{
    CWaitCursor wait;
    CString fileName = pathName.Right(pathName.GetLength() - pathName.ReverseFind('\\') - 1);
    CFileException fe;
    CFile* pFile = GetFile(pathName, CFile::modeRead | CFile::shareDenyWrite, &fe);
    if (pFile == NULL) {
        ReportSaveLoadException(pathName, &fe, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
        return auto_ptr<TCampaignScenarioMap>();
    }
    auto_ptr<char> pData;
    try {
        UINT size = pFile->GetLength();
        if (size > 0) {
            pData = auto_ptr<char>(new char[size]);
            if (pData.get() == NULL)
                throw TAllocationFailure();
            pFile->Read(pData.get(), size);
        }
        ReleaseFile(pFile, FALSE);
        auto_ptr<TCampaignScenarioMap> pMap(
            new TGzDeflatedScenarioMap(string(fileName), size, pData, _m_version, false));
        if (pMap.get() == NULL)
            throw TAllocationFailure();
        return pMap;
    } catch (TCampaignScenarioMap::TCreateFailure& failure) {
        reportCreateFailure(string(fileName), failure, NULL);
        return auto_ptr<TCampaignScenarioMap>();
    } catch (CException* e) {
        ReleaseFile(pFile, TRUE);
        ReportSaveLoadException(pathName, e, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
        e->Delete();
        return auto_ptr<TCampaignScenarioMap>();
    } catch (...) {
        ReleaseFile(pFile, TRUE);
        throw;
    }
}

VA(0x00414890, 0x1c0)
void TCampaignDoc::exportScenarioMap(const TCampaignScenarioMap* pMap, const CString& pathName)
{
    CWaitCursor wait;
    const TGzDeflatedScenarioMap* pDeflatedMap = static_cast<const TGzDeflatedScenarioMap*>(pMap);
    int size = pDeflatedMap->getSize();
    const char* pData = pDeflatedMap->getData();
    CString fileName = pathName.Right(pathName.GetLength() - pathName.ReverseFind('\\') - 1);
    CFileException fe;
    CFile* pFile = GetFile(pathName, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive, &fe);
    if (pFile == NULL) {
        ReportSaveLoadException(pathName, &fe, TRUE, AFX_IDP_INVALID_FILENAME);
        return;
    }
    try {
        pFile->Write(pData, size);
        ReleaseFile(pFile, FALSE);
    } catch (CException* e) {
        ReleaseFile(pFile, TRUE);
        ReportSaveLoadException(pathName, e, TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
        e->Delete();
    } catch (...) {
        ReleaseFile(pFile, TRUE);
        throw;
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

VA(0x00414be0, 0x1b0)
void TCampaignDoc::OnRefreshScenarioMaps()
{
    CWaitCursor wait;
    CString directory;
    GetCurrentDirectory(MAX_PATH, directory.GetBuffer(MAX_PATH));
    directory.ReleaseBuffer();
    if (directory[directory.GetLength() - 1] != '\\')
        directory += '\\';
    for (unsigned int scenario = 0; scenario < _m_pCampaign->getMapTraits().m_numRegions; scenario++) {
        const TCampaignScenarioMap* pMap = _m_pCampaign->getScenario(scenario).getMap();
        if (pMap == NULL)
            continue;
        try {
            auto_ptr<TCampaignScenarioMap> pNewMap = loadScenarioMap(directory + pMap->m_fileName.c_str());
            if (pNewMap.get() != NULL) {
                _m_pCampaign->setScenarioMap(scenario, pNewMap);
                SetModifiedFlag();
            }
        } catch (TCampaignScenarioMap::TCreateFailure& failure) {
            reportCreateFailure(pMap->m_fileName, failure, NULL);
        }
    }
}

VA(0x00414d90, 0xb5)
void TCampaignDoc::OnExportScenarioMaps()
{
    CWaitCursor wait;
    CString directory;
    GetCurrentDirectory(MAX_PATH, directory.GetBuffer(MAX_PATH));
    directory.ReleaseBuffer();
    if (directory[directory.GetLength() - 1] != '\\')
        directory += '\\';
    for (unsigned int scenario = 0; scenario < _m_pCampaign->getMapTraits().m_numRegions; scenario++) {
        const TCampaignScenarioMap* pMap = _m_pCampaign->getScenario(scenario).getMap();
        if (pMap == NULL)
            continue;
        CString pathName = directory + pMap->m_fileName.c_str();
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(pathName, &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            FindClose(hFind);
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                CString prompt;
                prompt.Format(kFileExistsFmtStr, (LPCTSTR)pathName);
                if (AfxMessageBox(prompt, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                    continue;
            }
        }
        exportScenarioMap(pMap, pathName);
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
