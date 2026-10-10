// VictoryCondition.cpp - comparing victory and loss conditions through
// their visitors (h3maped 0x4cbd22..0x4cc5ad; Loki h3maped object 36).
// Equivalence dispatches on the left kind, then on the right, and
// compares two conditions of the same kind with their operator==. The
// Windows release has no cloners here, and its testers keep Loki's
// dispatchers' members under the names RTTI records.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/VictoryCondition.h"

namespace {

template<class T>
class TVictoryConditionRHSEquivalencyTester : public TVictoryCondition::TVisitor {
public:
    TVictoryConditionRHSEquivalencyTester(const T& lhs) : _m_lhs(lhs), _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

    virtual void visit(const T& rhs) { _m_bEquivalent = _m_lhs == rhs; }

private:
    const T& _m_lhs;
    bool _m_bEquivalent;
};

class TVictoryConditionEquivalencyTester : public TVictoryCondition::TVisitor {
public:
    TVictoryConditionEquivalencyTester() : _m_bEquivalent(false) {}

    bool test(const TVictoryCondition& lhs, const TVictoryCondition& rhs)
    {
        _m_pRhs = &rhs;
        lhs.accept(this);
        return _m_bEquivalent;
    }

private:
    template<class T>
    void _test(const T& lhs)
    {
        TVictoryConditionRHSEquivalencyTester<T> tester(lhs);
        _m_pRhs->accept(&tester);
        _m_bEquivalent = tester.getBEquivalent();
    }

public:
    virtual void visit(const TVCAquireArtifact& lhs) { _test(lhs); }
    virtual void visit(const TVCAccumulateCreature& lhs) { _test(lhs); }
    virtual void visit(const TVCAccumulateResource& lhs) { _test(lhs); }
    virtual void visit(const TVCUpgradeTown& lhs) { _test(lhs); }
    virtual void visit(const TVCBuildHolyGrailStruct& lhs) { _test(lhs); }
    virtual void visit(const TVCDefeatHero& lhs) { _test(lhs); }
    virtual void visit(const TVCCaptureTown& lhs) { _test(lhs); }
    virtual void visit(const TVCDefeatMonster& lhs) { _test(lhs); }
    virtual void visit(const TVCFlagAllCreatureGenerators& lhs) { _test(lhs); }
    virtual void visit(const TVCFlagAllMines& lhs) { _test(lhs); }
    virtual void visit(const TVCTransportArtifact& lhs) { _test(lhs); }

private:
    const TVictoryCondition* _m_pRhs;
    bool _m_bEquivalent;
};

template<class T>
class TLossConditionRHSEquivalencyTester : public TLossCondition::TVisitor {
public:
    TLossConditionRHSEquivalencyTester(const T& lhs) : _m_lhs(lhs), _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

    virtual void visit(const T& rhs) { _m_bEquivalent = _m_lhs == rhs; }

private:
    const T& _m_lhs;
    bool _m_bEquivalent;
};

class TLossConditionEquivalencyTester : public TLossCondition::TVisitor {
public:
    TLossConditionEquivalencyTester() : _m_bEquivalent(false) {}

    bool test(const TLossCondition& lhs, const TLossCondition& rhs)
    {
        _m_pRhs = &rhs;
        lhs.accept(this);
        return _m_bEquivalent;
    }

private:
    template<class T>
    void _test(const T& lhs)
    {
        TLossConditionRHSEquivalencyTester<T> tester(lhs);
        _m_pRhs->accept(&tester);
        _m_bEquivalent = tester.getBEquivalent();
    }

public:
    virtual void visit(const TLCLoseTown& lhs) { _test(lhs); }
    virtual void visit(const TLCLoseHero& lhs) { _test(lhs); }
    virtual void visit(const TLCTimeExpires& lhs) { _test(lhs); }

private:
    const TLossCondition* _m_pRhs;
    bool _m_bEquivalent;
};

}  // namespace

VA(0x004cbd3e, 0x3d)
bool TVictoryCondition::equivalent(const TVictoryCondition& lhs, const TVictoryCondition& rhs)
{
    TVictoryConditionEquivalencyTester tester;
    return tester.test(lhs, rhs);
}

VA(0x004cbdff, 0x3d)
bool TLossCondition::equivalent(const TLossCondition& lhs, const TLossCondition& rhs)
{
    TLossConditionEquivalencyTester tester;
    return tester.test(lhs, rhs);
}
