// VictoryCondition.h - the map's victory and loss conditions (Loki h3maped
// VictoryCondition.cpp). Each condition kind is a class with an accept for
// its visitor; VictoryCondition.cpp clones and compares conditions with
// visitors (double dispatch for the comparison). A victory condition
// carries whether the normal victory still applies and whether the
// computer players can win by it; the kinds fix one or both where the
// editor offers no choice (retail's constructors pass the constants).
// The vtable orders are retail's (__vt_Q217TVictoryCondition8TVisitor and
// the anonymous RHS dispatchers). A loss condition records its kind in
// an ordinal (eLCNone and kNumLossConditionTypes are the assert spellings;
// the three kinds are named after their classes). Member names follow
// the getters; THallLevel/TCastleLevel's enumerators are not recovered.
#ifndef HOMM3_EDITOR_VICTORYCONDITION_H
#define HOMM3_EDITOR_VICTORYCONDITION_H

#include "artifact.h"
#include "creaturetype.h"
#include "armygrp.h"
#include "artifact_type.h"
#include "editor/GameResource.h"
#include "editor/MapObjectRef.h"

class TVCAquireArtifact;
class TVCAccumulateCreature;
class TVCAccumulateResource;
class TVCUpgradeTown;
class TVCBuildHolyGrailStruct;
class TVCDefeatHero;
class TVCCaptureTown;
class TVCDefeatMonster;
class TVCFlagAllCreatureGenerators;
class TVCFlagAllMines;
class TVCTransportArtifact;

class TVictoryCondition {
public:
    class TVisitor {
    public:
        virtual ~TVisitor() {}
        virtual void visit(const TVCAquireArtifact& vc) = 0;
        virtual void visit(const TVCAccumulateCreature& vc) = 0;
        virtual void visit(const TVCAccumulateResource& vc) = 0;
        virtual void visit(const TVCUpgradeTown& vc) = 0;
        virtual void visit(const TVCBuildHolyGrailStruct& vc) = 0;
        virtual void visit(const TVCDefeatHero& vc) = 0;
        virtual void visit(const TVCCaptureTown& vc) = 0;
        virtual void visit(const TVCDefeatMonster& vc) = 0;
        virtual void visit(const TVCFlagAllCreatureGenerators& vc) = 0;
        virtual void visit(const TVCFlagAllMines& vc) = 0;
        virtual void visit(const TVCTransportArtifact& vc) = 0;
    };

    TVictoryCondition(bool bAllowNormalVictory, bool bAppliesToComputer)
        : _m_bAllowNormalVictory(bAllowNormalVictory), _m_bAppliesToComputer(bAppliesToComputer) {}
    virtual ~TVictoryCondition() {}

    virtual void accept(TVisitor* pVisitor) const = 0;

    bool getBAllowNormalVictory() const { return _m_bAllowNormalVictory; }
    bool getBAppliesToComputer() const { return _m_bAppliesToComputer; }

    static TVictoryCondition* clone(const TVictoryCondition& vc, void* (*pfnAllocator)(unsigned int));
    static bool equivalent(const TVictoryCondition& lhs, const TVictoryCondition& rhs);

protected:
    static bool equals(const TVictoryCondition& lhs, const TVictoryCondition& rhs)
    {
        return lhs._m_bAllowNormalVictory == rhs._m_bAllowNormalVictory
            && lhs._m_bAppliesToComputer == rhs._m_bAppliesToComputer;
    }

private:
    bool _m_bAllowNormalVictory;
    bool _m_bAppliesToComputer;
};

class TVCAquireArtifact : public TVictoryCondition {
public:
    TVCAquireArtifact(bool bAppliesToComputer, TArtifact artifact)
        : TVictoryCondition(true, bAppliesToComputer), _m_artifact(artifact) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    TArtifact getArtifact() const { return _m_artifact; }

    friend bool operator==(const TVCAquireArtifact& lhs, const TVCAquireArtifact& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_artifact == rhs._m_artifact;
    }

private:
    TArtifact _m_artifact;
};

class TVCAccumulateCreature : public TVictoryCondition {
public:
    TVCAccumulateCreature(bool bAllowNormalVictory, bool bAppliesToComputer, TCreatureType creatureType, unsigned int quantity)
        : TVictoryCondition(bAllowNormalVictory, bAppliesToComputer), _m_creatureType(creatureType), _m_quantity(quantity) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    TCreatureType getCreatureType() const { return _m_creatureType; }
    unsigned int getQuantity() const { return _m_quantity; }

    friend bool operator==(const TVCAccumulateCreature& lhs, const TVCAccumulateCreature& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_creatureType == rhs._m_creatureType
            && lhs._m_quantity == rhs._m_quantity;
    }

private:
    TCreatureType _m_creatureType;
    unsigned int _m_quantity;
};

class TVCAccumulateResource : public TVictoryCondition {
public:
    TVCAccumulateResource(bool bAllowNormalVictory, bool bAppliesToComputer, TGameResourceType resourceType, unsigned int quantity)
        : TVictoryCondition(bAllowNormalVictory, bAppliesToComputer), _m_resourceType(resourceType), _m_quantity(quantity) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    TGameResourceType getResourceType() const { return _m_resourceType; }
    unsigned int getQuantity() const { return _m_quantity; }

    friend bool operator==(const TVCAccumulateResource& lhs, const TVCAccumulateResource& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_resourceType == rhs._m_resourceType
            && lhs._m_quantity == rhs._m_quantity;
    }

private:
    TGameResourceType _m_resourceType;
    unsigned int _m_quantity;
};

class TVCUpgradeTown : public TVictoryCondition {
public:
    enum THallLevel {
    };

    enum TCastleLevel {
    };

    TVCUpgradeTown(bool bAllowNormalVictory, const TMapObjectRef& townRef, THallLevel hallLevel, TCastleLevel castleLevel)
        : TVictoryCondition(bAllowNormalVictory, true), _m_townRef(townRef), _m_hallLevel(hallLevel), _m_castleLevel(castleLevel) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TMapObjectRef& getTownRef() const { return _m_townRef; }
    THallLevel getHallLevel() const { return _m_hallLevel; }
    TCastleLevel getCastleLevel() const { return _m_castleLevel; }

    friend bool operator==(const TVCUpgradeTown& lhs, const TVCUpgradeTown& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_townRef == rhs._m_townRef
            && lhs._m_hallLevel == rhs._m_hallLevel
            && lhs._m_castleLevel == rhs._m_castleLevel;
    }

private:
    TMapObjectRef _m_townRef;
    THallLevel _m_hallLevel;
    TCastleLevel _m_castleLevel;
};

class TVCBuildHolyGrailStruct : public TVictoryCondition {
public:
    TVCBuildHolyGrailStruct(const TMapObjectRef& townRef)
        : TVictoryCondition(true, true), _m_townRef(townRef) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TMapObjectRef& getTownRef() const { return _m_townRef; }

    friend bool operator==(const TVCBuildHolyGrailStruct& lhs, const TVCBuildHolyGrailStruct& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_townRef == rhs._m_townRef;
    }

private:
    TMapObjectRef _m_townRef;
};

class TVCDefeatHero : public TVictoryCondition {
public:
    TVCDefeatHero(const TMapObjectRef& heroRef)
        : TVictoryCondition(false, false), _m_heroRef(heroRef) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TMapObjectRef& getHeroRef() const { return _m_heroRef; }

    friend bool operator==(const TVCDefeatHero& lhs, const TVCDefeatHero& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_heroRef == rhs._m_heroRef;
    }

private:
    TMapObjectRef _m_heroRef;
};

class TVCCaptureTown : public TVictoryCondition {
public:
    TVCCaptureTown(bool bAllowNormalVictory, bool bAppliesToComputer, const TMapObjectRef& townRef)
        : TVictoryCondition(bAllowNormalVictory, bAppliesToComputer), _m_townRef(townRef) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TMapObjectRef& getTownRef() const { return _m_townRef; }

    friend bool operator==(const TVCCaptureTown& lhs, const TVCCaptureTown& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_townRef == rhs._m_townRef;
    }

private:
    TMapObjectRef _m_townRef;
};

class TVCDefeatMonster : public TVictoryCondition {
public:
    TVCDefeatMonster(bool bAllowNormalVictory, const TMapObjectRef& monsterRef)
        : TVictoryCondition(bAllowNormalVictory, true), _m_monsterRef(monsterRef) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TMapObjectRef& getMonsterRef() const { return _m_monsterRef; }

    friend bool operator==(const TVCDefeatMonster& lhs, const TVCDefeatMonster& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_monsterRef == rhs._m_monsterRef;
    }

private:
    TMapObjectRef _m_monsterRef;
};

class TVCFlagAllCreatureGenerators : public TVictoryCondition {
public:
    TVCFlagAllCreatureGenerators(bool bAllowNormalVictory, bool bAppliesToComputer)
        : TVictoryCondition(bAllowNormalVictory, bAppliesToComputer) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TVCFlagAllCreatureGenerators& lhs, const TVCFlagAllCreatureGenerators& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs);
    }
};

class TVCFlagAllMines : public TVictoryCondition {
public:
    TVCFlagAllMines(bool bAllowNormalVictory, bool bAppliesToComputer)
        : TVictoryCondition(bAllowNormalVictory, bAppliesToComputer) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    friend bool operator==(const TVCFlagAllMines& lhs, const TVCFlagAllMines& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs);
    }
};

class TVCTransportArtifact : public TVictoryCondition {
public:
    TVCTransportArtifact(bool bAppliesToComputer, TArtifact artifact, const TMapObjectRef& townRef)
        : TVictoryCondition(true, bAppliesToComputer), _m_artifact(artifact), _m_townRef(townRef) {}

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TArtifact& getArtifact() const { return _m_artifact; }
    const TMapObjectRef& getTownRef() const { return _m_townRef; }

    friend bool operator==(const TVCTransportArtifact& lhs, const TVCTransportArtifact& rhs)
    {
        return TVictoryCondition::equals(lhs, rhs)
            && lhs._m_artifact == rhs._m_artifact
            && lhs._m_townRef == rhs._m_townRef;
    }

private:
    TArtifact _m_artifact;
    TMapObjectRef _m_townRef;
};

// The kinds as the map file numbers them (GameMap.cpp's readers assert
// "vcData.m_type >= eVCNone && vcData.m_type < kNumVictoryConditionTypes"
// and the loss counterpart against -1..10 and -1..2).
enum TVictoryConditionType {
    eVCNone = -1,
    eVCAquireArtifact,
    eVCAccumulateCreature,
    eVCAccumulateResource,
    eVCUpgradeTown,
    eVCBuildHolyGrailStruct,
    eVCDefeatHero,
    eVCCaptureTown,
    eVCDefeatMonster,
    eVCFlagAllCreatureGenerators,
    eVCFlagAllMines,
    eVCTransportArtifact,
    kNumVictoryConditionTypes
};

enum TLossConditionType {
    eLCNone = -1,
    eLCLoseTown,
    eLCLoseHero,
    eLCTimeExpires,
    kNumLossConditionTypes
};

class TLCLoseTown;
class TLCLoseHero;
class TLCTimeExpires;

class TLossCondition {
public:
    class TVisitor {
    public:
        virtual ~TVisitor() {}
        virtual void visit(const TLCLoseTown& lc) = 0;
        virtual void visit(const TLCLoseHero& lc) = 0;
        virtual void visit(const TLCTimeExpires& lc) = 0;
    };

    virtual ~TLossCondition() {}

    virtual void accept(TVisitor* pVisitor) const = 0;

    static TLossCondition* clone(const TLossCondition& lc, void* (*pfnAllocator)(unsigned int));
    static bool equivalent(const TLossCondition& lhs, const TLossCondition& rhs);

protected:
    // Each kind's constructor stores its 1-based ordinal here (0 is no
    // condition); TMapSpecsLossCondPage::OnInitDialog reads it back to pick
    // its radio button. Its name is not recorded.
    friend class TMapSpecsLossCondPage;
    int _m_kind;
};

class TLCLoseTown : public TLossCondition {
public:
    TLCLoseTown(const TMapObjectRef& townRef)
        : _m_townRef(townRef)
    {
        _m_kind = 1;
    }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TMapObjectRef& getTownRef() const { return _m_townRef; }

    friend bool operator==(const TLCLoseTown& lhs, const TLCLoseTown& rhs)
    {
        return lhs._m_townRef == rhs._m_townRef;
    }

private:
    TMapObjectRef _m_townRef;
};

class TLCLoseHero : public TLossCondition {
public:
    TLCLoseHero(const TMapObjectRef& heroRef)
        : _m_heroRef(heroRef)
    {
        _m_kind = 2;
    }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    const TMapObjectRef& getHeroRef() const { return _m_heroRef; }

    friend bool operator==(const TLCLoseHero& lhs, const TLCLoseHero& rhs)
    {
        return lhs._m_heroRef == rhs._m_heroRef;
    }

private:
    TMapObjectRef _m_heroRef;
};

class TLCTimeExpires : public TLossCondition {
public:
    TLCTimeExpires(unsigned int numDays)
        : _m_numDays(numDays)
    {
        _m_kind = 3;
    }

    virtual void accept(TVisitor* pVisitor) const { pVisitor->visit(*this); }

    unsigned int getNumDays() const { return _m_numDays; }

    friend bool operator==(const TLCTimeExpires& lhs, const TLCTimeExpires& rhs)
    {
        return lhs._m_numDays == rhs._m_numDays;
    }

private:
    unsigned int _m_numDays;
};

#endif  /* HOMM3_EDITOR_VICTORYCONDITION_H */
