// Campaign.cpp - the campaign model: scenarios, their starting options and
// the bonuses those options offer.
#include "campaign_editor/stdafx.h"

#include "va.h"
#include "campaign_editor/Campaign.h"

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
