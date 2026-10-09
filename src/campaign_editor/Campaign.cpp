// Campaign.cpp - the campaign model: scenarios, their starting options and
// the bonuses those options offer.
#include "campaign_editor/stdafx.h"

#include <string.h>

#include "va.h"
#include "bitset_iterator.h"
#include "campaignmap.h"
#include "editor/RawStream.h"
#include "campaign_editor/Campaign.h"

class TCampaign::_TImpl {
public:
    explicit _TImpl(int type);

    unsigned int getNumScenarios() const { return g_campaignMapTraits[m_type].m_numRegions; }

    void setName(const string& newName);
    void setDescription(const string& newDescription);
    void setMusic(int newMusic);
    void setScenarioMap(int scenario, auto_ptr<TCampaignScenarioMap> pMap);
    void removeScenarioMap(int scenario);
    void setBPrerequisite(int scenario, int prerequisite, bool bPrerequisite);
    void setScenarioStartingOptions(int scenario, auto_ptr<TScenarioStartingOptions> pOptions);
    bool getBPrerequisite(int scenario, int prerequisite) const;
    bool getBDirectPrerequisite(int scenario, int prerequisite) const;

    int m_type;
    string m_name;
    string m_description;
    bool m_bDifficultyChoice;
    int m_music;
    vector<TScenario> m_scenarios;
};

class TScenario::_TImpl {
public:
    explicit _TImpl(int numScenarios);
    _TImpl(const _TImpl& other);

    void setStartingOptions(auto_ptr<TScenarioStartingOptions> pOptions);
    void setPrologue(auto_ptr<TScenarioPrologue> pPrologue) { m_pPrologue = pPrologue; }
    void removePrologue() { m_pPrologue = auto_ptr<TScenarioPrologue>(); }
    void setEpilogue(auto_ptr<TScenarioPrologue> pEpilogue) { m_pEpilogue = pEpilogue; }
    void removeEpilogue() { m_pEpilogue = auto_ptr<TScenarioPrologue>(); }

    TRefCountingAutoPtr<TCampaignScenarioMap> m_pMap;
    vector<bool> m_prerequisites;
    int m_regionColor;
    int m_difficulty;
    string m_regionDesc;
    auto_ptr<TScenarioPrologue> m_pPrologue;
    auto_ptr<TScenarioPrologue> m_pEpilogue;
    TScenarioCrossover m_crossover;
    auto_ptr<TScenarioStartingOptions> m_pStartingOptions;
};

namespace {

class TScenarioStartingBonusEquivalencyTester : public TScenarioStartingBonus::TVisitor {
public:
    TScenarioStartingBonusEquivalencyTester() : m_pRhs(NULL), m_bEquivalent(false) {}

    bool test(const TScenarioStartingBonus& lhs, const TScenarioStartingBonus& rhs);

    virtual void visit(const TScenarioBonusSpell& lhs);
    virtual void visit(const TScenarioBonusCreature& lhs);
    virtual void visit(const TScenarioBonusBuilding& lhs);
    virtual void visit(const TScenarioBonusArtifact& lhs);
    virtual void visit(const TScenarioBonusSpellScroll& lhs);
    virtual void visit(const TScenarioBonusPrimarySkill& lhs);
    virtual void visit(const TScenarioBonusSecondarySkill& lhs);
    virtual void visit(const TScenarioBonusResource& lhs);

private:
    const TScenarioStartingBonus* m_pRhs;
    bool m_bEquivalent;
};

template<class T>
class TScenarioStartingBonusRHSEquivalencyTester : public TScenarioStartingBonus::TVisitor {
public:
    explicit TScenarioStartingBonusRHSEquivalencyTester(const T& lhs)
        : m_pLhs(&lhs), m_bEquivalent(false) {}

    virtual void visit(const T& rhs) { m_bEquivalent = *m_pLhs == rhs; }

    bool isEquivalent() const { return m_bEquivalent; }

private:
    const T* m_pLhs;
    bool m_bEquivalent;
};

class TScenarioStartingBonusCloner : public TScenarioStartingBonus::TVisitor {
public:
    auto_ptr<TScenarioStartingBonus> clone(const TScenarioStartingBonus& bonus);

    virtual void visit(const TScenarioBonusSpell& bonus);
    virtual void visit(const TScenarioBonusCreature& bonus);
    virtual void visit(const TScenarioBonusBuilding& bonus);
    virtual void visit(const TScenarioBonusArtifact& bonus);
    virtual void visit(const TScenarioBonusSpellScroll& bonus);
    virtual void visit(const TScenarioBonusPrimarySkill& bonus);
    virtual void visit(const TScenarioBonusSecondarySkill& bonus);
    virtual void visit(const TScenarioBonusResource& bonus);

private:
    template<class T>
    void setClone(const T& bonus)
    {
        m_pClone = auto_ptr<TScenarioStartingBonus>(new T(bonus));
    }

    auto_ptr<TScenarioStartingBonus> m_pClone;
};


class TScenarioStartingOptionsEquivalencyTester : public TScenarioStartingOptions::TVisitor {
public:
    TScenarioStartingOptionsEquivalencyTester() : m_pRhs(NULL), m_bEquivalent(false) {}

    bool test(const TScenarioStartingOptions& lhs, const TScenarioStartingOptions& rhs);

    virtual void visit(const TScenarioOptionsBonus& lhs);
    virtual void visit(const TScenarioOptionsCrossoverScenario& lhs);
    virtual void visit(const TScenarioOptionsStartingHero& lhs);

private:
    const TScenarioStartingOptions* m_pRhs;
    bool m_bEquivalent;
};

template<class T>
class TScenarioStartingOptionsRHSEquivalencyTester : public TScenarioStartingOptions::TVisitor {
public:
    explicit TScenarioStartingOptionsRHSEquivalencyTester(const T& lhs)
        : m_pLhs(&lhs), m_bEquivalent(false) {}

    virtual void visit(const T& rhs) { m_bEquivalent = *m_pLhs == rhs; }

    bool isEquivalent() const { return m_bEquivalent; }

private:
    const T* m_pLhs;
    bool m_bEquivalent;
};

class TScenarioStartingOptionsCloner : public TScenarioStartingOptions::TVisitor {
public:
    auto_ptr<TScenarioStartingOptions> clone(const TScenarioStartingOptions& options);

    virtual void visit(const TScenarioOptionsBonus& options);
    virtual void visit(const TScenarioOptionsCrossoverScenario& options);
    virtual void visit(const TScenarioOptionsStartingHero& options);

private:
    template<class T>
    void setClone(const T& options)
    {
        m_pClone = auto_ptr<TScenarioStartingOptions>(new T(options));
    }

    auto_ptr<TScenarioStartingOptions> m_pClone;
};


// The crossover planes as earlier campaign formats stored them.
enum {
    kNumOldCreatures = 138,
    kNumOldArtifacts = 129,
    kNumOriginalArtifacts = 127
};

// Reads a run of a constant bitset as a range (the counterpart of
// bitset_iterator, whose elements are references).
template<size_t N>
class const_bitset_iterator {
public:
    const_bitset_iterator(const bitset<N>& bits, size_t position = 0)
        : m_bits(&bits), m_position(position) {}

    bool operator*() const { return (*m_bits)[m_position]; }

    const_bitset_iterator& operator++()
    {
        ++m_position;
        return *this;
    }

    friend bool operator!=(const const_bitset_iterator& left, const const_bitset_iterator& right)
    {
        return left.m_bits != right.m_bits || left.m_position != right.m_position;
    }

private:
    const bitset<N>* m_bits;
    size_t m_position;
};

template<size_t N>
inline void readCrossoverBits(TRawIStream& stream, bitset<N>& bits)
{
    unsigned char bytes[(N + 7) / 8];
    stream >> bytes;
    for (unsigned int i = 0; i < N; i++)
        bits[i] = (bytes[i / 8] & (1 << (i % 8))) != 0;
}

template<size_t N>
inline void writeCrossoverBits(TRawOStream& stream, const bitset<N>& bits)
{
    unsigned char bytes[(N + 7) / 8];
    memset(bytes, 0, sizeof(bytes));
    for (unsigned int i = 0; i < N; i++) {
        if (bits[i])
            bytes[i / 8] |= 1 << (i % 8);
    }
    stream << bytes;
}

}

VA(0x00405020, 0x23)
bool TScenarioStartingBonusEquivalencyTester::test(const TScenarioStartingBonus& lhs,
                                                    const TScenarioStartingBonus& rhs)
{
    m_pRhs = &rhs;
    lhs.accept(*this);
    bool bEquivalent = m_bEquivalent;
    m_pRhs = NULL;
    m_bEquivalent = false;
    return bEquivalent;
}

VA(0x00405050, 0x2c)
bool operator==(const TScenarioStartingBonus& lhs, const TScenarioStartingBonus& rhs)
{
    TScenarioStartingBonusEquivalencyTester tester;
    return tester.test(lhs, rhs);
}

VA(0x00405080, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusSpell& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusSpell> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x004050c0, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusCreature& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusCreature> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405100, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusBuilding& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusBuilding> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405140, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusArtifact& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusArtifact> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405180, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusSpellScroll& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusSpellScroll> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x004051c0, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusPrimarySkill& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusPrimarySkill> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405200, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusSecondarySkill& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusSecondarySkill> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405240, 0x36)
void TScenarioStartingBonusEquivalencyTester::visit(const TScenarioBonusResource& lhs)
{
    TScenarioStartingBonusRHSEquivalencyTester<TScenarioBonusResource> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405280, 0x7e)
auto_ptr<TScenarioStartingBonus> TScenarioStartingBonus::clone() const
{
    TScenarioStartingBonusCloner cloner;
    return cloner.clone(*this);
}

VA(0x00405300, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusSpell& bonus)
{
    setClone(bonus);
}

VA(0x00405310, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusCreature& bonus)
{
    setClone(bonus);
}

VA(0x00405320, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusBuilding& bonus)
{
    setClone(bonus);
}

VA(0x00405330, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusArtifact& bonus)
{
    setClone(bonus);
}

VA(0x00405340, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusSpellScroll& bonus)
{
    setClone(bonus);
}

VA(0x00405350, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusPrimarySkill& bonus)
{
    setClone(bonus);
}

VA(0x00405360, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusSecondarySkill& bonus)
{
    setClone(bonus);
}

VA(0x00405370, 0xd)
void TScenarioStartingBonusCloner::visit(const TScenarioBonusResource& bonus)
{
    setClone(bonus);
}

VA(0x00405380, 0x2e)
auto_ptr<TScenarioStartingBonus> TScenarioStartingBonusCloner::clone(const TScenarioStartingBonus& bonus)
{
    bonus.accept(*this);
    return m_pClone;
}

VA(0x004053d0, 0x4e)
TScenarioBonusPrimarySkill::TScenarioBonusPrimarySkill(int hero, const int aSkills[kNumPrimarySkills])
    : TScenarioHeroBonus(hero)
{
    uninitialized_copy(aSkills, aSkills + kNumPrimarySkills, m_skills);
}

VA(0x00405480, 0x20)
TScenarioBonusSecondarySkill::TScenarioBonusSecondarySkill(int hero, int skill, int level)
    : TScenarioHeroBonus(hero), m_skill(skill), m_level(level)
{
}

VA(0x004054b0, 0x19)
TScenarioBonusResource::TScenarioBonusResource(int resource, int amount)
    : m_resource(resource), m_amount(amount)
{
}

bool TScenarioStartingOptionsEquivalencyTester::test(const TScenarioStartingOptions& lhs,
                                                     const TScenarioStartingOptions& rhs)
{
    m_pRhs = &rhs;
    lhs.accept(*this);
    bool bEquivalent = m_bEquivalent;
    m_pRhs = NULL;
    m_bEquivalent = false;
    return bEquivalent;
}

VA(0x00405510, 0x2c)
bool operator==(const TScenarioStartingOptions& lhs, const TScenarioStartingOptions& rhs)
{
    TScenarioStartingOptionsEquivalencyTester tester;
    return tester.test(lhs, rhs);
}

VA(0x00405540, 0x36)
void TScenarioStartingOptionsEquivalencyTester::visit(const TScenarioOptionsBonus& lhs)
{
    TScenarioStartingOptionsRHSEquivalencyTester<TScenarioOptionsBonus> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405580, 0x36)
void TScenarioStartingOptionsEquivalencyTester::visit(const TScenarioOptionsCrossoverScenario& lhs)
{
    TScenarioStartingOptionsRHSEquivalencyTester<TScenarioOptionsCrossoverScenario> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x004055c0, 0x36)
void TScenarioStartingOptionsEquivalencyTester::visit(const TScenarioOptionsStartingHero& lhs)
{
    TScenarioStartingOptionsRHSEquivalencyTester<TScenarioOptionsStartingHero> rhsTester(lhs);
    m_pRhs->accept(rhsTester);
    m_bEquivalent = rhsTester.isEquivalent();
}

VA(0x00405600, 0x7e)
auto_ptr<TScenarioStartingOptions> TScenarioStartingOptions::clone() const
{
    TScenarioStartingOptionsCloner cloner;
    return cloner.clone(*this);
}

VA(0x00405680, 0xd)
void TScenarioStartingOptionsCloner::visit(const TScenarioOptionsBonus& options)
{
    setClone(options);
}

VA(0x00405690, 0xd)
void TScenarioStartingOptionsCloner::visit(const TScenarioOptionsCrossoverScenario& options)
{
    setClone(options);
}

VA(0x004056a0, 0xd)
void TScenarioStartingOptionsCloner::visit(const TScenarioOptionsStartingHero& options)
{
    setClone(options);
}

auto_ptr<TScenarioStartingOptions> TScenarioStartingOptionsCloner::clone(const TScenarioStartingOptions& options)
{
    options.accept(*this);
    return m_pClone;
}

VA(0x004056b0, 0x1a0)
TScenarioOptionsBonus::TScenarioOptionsBonus(const TScenarioOptionsBonus& other)
    : m_player(other.m_player)
{
    m_bonuses.reserve(other.m_bonuses.size());
    for (vector<auto_ptr<TScenarioStartingBonus> >::const_iterator it = other.m_bonuses.begin();
         it != other.m_bonuses.end(); ++it) {
        auto_ptr<TScenarioStartingBonus> pBonus = (*it)->clone();
        if (pBonus.get() == NULL)
            throw TAllocationFailure();
        m_bonuses.push_back(pBonus);
    }
}

VA(0x00405920, 0xb4)
TScenarioOptionsBonus::TScenarioOptionsBonus(int player,
                                             const vector<auto_ptr<TScenarioStartingBonus> >& bonuses)
    : m_player(player), m_bonuses(bonuses)
{
}

VA(0x004059e0, 0x8d)
void TScenarioOptionsBonus::removeBonus(int index)
{
    m_bonuses.erase(m_bonuses.begin() + index);
}

VA(0x00405a70, 0x7f)
bool operator==(const TScenarioOptionsBonus& lhs, const TScenarioOptionsBonus& rhs)
{
    if (lhs.m_player != rhs.m_player || lhs.m_bonuses.size() != rhs.m_bonuses.size())
        return false;
    for (unsigned int i = 0; i < lhs.m_bonuses.size(); i++) {
        if (!(*lhs.m_bonuses[i] == *rhs.m_bonuses[i]))
            return false;
    }
    return true;
}

VA(0x00405bc0, 0x3f)
void TScenarioOptionsCrossoverScenario::removeChoice(int index)
{
    m_choices.erase(m_choices.begin() + index);
}

void TScenarioOptionsStartingHero::removeChoice(int index)
{
    m_choices.erase(m_choices.begin() + index);
}

VA(0x00405ce0, 0x195)
TCampaign::_TImpl::_TImpl(int type)
    : m_type(type), m_bDifficultyChoice(false), m_music(0x22)
{
    unsigned int numScenarios = getNumScenarios();
    m_scenarios.resize(numScenarios, TScenario(numScenarios));
}

VA(0x00407180, 0x131)
void TCampaign::_TImpl::setName(const string& newName)
{
    m_name = newName;
}

VA(0x004072c0, 0x131)
void TCampaign::_TImpl::setDescription(const string& newDescription)
{
    m_description = newDescription;
}

VA(0x00407400, 0xa)
void TCampaign::_TImpl::setMusic(int newMusic)
{
    m_music = newMusic;
}

VA(0x00407410, 0x63)
void TCampaign::_TImpl::setScenarioMap(int scenario, auto_ptr<TCampaignScenarioMap> pMap)
{
    m_scenarios[scenario].setMap(pMap);
}

VA(0x00407480, 0xe4)
void TCampaign::_TImpl::removeScenarioMap(int scenario)
{
    TScenario& rScenario = m_scenarios[scenario];
    rScenario.removeMap();
    for (unsigned int other = 0; other < getNumScenarios(); other++) {
        if (other == scenario)
            continue;
        if (getBPrerequisite(scenario, other))
            rScenario.setBPrerequisite(other, false);
        TScenario& rOther = m_scenarios[other];
        if (!getBPrerequisite(other, scenario))
            continue;
        rOther.setBPrerequisite(scenario, false);
        const TScenarioOptionsCrossoverScenario* pCrossover =
            dynamic_cast<const TScenarioOptionsCrossoverScenario*>(rOther.getStartingOptions());
        if (pCrossover == NULL)
            continue;
        for (unsigned int choice = pCrossover->m_choices.size(); choice > 0;) {
            --choice;
            if (pCrossover->m_choices[choice].m_scenario == scenario) {
                rOther.removeStartingOptionsChoice(choice);
                break;
            }
        }
    }
}

VA(0x00407570, 0x147)
void TCampaign::_TImpl::setBPrerequisite(int scenario, int prerequisite, bool bPrerequisite)
{
    TScenario& rScenario = m_scenarios[scenario];
    if (bPrerequisite == rScenario.getBPrerequisite(prerequisite))
        return;
    if (bPrerequisite) {
        unsigned int other;
        for (other = 0; other < getNumScenarios(); other++) {
            if (other != scenario && other != prerequisite && getBDirectPrerequisite(other, scenario))
                setBPrerequisite(other, prerequisite, true);
        }
        for (other = 0; other < getNumScenarios(); other++) {
            if (other != scenario && other != prerequisite && getBPrerequisite(prerequisite, other))
                rScenario.setBPrerequisite(other, true);
        }
    } else {
        const TScenarioOptionsCrossoverScenario* pCrossover =
            dynamic_cast<const TScenarioOptionsCrossoverScenario*>(rScenario.getStartingOptions());
        if (pCrossover != NULL) {
            for (unsigned int choice = pCrossover->m_choices.size(); choice > 0;) {
                --choice;
                if (pCrossover->m_choices[choice].m_scenario == prerequisite) {
                    rScenario.removeStartingOptionsChoice(choice);
                    break;
                }
            }
        }
    }
    rScenario.setBPrerequisite(prerequisite, bPrerequisite);
}

VA(0x004076c0, 0x63)
void TCampaign::_TImpl::setScenarioStartingOptions(int scenario, auto_ptr<TScenarioStartingOptions> pOptions)
{
    m_scenarios[scenario].setStartingOptions(pOptions);
}

VA(0x00407730, 0x17)
bool TCampaign::_TImpl::getBPrerequisite(int scenario, int prerequisite) const
{
    return m_scenarios[scenario].getBPrerequisite(prerequisite);
}

VA(0x00407750, 0x73)
bool TCampaign::_TImpl::getBDirectPrerequisite(int scenario, int prerequisite) const
{
    if (!getBPrerequisite(scenario, prerequisite))
        return false;
    for (unsigned int other = 0; other < getNumScenarios(); other++) {
        if (other != scenario && other != prerequisite && getBPrerequisite(scenario, other)
            && getBPrerequisite(other, prerequisite))
            return false;
    }
    return true;
}

VA(0x004077e0, 0x18e)
TCampaign::TCampaign(int type)
    : _m_pImpl(_TImpl(type))
{
}

VA(0x00407a50, 0xf2)
TCampaign::~TCampaign()
{
}

VA(0x00407b50, 0x3e)
TCampaign& TCampaign::operator=(const TCampaign& other)
{
    _m_pImpl = other._m_pImpl;
    return *this;
}

VA(0x00407bc0, 0x24)
void TCampaign::setName(const string& newName)
{
    _m_pImpl->setName(newName);
}

VA(0x00407bf0, 0x24)
void TCampaign::setDescription(const string& newDescription)
{
    _m_pImpl->setDescription(newDescription);
}

VA(0x00407c20, 0x2b)
void TCampaign::setBDifficultyChoice(bool bDifficultyChoice)
{
    _m_pImpl->m_bDifficultyChoice = bDifficultyChoice;
}

VA(0x00407c50, 0x24)
void TCampaign::setMusic(int newMusic)
{
    _m_pImpl->setMusic(newMusic);
}

VA(0x00407c80, 0x60)
void TCampaign::setScenarioMap(int scenario, auto_ptr<TCampaignScenarioMap> pMap)
{
    _m_pImpl->setScenarioMap(scenario, pMap);
}

VA(0x00407ce0, 0x24)
void TCampaign::removeScenarioMap(int scenario)
{
    _m_pImpl->removeScenarioMap(scenario);
}

VA(0x00407d10, 0x2e)
void TCampaign::setBPrerequisite(int scenario, int prerequisite, bool bPrerequisite)
{
    _m_pImpl->setBPrerequisite(scenario, prerequisite, bPrerequisite);
}

VA(0x00407d40, 0x60)
void TCampaign::setScenarioStartingOptions(int scenario, auto_ptr<TScenarioStartingOptions> pOptions)
{
    _m_pImpl->setScenarioStartingOptions(scenario, pOptions);
}

VA(0x00407da0, 0x21)
TScenario& TCampaign::getScenario(int scenario)
{
    return _m_pImpl->m_scenarios[scenario];
}

VA(0x00407df0, 0x6)
int TCampaign::getType() const
{
    return _m_pImpl->m_type;
}

VA(0x00407e00, 0x6)
const string& TCampaign::getName() const
{
    return _m_pImpl->m_name;
}

VA(0x00407e10, 0x6)
const string& TCampaign::getDescription() const
{
    return _m_pImpl->m_description;
}

VA(0x00407e20, 0x6)
bool TCampaign::getBDifficultyChoice() const
{
    return _m_pImpl->m_bDifficultyChoice;
}

VA(0x00407e30, 0x6)
int TCampaign::getMusic() const
{
    return _m_pImpl->m_music;
}

VA(0x00407e40, 0xf)
const TScenario& TCampaign::getScenario(int scenario) const
{
    return _m_pImpl->m_scenarios[scenario];
}

VA(0x00407e50, 0x17)
bool TCampaign::getBPrerequisite(int scenario, int prerequisite) const
{
    return _m_pImpl->getBPrerequisite(scenario, prerequisite);
}

VA(0x00407e70, 0x17)
bool TCampaign::getBDirectPrerequisite(int scenario, int prerequisite) const
{
    return _m_pImpl->getBDirectPrerequisite(scenario, prerequisite);
}

VA(0x00407e90, 0x4c)
TScenarioCrossover::TScenarioCrossover()
{
    m_retained.set(eRetainExperience);
    m_retained.set(eRetainPrimarySkills);
    m_retained.set(eRetainSecondarySkills);
    m_retained.set(eRetainSpells);
}

VA(0x00407ee0, 0x36b)
void TScenarioCrossover::read(TRawIStream& stream, int version)
{
    readCrossoverBits(stream, m_retained);
    if (version >= 4) {
        readCrossoverBits(stream, m_creatures);
    } else {
        bitset<kNumOldCreatures> oldCreatures;
        readCrossoverBits(stream, oldCreatures);
        copy(bitset_iterator<kNumOldCreatures>(oldCreatures),
             bitset_iterator<kNumOldCreatures>(oldCreatures, kNumOldCreatures),
             bitset_iterator<kNumCreatures>(m_creatures));
    }
    if (version >= 6) {
        readCrossoverBits(stream, m_artifacts);
    } else if (version >= 3) {
        bitset<kNumOldArtifacts> oldArtifacts;
        readCrossoverBits(stream, oldArtifacts);
        copy(bitset_iterator<kNumOldArtifacts>(oldArtifacts),
             bitset_iterator<kNumOldArtifacts>(oldArtifacts, kNumOldArtifacts),
             bitset_iterator<kNumArtifacts>(m_artifacts));
    } else {
        bitset<kNumOriginalArtifacts> originalArtifacts;
        readCrossoverBits(stream, originalArtifacts);
        copy(bitset_iterator<kNumOriginalArtifacts>(originalArtifacts),
             bitset_iterator<kNumOriginalArtifacts>(originalArtifacts, kNumOriginalArtifacts),
             bitset_iterator<kNumArtifacts>(m_artifacts));
    }
}

VA(0x00408250, 0x1f0)
void TScenarioCrossover::write(TRawOStream& stream, int version) const
{
    writeCrossoverBits(stream, m_retained);
    writeCrossoverBits(stream, m_creatures);
    if (version >= 2) {
        writeCrossoverBits(stream, m_artifacts);
    } else {
        bitset<kNumOldArtifacts> oldArtifacts;
        copy(const_bitset_iterator<kNumArtifacts>(m_artifacts),
             const_bitset_iterator<kNumArtifacts>(m_artifacts, kNumOldArtifacts),
             bitset_iterator<kNumOldArtifacts>(oldArtifacts));
        writeCrossoverBits(stream, oldArtifacts);
    }
}

VA(0x00408440, 0x1c)
TScenarioPrologue::TScenarioPrologue()
    : m_movie(0), m_music(0)
{
}

VA(0x00408460, 0x139)
TScenarioPrologue::TScenarioPrologue(int movie, int music, const string& text)
    : m_movie(movie), m_music(music), m_text(text)
{
}

VA(0x004085a0, 0x219)
TRawIStream& operator>>(TRawIStream& stream, TScenarioPrologue& prologue)
{
    string text;
    signed char movie;
    stream >> movie;
    signed char music;
    stream >> music;
    stream >> text;
    prologue = TScenarioPrologue(movie, music, text);
    return stream;
}

VA(0x004088d0, 0xe4)
TRawOStream& operator<<(TRawOStream& stream, const TScenarioPrologue& prologue)
{
    stream << static_cast<ubyte>(prologue.m_movie);
    stream << static_cast<ubyte>(prologue.m_music);
    stream << prologue.m_text;
    return stream;
}

VA(0x004089c0, 0x336)
TScenario::_TImpl::_TImpl(const _TImpl& other)
    : m_pMap(other.m_pMap), m_prerequisites(other.m_prerequisites),
      m_regionColor(other.m_regionColor), m_difficulty(other.m_difficulty),
      m_regionDesc(other.m_regionDesc), m_crossover(other.m_crossover)
{
    if (m_pMap.get() == NULL)
        return;
    if (other.m_pPrologue.get() != NULL) {
        m_pPrologue = auto_ptr<TScenarioPrologue>(new TScenarioPrologue(*other.m_pPrologue));
        if (m_pPrologue.get() == NULL)
            throw TAllocationFailure();
    }
    if (other.m_pEpilogue.get() != NULL) {
        m_pEpilogue = auto_ptr<TScenarioPrologue>(new TScenarioPrologue(*other.m_pEpilogue));
        if (m_pEpilogue.get() == NULL)
            throw TAllocationFailure();
    }
    m_pStartingOptions = other.m_pStartingOptions->clone();
    if (m_pStartingOptions.get() == NULL)
        throw TAllocationFailure();
}

VA(0x00408d00, 0xc4)
TScenario::_TImpl::_TImpl(int numScenarios)
    : m_pMap(auto_ptr<TCampaignScenarioMap>()), m_prerequisites(numScenarios),
      m_regionColor(0), m_difficulty(1)
{
}

VA(0x00409350, 0x92)
void TScenario::_TImpl::setStartingOptions(auto_ptr<TScenarioStartingOptions> pOptions)
{
    m_pStartingOptions = pOptions;
}

VA(0x00409580, 0x1af)
TScenario::TScenario(int numScenarios)
    : _m_pImpl(_TImpl(numScenarios))
{
}

VA(0x00409850, 0x2b)
void TScenario::setRegionColor(int newRegionColor)
{
    _m_pImpl->m_regionColor = newRegionColor;
}

VA(0x00409880, 0x2b)
void TScenario::setDifficulty(int newDifficulty)
{
    _m_pImpl->m_difficulty = newDifficulty;
}

VA(0x004098b0, 0x143)
void TScenario::setRegionDesc(const string& newRegionDesc)
{
    _m_pImpl->m_regionDesc = newRegionDesc;
}

VA(0x00409a00, 0x5b)
void TScenario::setPrologue(auto_ptr<TScenarioPrologue> pPrologue)
{
    _m_pImpl->setPrologue(pPrologue);
}

VA(0x00409b30, 0x1d)
void TScenario::removePrologue()
{
    _m_pImpl->removePrologue();
}

VA(0x00409bc0, 0x5b)
void TScenario::setEpilogue(auto_ptr<TScenarioPrologue> pEpilogue)
{
    _m_pImpl->setEpilogue(pEpilogue);
}

VA(0x00409cf0, 0x1d)
void TScenario::removeEpilogue()
{
    _m_pImpl->removeEpilogue();
}

VA(0x00409d80, 0x27)
void TScenario::setCrossover(const TScenarioCrossover& newCrossover)
{
    _m_pImpl->m_crossover = newCrossover;
}

VA(0x00409db0, 0x9)
const TCampaignScenarioMap* TScenario::getMap() const
{
    return _m_pImpl->m_pMap.get();
}

VA(0x00409dc0, 0x6)
int TScenario::getRegionColor() const
{
    return _m_pImpl->m_regionColor;
}

VA(0x00409dd0, 0x6)
int TScenario::getDifficulty() const
{
    return _m_pImpl->m_difficulty;
}

VA(0x00409de0, 0x6)
const string& TScenario::getRegionDesc() const
{
    return _m_pImpl->m_regionDesc;
}

VA(0x00409df0, 0x6)
const TScenarioPrologue* TScenario::getPrologue() const
{
    return _m_pImpl->m_pPrologue.get();
}

VA(0x00409e00, 0x6)
const TScenarioPrologue* TScenario::getEpilogue() const
{
    return _m_pImpl->m_pEpilogue.get();
}

VA(0x00409e10, 0x6)
const TScenarioCrossover& TScenario::getCrossover() const
{
    return _m_pImpl->m_crossover;
}

VA(0x00409e20, 0x6)
const TScenarioStartingOptions* TScenario::getStartingOptions() const
{
    return _m_pImpl->m_pStartingOptions.get();
}

VA(0x0040a0a0, 0x25)
void TScenario::setBPrerequisite(int scenario, bool bPrerequisite)
{
    _m_pImpl->m_prerequisites[scenario] = bPrerequisite;
}

VA(0x0040a0d0, 0x5b)
void TScenario::setStartingOptions(auto_ptr<TScenarioStartingOptions> pOptions)
{
    _m_pImpl->setStartingOptions(pOptions);
}

VA(0x0040a160, 0xf)
bool TScenario::getBPrerequisite(int scenario) const
{
    return _m_pImpl->m_prerequisites[scenario];
}
