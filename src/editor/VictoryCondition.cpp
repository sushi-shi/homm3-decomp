// VictoryCondition.cpp - Loki h3maped object 36: cloning and comparing
// victory and loss conditions through their visitors. The cloner
// copy-constructs the visited kind into storage from the caller's
// allocator; equivalence dispatches on the left kind, then on the right,
// and compares two conditions of the same kind with their operator==.
#include <assert.h>
#include <new>
#include <stdexcept>

#include "editor/VictoryCondition.h"

namespace {

class TVictoryConditionCloner : public TVictoryCondition::TVisitor {
public:
    TVictoryConditionCloner(void* (*pfnAllocator)(unsigned int))
        : _m_pfnAllocator(pfnAllocator), _m_pClone(NULL)
    {
#line 40
        assert(_m_pfnAllocator != __null);
    }
    ~TVictoryConditionCloner()
    {
#line 42
        assert(_m_pClone == __null);
    }

private:
    template<class T>
    void _clone(const T& vc)
    {
        _m_pClone = new ((*_m_pfnAllocator)(sizeof(T))) T(vc);
    }

public:
    TVictoryCondition* release()
    {
        TVictoryCondition* result = _m_pClone;
        _m_pClone = NULL;
        return result;
    }

    virtual void visit(const TVCAquireArtifact& vc) { _clone(vc); }
    virtual void visit(const TVCAccumulateCreature& vc) { _clone(vc); }
    virtual void visit(const TVCAccumulateResource& vc) { _clone(vc); }
    virtual void visit(const TVCUpgradeTown& vc) { _clone(vc); }
    virtual void visit(const TVCBuildHolyGrailStruct& vc) { _clone(vc); }
    virtual void visit(const TVCDefeatHero& vc) { _clone(vc); }
    virtual void visit(const TVCCaptureTown& vc) { _clone(vc); }
    virtual void visit(const TVCDefeatMonster& vc) { _clone(vc); }
    virtual void visit(const TVCFlagAllCreatureGenerators& vc) { _clone(vc); }
    virtual void visit(const TVCFlagAllMines& vc) { _clone(vc); }
    virtual void visit(const TVCTransportArtifact& vc) { _clone(vc); }

private:
    void* (*_m_pfnAllocator)(unsigned int);
    TVictoryCondition* _m_pClone;
};

class TVictoryConditionRHSDispatcherBase : public TVictoryCondition::TVisitor {
public:
    TVictoryConditionRHSDispatcherBase() : _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

    virtual void visit(const TVCAquireArtifact& rhs) {}
    virtual void visit(const TVCAccumulateCreature& rhs) {}
    virtual void visit(const TVCAccumulateResource& rhs) {}
    virtual void visit(const TVCUpgradeTown& rhs) {}
    virtual void visit(const TVCBuildHolyGrailStruct& rhs) {}
    virtual void visit(const TVCDefeatHero& rhs) {}
    virtual void visit(const TVCCaptureTown& rhs) {}
    virtual void visit(const TVCDefeatMonster& rhs) {}
    virtual void visit(const TVCFlagAllCreatureGenerators& rhs) {}
    virtual void visit(const TVCFlagAllMines& rhs) {}
    virtual void visit(const TVCTransportArtifact& rhs) {}

protected:
    bool _m_bEquivalent;
};

template<class T>
class TVictoryConditionRHSDispatcher : public TVictoryConditionRHSDispatcherBase {
public:
    TVictoryConditionRHSDispatcher(const T& lhs) : _m_lhs(lhs) {}

    virtual void visit(const T& rhs) { _m_bEquivalent = _m_lhs == rhs; }

private:
    const T& _m_lhs;
};

class TVictoryConditionLHSDispatcher : public TVictoryCondition::TVisitor {
public:
    TVictoryConditionLHSDispatcher(const TVictoryCondition& rhs) : _m_rhs(rhs) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

private:
    template<class T>
    void _rhsDispatch(const T& lhs)
    {
        TVictoryConditionRHSDispatcher<T> rhsDispatcher(lhs);
        _m_rhs.accept(&rhsDispatcher);
        _m_bEquivalent = rhsDispatcher.getBEquivalent();
    }

public:
    virtual void visit(const TVCAquireArtifact& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCAccumulateCreature& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCAccumulateResource& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCUpgradeTown& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCBuildHolyGrailStruct& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCDefeatHero& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCCaptureTown& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCDefeatMonster& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCFlagAllCreatureGenerators& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCFlagAllMines& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TVCTransportArtifact& lhs) { _rhsDispatch(lhs); }

private:
    bool _m_bEquivalent;
    const TVictoryCondition& _m_rhs;
};

class TLossConditionCloner : public TLossCondition::TVisitor {
public:
    TLossConditionCloner(void* (*pfnAllocator)(unsigned int))
        : _m_pfnAllocator(pfnAllocator), _m_pClone(NULL)
    {
        if (_m_pfnAllocator == NULL)
            _m_pfnAllocator = ::operator new;
    }
    ~TLossConditionCloner()
    {
#line 158
        assert(_m_pClone == __null);
    }

private:
    template<class T>
    void _clone(const T& lc)
    {
        _m_pClone = new ((*_m_pfnAllocator)(sizeof(T))) T(lc);
    }

public:
    TLossCondition* release()
    {
        TLossCondition* result = _m_pClone;
        _m_pClone = NULL;
        return result;
    }

    virtual void visit(const TLCLoseTown& lc) { _clone(lc); }
    virtual void visit(const TLCLoseHero& lc) { _clone(lc); }
    virtual void visit(const TLCTimeExpires& lc) { _clone(lc); }

private:
    void* (*_m_pfnAllocator)(unsigned int);
    TLossCondition* _m_pClone;
};

class TLossConditionRHSDispatcherBase : public TLossCondition::TVisitor {
public:
    TLossConditionRHSDispatcherBase() : _m_bEquivalent(false) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

    virtual void visit(const TLCLoseTown& rhs) {}
    virtual void visit(const TLCLoseHero& rhs) {}
    virtual void visit(const TLCTimeExpires& rhs) {}

protected:
    bool _m_bEquivalent;
};

template<class T>
class TLossConditionRHSDispatcher : public TLossConditionRHSDispatcherBase {
public:
    TLossConditionRHSDispatcher(const T& lhs) : _m_lhs(lhs) {}

    virtual void visit(const T& rhs) { _m_bEquivalent = _m_lhs == rhs; }

private:
    const T& _m_lhs;
};

class TLossConditionLHSDispatcher : public TLossCondition::TVisitor {
public:
    TLossConditionLHSDispatcher(const TLossCondition& rhs) : _m_rhs(rhs) {}

    bool getBEquivalent() const { return _m_bEquivalent; }

private:
    template<class T>
    void _rhsDispatch(const T& lhs)
    {
        TLossConditionRHSDispatcher<T> rhsDispatcher(lhs);
        _m_rhs.accept(&rhsDispatcher);
        _m_bEquivalent = rhsDispatcher.getBEquivalent();
    }

public:
    virtual void visit(const TLCLoseTown& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TLCLoseHero& lhs) { _rhsDispatch(lhs); }
    virtual void visit(const TLCTimeExpires& lhs) { _rhsDispatch(lhs); }

private:
    bool _m_bEquivalent;
    const TLossCondition& _m_rhs;
};

}  // namespace

TVictoryCondition* TVictoryCondition::clone(const TVictoryCondition& vc, void* (*pfnAllocator)(unsigned int))
{
    TVictoryConditionCloner cloner(pfnAllocator);
    vc.accept(&cloner);
    return cloner.release();
}

bool TVictoryCondition::equivalent(const TVictoryCondition& lhs, const TVictoryCondition& rhs)
{
    TVictoryConditionLHSDispatcher lhsDispatcher(rhs);
    lhs.accept(&lhsDispatcher);
    return lhsDispatcher.getBEquivalent();
}

TLossCondition* TLossCondition::clone(const TLossCondition& lc, void* (*pfnAllocator)(unsigned int))
{
    TLossConditionCloner cloner(pfnAllocator);
    lc.accept(&cloner);
    return cloner.release();
}

bool TLossCondition::equivalent(const TLossCondition& lhs, const TLossCondition& rhs)
{
    TLossConditionLHSDispatcher lhsDispatcher(rhs);
    lhs.accept(&lhsDispatcher);
    return lhsDispatcher.getBEquivalent();
}
