// QuestLocation.cpp - a Seer's Hut or a Quest Guard (h3maped
// 0x496124..0x49717b; Complete only): the quest, its deadline and its
// proposal, progress and completion messages. The map format writes a
// quest as its kind (0 for none, then the format's numbers 1..9) and its
// terms; three messages from map version 19, one before it.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "editor/MapEditorText.h"
#include "editor/QuestLocation.h"
#include "editor/RawStream.h"

namespace {

enum TQuestType {
    eQuestNone = 0,
    eQuestAchieveExperienceLevel = 1,
    eQuestAchievePrimarySkillLevel = 2,
    eQuestDefeatHero = 3,
    eQuestDefeatMonster = 4,
    eQuestBringArtifacts = 5,
    eQuestBringCreatures = 6,
    eQuestBringResources = 7,
    eQuestBeASpecificHero = 8,
    eQuestBelongToASpecificPlayer = 9,
    kNumQuestTypes = 10
};

class TQuestWriter : public TQuest::TVisitor {
public:
    TQuestWriter(TRawOStream* pOStream, int version) : _m_pOStream(pOStream), _m_version(version) {}

    void write(const TQuest* pQuest);

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
    void writeType(TQuestType type) { *_m_pOStream << static_cast<signed char>(type); }

    TRawOStream* _m_pOStream;
    int _m_version;
};

class TQuestReader {
public:
    TQuestReader(TRawIStream* pIStream, int version) : _m_pIStream(pIStream), _m_version(version) {}

    std::auto_ptr<TQuest> read();

private:
    TQuest* readAchieveExperienceLevel();
    TQuest* readAchievePrimarySkillLevel();
    TQuest* readDefeatHero();
    TQuest* readDefeatMonster();
    TQuest* readBringArtifacts();
    TQuest* readBringCreatures();
    TQuest* readBringResources();
    TQuest* readBeASpecificHero();
    TQuest* readBelongToASpecificPlayer();

    static TQuest* (TQuestReader::* const _s_apfnRead[kNumQuestTypes])();

    TRawIStream* _m_pIStream;
    int _m_version;
};

DATA(0x005403f4)
TQuest* (TQuestReader::* const TQuestReader::_s_apfnRead[kNumQuestTypes])() = {
    NULL,
    &TQuestReader::readAchieveExperienceLevel,
    &TQuestReader::readAchievePrimarySkillLevel,
    &TQuestReader::readDefeatHero,
    &TQuestReader::readDefeatMonster,
    &TQuestReader::readBringArtifacts,
    &TQuestReader::readBringCreatures,
    &TQuestReader::readBringResources,
    &TQuestReader::readBeASpecificHero,
    &TQuestReader::readBelongToASpecificPlayer
};

}  // namespace

VA(0x004962f8, 0x28)
void TQuestWriter::write(const TQuest* pQuest)
{
    if (pQuest != NULL)
        pQuest->accept(*this);
    else
        writeType(eQuestNone);
}

VA(0x00496320, 0x31)
void TQuestWriter::visit(const TQuestAchieveExperienceLevel& quest)
{
    writeType(eQuestAchieveExperienceLevel);
    *_m_pOStream << static_cast<unsigned int>(quest.getLevel());
}

VA(0x00496351, 0x3e)
void TQuestWriter::visit(const TQuestAchievePrimarySkillLevel& quest)
{
    writeType(eQuestAchievePrimarySkillLevel);
    for (int primarySkill = 0; primarySkill < kNumPrimarySkills; ++primarySkill)
        *_m_pOStream << static_cast<signed char>(quest.getLevel(TPrimarySkill(primarySkill)));
}

VA(0x0049638f, 0x31)
void TQuestWriter::visit(const TQuestDefeatHero& quest)
{
    writeType(eQuestDefeatHero);
    *_m_pOStream << quest.getHeroLinkID();
}

VA(0x004963c0, 0x31)
void TQuestWriter::visit(const TQuestDefeatMonster& quest)
{
    writeType(eQuestDefeatMonster);
    *_m_pOStream << quest.getMonsterLinkID();
}

VA(0x004963f1, 0x7d)
void TQuestWriter::visit(const TQuestBringArtifacts& quest)
{
    writeType(eQuestBringArtifacts);
    *_m_pOStream << static_cast<unsigned char>(quest.getArtifacts().size());
    for (std::multiset<TArtifact>::const_iterator pArtifact = quest.getArtifacts().begin();
         pArtifact != quest.getArtifacts().end(); ++pArtifact)
        if (_m_version >= 1)
            *_m_pOStream << static_cast<short>(*pArtifact);
        else
            *_m_pOStream << static_cast<unsigned char>(*pArtifact);
}

VA(0x0049646e, 0x7c)
void TQuestWriter::visit(const TQuestBringCreatures& quest)
{
    writeType(eQuestBringCreatures);
    *_m_pOStream << static_cast<unsigned char>(quest.getCreatures().size());
    for (TQuestBringCreatures::TCreatures::const_iterator pCreature = quest.getCreatures().begin();
         pCreature != quest.getCreatures().end(); ++pCreature) {
        *_m_pOStream << static_cast<unsigned short>(pCreature->first);
        *_m_pOStream << static_cast<unsigned short>(pCreature->second);
    }
}

VA(0x004964ea, 0x3f)
void TQuestWriter::visit(const TQuestBringResources& quest)
{
    writeType(eQuestBringResources);
    for (int type = 0; type < kNumGameResourceTypes; ++type)
        *_m_pOStream << static_cast<unsigned int>(quest.getQuantity(TGameResourceType(type)));
}

VA(0x00496529, 0x31)
void TQuestWriter::visit(const TQuestBeASpecificHero& quest)
{
    writeType(eQuestBeASpecificHero);
    *_m_pOStream << static_cast<unsigned char>(quest.getHeroID());
}

VA(0x0049655a, 0x31)
void TQuestWriter::visit(const TQuestBelongToASpecificPlayer& quest)
{
    writeType(eQuestBelongToASpecificPlayer);
    *_m_pOStream << static_cast<unsigned char>(quest.getPlayer());
}

VA(0x0049658b, 0x9e)
std::auto_ptr<TQuest> TQuestReader::read()
{
    signed char type;
    *_m_pIStream >> type;
    TQuestType questType = TQuestType(type);
    if (questType == eQuestNone)
        return std::auto_ptr<TQuest>();
    std::auto_ptr<TQuest> pQuest((this->*_s_apfnRead[questType])());
    if (pQuest.get() == NULL)
        throw TAllocationFailure();
    return pQuest;
}

VA(0x00496629, 0x41)
TQuest* TQuestReader::readAchieveExperienceLevel()
{
    unsigned int level;
    *_m_pIStream >> level;
    return new TQuestAchieveExperienceLevel(level);
}

VA(0x0049666a, 0x64)
TQuest* TQuestReader::readAchievePrimarySkillLevel()
{
    TArray<int, kNumPrimarySkills> aLevel;
    for (int primarySkill = 0; primarySkill < kNumPrimarySkills; ++primarySkill) {
        signed char level;
        *_m_pIStream >> level;
        aLevel[primarySkill] = level;
    }
    return new TQuestAchievePrimarySkillLevel(aLevel);
}

VA(0x004966ce, 0x41)
TQuest* TQuestReader::readDefeatHero()
{
    unsigned int heroLinkID;
    *_m_pIStream >> heroLinkID;
    return new TQuestDefeatHero(heroLinkID);
}

VA(0x0049670f, 0x41)
TQuest* TQuestReader::readDefeatMonster()
{
    unsigned int monsterLinkID;
    *_m_pIStream >> monsterLinkID;
    return new TQuestDefeatMonster(monsterLinkID);
}

VA(0x00496750, 0xba)
TQuest* TQuestReader::readBringArtifacts()
{
    unsigned char value;
    *_m_pIStream >> value;
    unsigned int numArtifacts = value;
    std::multiset<TArtifact> artifacts;
    for (; numArtifacts > 0; --numArtifacts) {
        int artifact;
        if (_m_version >= 21) {
            short value;
            *_m_pIStream >> value;
            artifact = value;
        } else {
            unsigned char value;
            *_m_pIStream >> value;
            artifact = value;
        }
        artifacts.insert(TArtifact(artifact));
    }
    return new TQuestBringArtifacts(artifacts);
}

VA(0x0049680a, 0xb8)
TQuest* TQuestReader::readBringCreatures()
{
    unsigned char value;
    *_m_pIStream >> value;
    unsigned int numCreatures = value;
    TQuestBringCreatures::TCreatures creatures;
    for (unsigned int i = 0; i < numCreatures; ++i) {
        unsigned short type;
        *_m_pIStream >> type;
        int& quantity = creatures[TCreatureType(type)];
        unsigned short value;
        *_m_pIStream >> value;
        quantity = value;
    }
    return new TQuestBringCreatures(creatures);
}

VA(0x004968c2, 0x63)
TQuest* TQuestReader::readBringResources()
{
    TArray<int, kNumGameResourceTypes> aQuantity;
    for (int type = 0; type < kNumGameResourceTypes; ++type) {
        unsigned int quantity;
        *_m_pIStream >> quantity;
        aQuantity[type] = quantity;
    }
    return new TQuestBringResources(aQuantity);
}

VA(0x00496925, 0x45)
TQuest* TQuestReader::readBeASpecificHero()
{
    unsigned char value;
    *_m_pIStream >> value;
    int heroID = value;
    return new TQuestBeASpecificHero(heroID);
}

VA(0x0049696a, 0x45)
TQuest* TQuestReader::readBelongToASpecificPlayer()
{
    unsigned char value;
    *_m_pIStream >> value;
    TPlayer player = TPlayer(value);
    return new TQuestBelongToASpecificPlayer(player);
}

VA(0x004969af, 0x7e)
TQuestLocation::TQuestLocation(const TObjectType& objType) : TGameObject(objType), _m_deadline(-1)
{
}

VA(0x00496a2d, 0x8b)
TQuestLocation::TQuestLocation(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType)
{
    read(pIStream, version);
}

VA(0x00496ab8, 0xec)
TQuestLocation::TQuestLocation(const TQuestLocation& other)
    : TGameObject(other), _m_deadline(other._m_deadline), _m_aMessage(other._m_aMessage)
{
    if (other.getPQuest() != NULL) {
        _m_pQuest = other.getPQuest()->clone();
        if (_m_pQuest.get() == NULL)
            throw TAllocationFailure();
    }
}

VA(0x00496ba4, 0x19)
void TQuestLocation::setQuest(std::auto_ptr<TQuest> pQuest)
{
    _m_pQuest = pQuest;
}

VA(0x00496bbd, 0x2d)
void TQuestLocation::clearQuest()
{
    _m_pQuest = std::auto_ptr<TQuest>();
    resetQuestTerms();
}

VA(0x00496bea, 0x1f)
void TQuestLocation::setMessage(unsigned int i, const std::string& newMessage)
{
    _m_aMessage[i] = newMessage;
}

VA(0x00496c09, 0x14b)
void TQuestLocation::read(TRawIStream* pIStream, int version)
{
    TQuestReader reader(pIStream, version);
    _m_pQuest = reader.read();
    if (_m_pQuest.get() != NULL) {
        long deadline;
        *pIStream >> deadline;
        _m_deadline = deadline;
        if (version >= 19) {
            std::string message;
            for (unsigned int i = 0; i < s_kNumMessages; ++i) {
                *pIStream >> message;
                _m_aMessage[i] = message;
            }
        } else {
            std::string message;
            *pIStream >> message;
            _m_aMessage[0] = message;
        }
    } else {
        _m_deadline = -1;
        for (unsigned int i = 0; i < s_kNumMessages; ++i)
            _m_aMessage[i] = std::string();
    }
}

VA(0x00496d54, 0x174)
void TQuestLocation::importText(std::istream* pIStream, EGameVersion version)
{
    std::string line;
    getline(*pIStream, line);
    if (line != std::string(kMessageStr) + ':')
        throw TImportTextFailure();
    for (unsigned int i = 0; i < s_kNumMessages; ++i)
        if (!_m_aMessage[i].empty()) {
            getline(*pIStream, line);
            replace(line.begin(), line.end(), '\t', '\n');
            _m_aMessage[i] = line;
        }
}

VA(0x00496ec8, 0xe)
const std::string& TQuestLocation::getMessage(unsigned int i) const
{
    return _m_aMessage[i];
}

VA(0x00496ed6, 0x5c)
void TQuestLocation::write(TRawOStream* pOStream, int version) const
{
    TQuestWriter writer(pOStream, version);
    writer.write(getPQuest());
    if (getPQuest() != NULL) {
        *pOStream << static_cast<long>(_m_deadline);
        for (unsigned int i = 0; i < s_kNumMessages; ++i)
            *pOStream << _m_aMessage[i];
    }
}

VA(0x00496f32, 0x9)
bool TQuestLocation::isCustomized() const
{
    return getPQuest() != NULL;
}

VA(0x00496f3b, 0x1f)
bool TQuestLocation::hasText() const
{
    if (getPQuest() != NULL)
        for (unsigned int i = 0; i < s_kNumMessages; ++i)
            if (!_m_aMessage[i].empty())
                return true;
    return false;
}

VA(0x00496f5a, 0xec)
void TQuestLocation::exportText(std::ostream* pOStream, EGameVersion version) const
{
    *pOStream << kMessageStr << ':' << '\n';
    for (unsigned int i = 0; i < s_kNumMessages; ++i)
        if (!_m_aMessage[i].empty()) {
            std::string message = _m_aMessage[i];
            replace(message.begin(), message.end(), '\n', '\t');
            *pOStream << message.c_str();
            *pOStream << '\n';
        }
}

VA(0x00497046, 0x62)
void TQuestLocation::resetQuestTerms()
{
    _m_deadline = -1;
    for (unsigned int i = 0; i < s_kNumMessages; ++i)
        _m_aMessage[i] = std::string();
}
