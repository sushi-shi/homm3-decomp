// Town.cpp - Loki h3maped object 35: the town type traits (each type's
// building traits and creature generators, named from towntype.txt and the
// building and dwelling spreadsheets), the town's generator bonuses and
// timed events, and the town on the map. Assert and throw lines come from
// the retail immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <functional>
#include <iostream.h>
#include <string>
#include <vector>

#include "adventureobjecttype.h"
#include "autoarrayptr.h"
#include "creaturetype.h"
#include "exceptions.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"
#include "editor/Town.h"

#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

namespace {
// A generator of one town type's creatures.
class TAbsoluteGeneratorTraits : public TTown::TGeneratorTraits {
public:
    TAbsoluteGeneratorTraits(TCreatureType baseType, TCreatureType upgradeType)
        : _m_baseType(baseType), _m_upgradeType(upgradeType)
    {
#line 62
        assert(baseType >= 0 && baseType < kNumCreatureTypes);
        assert(upgradeType >= 0 && upgradeType < kNumCreatureTypes);
    }

    virtual const char* getBaseCreatureName() const { return akCreatureTypeTraits[_m_baseType].m_name; }
    virtual const char* getUpgradeCreatureName() const { return akCreatureTypeTraits[_m_upgradeType].m_name; }

private:
    TCreatureType _m_baseType;
    TCreatureType _m_upgradeType;
};

// A random town's generator, named by level from crgenerc.txt.
class TIndeterminateGeneratorTraits : public TTown::TGeneratorTraits {
public:
    TIndeterminateGeneratorTraits(TTown::TGeneratorType type) : _m_type(type)
    {
#line 84
        assert(type >= 0 && type < TTown::s_kNumGeneratorTypes);
    }

    static void initialize();

    virtual const char* getBaseCreatureName() const { return _s_akBaseName[_m_type]; }
    virtual const char* getUpgradeCreatureName() const { return _s_akUpgradeName[_m_type]; }

private:
    static const char* _s_akBaseName[TTown::s_kNumGeneratorTypes];
    static const char* _s_akUpgradeName[TTown::s_kNumGeneratorTypes];

    TTown::TGeneratorType _m_type;
};

const char* TIndeterminateGeneratorTraits::_s_akBaseName[TTown::s_kNumGeneratorTypes];
const char* TIndeterminateGeneratorTraits::_s_akUpgradeName[TTown::s_kNumGeneratorTypes];

void TIndeterminateGeneratorTraits::initialize()
{
    static TAutoArrayPtr<char> pNames(0);
    {
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("crgenerc.txt"));
        if (!pTextResource.get())
#line 114
            throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"crgenerc.txt\".");
        assert(pTextResource->GetNumberOfStrings() >= TTown::s_kNumGeneratorTypes * 2);
        int size = 0;
        unsigned int i;
        for (i = 0; i < TTown::s_kNumGeneratorTypes * 2; i++)
            size += strlen(pTextResource->GetText(i)) + 1;
        pNames = TAutoArrayPtr<char>(new char[size]);
        if (!pNames.get())
#line 126
            throw TAllocationFailure(__FILE__, __LINE__);
        char* p = pNames.get();
        for (i = 0; i < TTown::s_kNumGeneratorTypes * 2; i++) {
            const char* text = pTextResource->GetText(i);
            int length = strlen(text) + 1;
            memcpy(p, text, length);
            p += length;
        }
    }
    const char* p = pNames.get();
    for (unsigned int type = 0; type < TTown::s_kNumGeneratorTypes; type++) {
        _s_akBaseName[type] = p;
        p += strlen(p) + 1;
        _s_akUpgradeName[type] = p;
        p += strlen(p) + 1;
    }
}

TTown::TBuildingTraits aCastleBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingMageGuild3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingShipyard), TTown::TBuildingTraits(eBuildingTavern), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingDwelling3), TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits castleGenerator1(eCreaturePikeman, eCreatureHalberdier);
const TAbsoluteGeneratorTraits castleGenerator2(eCreatureLightCrossbowman, eCreatureHeavyCrossbowman);
const TAbsoluteGeneratorTraits castleGenerator3(eCreatureGriffin, eCreatureRoyalGriffin);
const TAbsoluteGeneratorTraits castleGenerator4(eCreatureSwordsman, eCreatureCrusader);
const TAbsoluteGeneratorTraits castleGenerator5(eCreatureMonk, eCreatureZealot);
const TAbsoluteGeneratorTraits castleGenerator6(eCreatureCavalier, eCreatureChampion);
const TAbsoluteGeneratorTraits castleGenerator7(eCreatureAngel, eCreatureArchangel);

const TTown::TGeneratorTraits* const apCastleGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &castleGenerator1, &castleGenerator2, &castleGenerator3, &castleGenerator4, &castleGenerator5, &castleGenerator6, &castleGenerator7
};

TTown::TBuildingTraits aRampartBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingMageGuild3),
    TTown::TBuildingTraits(eBuildingMageGuild4), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingSpecial1), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingDwelling5), TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits rampartGenerator1(eCreatureCentaur, eCreatureEliteCentaur);
const TAbsoluteGeneratorTraits rampartGenerator2(eCreatureDwarf, eCreatureBattleDwarf);
const TAbsoluteGeneratorTraits rampartGenerator3(eCreatureWoodElf, eCreatureGrandElf);
const TAbsoluteGeneratorTraits rampartGenerator4(eCreaturePegasus, eCreatureSilverPegasus);
const TAbsoluteGeneratorTraits rampartGenerator5(eCreatureTreefolk, eCreatureBriarTreefolk);
const TAbsoluteGeneratorTraits rampartGenerator6(eCreatureUnicorn, eCreatureWarUnicorn);
const TAbsoluteGeneratorTraits rampartGenerator7(eCreatureGreenDragon, eCreatureGoldDragon);

const TTown::TGeneratorTraits* const apRampartGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &rampartGenerator1, &rampartGenerator2, &rampartGenerator3, &rampartGenerator4, &rampartGenerator5, &rampartGenerator6, &rampartGenerator7
};

TTown::TBuildingTraits aTowerBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingMageGuild3),
    TTown::TBuildingTraits(eBuildingMageGuild4), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingCastle),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits towerGenerator1(eCreatureApprenticeGremlin, eCreatureMasterGremlin);
const TAbsoluteGeneratorTraits towerGenerator2(eCreatureStoneGargoyle, eCreatureObsidianGargoyle);
const TAbsoluteGeneratorTraits towerGenerator3(eCreatureStoneGolem, eCreatureIronGolem);
const TAbsoluteGeneratorTraits towerGenerator4(eCreatureMage, eCreatureArchMage);
const TAbsoluteGeneratorTraits towerGenerator5(eCreatureGenie, eCreatureCaliph);
const TAbsoluteGeneratorTraits towerGenerator6(eCreatureNagaSentinel, eCreatureNagaGuardian);
const TAbsoluteGeneratorTraits towerGenerator7(eCreatureLesserTitan, eCreatureGreaterTitan);

const TTown::TGeneratorTraits* const apTowerGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &towerGenerator1, &towerGenerator2, &towerGenerator3, &towerGenerator4, &towerGenerator5, &towerGenerator6, &towerGenerator7
};

TTown::TBuildingTraits aInfernoBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingMageGuild3),
    TTown::TBuildingTraits(eBuildingMageGuild4), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingMageGuild1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingDwelling3), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits infernoGenerator1(eCreatureImp, eCreatureFamiliar);
const TAbsoluteGeneratorTraits infernoGenerator2(eCreatureGog, eCreatureMagog);
const TAbsoluteGeneratorTraits infernoGenerator3(eCreatureHellHound, eCreatureCerberus);
const TAbsoluteGeneratorTraits infernoGenerator4(eCreatureSingleHornedDemon, eCreatureDualHornedDemon);
const TAbsoluteGeneratorTraits infernoGenerator5(eCreaturePitFiend, eCreaturePitFoe);
const TAbsoluteGeneratorTraits infernoGenerator6(eCreatureEfreet, eCreatureEfreetSultan);
const TAbsoluteGeneratorTraits infernoGenerator7(eCreatureDevil, eCreatureArchDevil);

const TTown::TGeneratorTraits* const apInfernoGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &infernoGenerator1, &infernoGenerator2, &infernoGenerator3, &infernoGenerator4, &infernoGenerator5, &infernoGenerator6, &infernoGenerator7
};

TTown::TBuildingTraits aNecropolisBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingMageGuild3),
    TTown::TBuildingTraits(eBuildingMageGuild4), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits necropolisGenerator1(eCreatureSkeleton, eCreatureSkeletonWarrior);
const TAbsoluteGeneratorTraits necropolisGenerator2(eCreatureZombie, eCreatureZombieLord);
const TAbsoluteGeneratorTraits necropolisGenerator3(eCreatureWight, eCreatureWraith);
const TAbsoluteGeneratorTraits necropolisGenerator4(eCreatureVampire, eCreatureNosferatu);
const TAbsoluteGeneratorTraits necropolisGenerator5(eCreatureLich, eCreaturePowerLich);
const TAbsoluteGeneratorTraits necropolisGenerator6(eCreatureBlackKnight, eCreatureBlackLord);
const TAbsoluteGeneratorTraits necropolisGenerator7(eCreatureBoneDragon, eCreatureGhostDragon);

const TTown::TGeneratorTraits* const apNecropolisGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &necropolisGenerator1, &necropolisGenerator2, &necropolisGenerator3, &necropolisGenerator4, &necropolisGenerator5, &necropolisGenerator6, &necropolisGenerator7
};

TTown::TBuildingTraits aDungeonBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingMageGuild3),
    TTown::TBuildingTraits(eBuildingMageGuild4), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits dungeonGenerator1(eCreatureTroglodyte, eCreatureInfernalTroglodyte);
const TAbsoluteGeneratorTraits dungeonGenerator2(eCreatureHarpy, eCreatureHarpyHag);
const TAbsoluteGeneratorTraits dungeonGenerator3(eCreatureBeholder, eCreatureEvilEye);
const TAbsoluteGeneratorTraits dungeonGenerator4(eCreatureMedusa, eCreatureMedusaQueen);
const TAbsoluteGeneratorTraits dungeonGenerator5(eCreatureMinotaur, eCreatureMinotaurKing);
const TAbsoluteGeneratorTraits dungeonGenerator6(eCreatureManticore, eCreatureScorpicore);
const TAbsoluteGeneratorTraits dungeonGenerator7(eCreatureRedDragon, eCreatureBlackDragon);

const TTown::TGeneratorTraits* const apDungeonGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &dungeonGenerator1, &dungeonGenerator2, &dungeonGenerator3, &dungeonGenerator4, &dungeonGenerator5, &dungeonGenerator6, &dungeonGenerator7
};

TTown::TBuildingTraits aStrongholdBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingBlacksmith),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits strongholdGenerator1(eCreatureGoblin, eCreatureHobgoblin);
const TAbsoluteGeneratorTraits strongholdGenerator2(eCreatureGoblinWolfRider, eCreatureHobgoblinWolfRider);
const TAbsoluteGeneratorTraits strongholdGenerator3(eCreatureOrc, eCreatureOrcChieftain);
const TAbsoluteGeneratorTraits strongholdGenerator4(eCreatureOgre, eCreatureOgreMage);
const TAbsoluteGeneratorTraits strongholdGenerator5(eCreatureRoc, eCreatureThunderbird);
const TAbsoluteGeneratorTraits strongholdGenerator6(eCreatureCyclops, eCreatureCyclopsLord);
const TAbsoluteGeneratorTraits strongholdGenerator7(eCreatureYoungBehemoth, eCreatureAncientBehemoth);

const TTown::TGeneratorTraits* const apStrongholdGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &strongholdGenerator1, &strongholdGenerator2, &strongholdGenerator3, &strongholdGenerator4, &strongholdGenerator5, &strongholdGenerator6, &strongholdGenerator7
};

TTown::TBuildingTraits aFortressBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TAbsoluteGeneratorTraits fortressGenerator1(eCreatureGnoll, eCreatureGnollMarauder);
const TAbsoluteGeneratorTraits fortressGenerator2(eCreaturePrimitiveLizardman, eCreatureAdvancedLizardman);
const TAbsoluteGeneratorTraits fortressGenerator3(eCreatureSerpentFly, eCreatureDragonFly);
const TAbsoluteGeneratorTraits fortressGenerator4(eCreatureBasilisk, eCreatureGreaterBasilisk);
const TAbsoluteGeneratorTraits fortressGenerator5(eCreatureCopperGorgon, eCreatureBronzeGorgon);
const TAbsoluteGeneratorTraits fortressGenerator6(eCreatureWyvern, eCreatureWyvernMonarch);
const TAbsoluteGeneratorTraits fortressGenerator7(eCreatureHydra, eCreatureChaosHydra);

const TTown::TGeneratorTraits* const apFortressGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &fortressGenerator1, &fortressGenerator2, &fortressGenerator3, &fortressGenerator4, &fortressGenerator5, &fortressGenerator6, &fortressGenerator7
};

TTown::TBuildingTraits aRandomBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall), TTown::TBuildingTraits(eBuildingCityHall),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2), TTown::TBuildingTraits(eBuildingMageGuild3),
    TTown::TBuildingTraits(eBuildingMageGuild4), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingDwelling3), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingDwelling5), TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling6),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling7)
};

const TIndeterminateGeneratorTraits randomGenerator1(TTown::TGeneratorType(0));
const TIndeterminateGeneratorTraits randomGenerator2(TTown::TGeneratorType(1));
const TIndeterminateGeneratorTraits randomGenerator3(TTown::TGeneratorType(2));
const TIndeterminateGeneratorTraits randomGenerator4(TTown::TGeneratorType(3));
const TIndeterminateGeneratorTraits randomGenerator5(TTown::TGeneratorType(4));
const TIndeterminateGeneratorTraits randomGenerator6(TTown::TGeneratorType(5));
const TIndeterminateGeneratorTraits randomGenerator7(TTown::TGeneratorType(6));

const TTown::TGeneratorTraits* const apRandomGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &randomGenerator1, &randomGenerator2, &randomGenerator3, &randomGenerator4, &randomGenerator5, &randomGenerator6, &randomGenerator7
};

TTown::TTypeTraits aTownTypeTraitsImp[kNumTownTypes + 1] = {
    TTown::TTypeTraits(aCastleBuildingTraits, apCastleGeneratorTraits),
    TTown::TTypeTraits(aRampartBuildingTraits, apRampartGeneratorTraits),
    TTown::TTypeTraits(aTowerBuildingTraits, apTowerGeneratorTraits),
    TTown::TTypeTraits(aInfernoBuildingTraits, apInfernoGeneratorTraits),
    TTown::TTypeTraits(aNecropolisBuildingTraits, apNecropolisGeneratorTraits),
    TTown::TTypeTraits(aDungeonBuildingTraits, apDungeonGeneratorTraits),
    TTown::TTypeTraits(aStrongholdBuildingTraits, apStrongholdGeneratorTraits),
    TTown::TTypeTraits(aFortressBuildingTraits, apFortressGeneratorTraits),
    TTown::TTypeTraits(aRandomBuildingTraits, apRandomGeneratorTraits)
};
}

const TTown::TTypeTraits* TTown::s_akTypeTraits = aTownTypeTraitsImp;

bool TTown::TTypeTraits::hasMageGuildLevel(unsigned int level) const
{
    static const TBuilding akGuildLevelBuilding[] = {
        eBuildingMageGuild1, eBuildingMageGuild2, eBuildingMageGuild3, eBuildingMageGuild4, eBuildingMageGuild5
    };
#line 775
    assert(level < ARRAY_SIZE( akGuildLevelBuilding ));
    return !m_akBuildingTraits[akGuildLevelBuilding[level]].isDisallowed();
}

void TTown::TGeneratorBonuses::set(TGeneratorType type, unsigned int newBonus)
{
#line 786
    assert(type >= 0 && type < TTown::s_kNumGeneratorTypes);
    assert(newBonus <= s_kMax);
    _m_bonuses[type] = newBonus;
}

unsigned int TTown::TGeneratorBonuses::get(TGeneratorType type) const
{
#line 795
    assert(type >= 0 && type < TTown::s_kNumGeneratorTypes);
    return _m_bonuses[type];
}

TRawOStream& operator<<(TRawOStream& stream, const TTown::TGeneratorBonuses& bonuses)
{
    for (unsigned int type = 0; type < TTown::s_kNumGeneratorTypes; type++)
        stream << (short) bonuses.get(TTown::TGeneratorType(type));
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TTown::TGeneratorBonuses& bonuses)
{
    for (unsigned int type = 0; type < TTown::s_kNumGeneratorTypes; type++) {
        short bonus;
        stream >> bonus;
        bonuses.set(TTown::TGeneratorType(type), bonus);
    }
    return stream;
}

TRawOStream& operator<<(TRawOStream& stream, const TTown::TTimedEvent& event)
{
    stream << static_cast<const ::TTimedEvent&>(event);
    unsigned char aBuildMask[6];
    fill_n(aBuildMask, sizeof(aBuildMask), 0);
    const bitset<kNumBuildings>& buildMask = event.getBuildMask();
    for (unsigned int building = 0; building < kNumBuildings; building++)
        if (buildMask[building])
            aBuildMask[building / 8] |= 1 << building % 8;
    stream << aBuildMask << event.getGeneratorBonuses();
    signed char aReserved[4];
    fill_n(aReserved, sizeof(aReserved), 0);
    stream << aReserved;
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TTown::TTimedEvent& event)
{
    stream >> static_cast< ::TTimedEvent&>(event);
    unsigned char aBuildMask[6];
    stream >> aBuildMask;
    bitset<kNumBuildings> buildMask;
    for (unsigned int building = 0; building < kNumBuildings; building++)
        buildMask[building] = (aBuildMask[building / 8] >> building % 8) & 1;
    event.setBuildMask(buildMask);
    TTown::TGeneratorBonuses generatorBonuses;
    stream >> generatorBonuses;
    event.setGeneratorBonuses(generatorBonuses);
    signed char aReserved[4];
    stream >> aReserved;
    return stream;
}

void TTown::initialize()
{
#line 885
    assert(kRandomTownStr != NULL);
    {
        static TAutoArrayPtr<char> pNames(0);
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("towntype.txt"));
        if (!pTextResource.get())
#line 893
            throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"towntype.txt\".");
        assert(pTextResource->GetNumberOfStrings() >= kNumTownTypes);
        int size = 0;
        unsigned int i;
        for (i = 0; i < kNumTownTypes; i++)
            size += strlen(pTextResource->GetText(i)) + 1;
        pNames = TAutoArrayPtr<char>(new char[size]);
        if (!pNames.get())
#line 905
            throw TAllocationFailure(__FILE__, __LINE__);
        char* p = pNames.get();
        for (i = 0; i < kNumTownTypes; i++) {
            const char* text = pTextResource->GetText(i);
            int length = strlen(text) + 1;
            memcpy(p, text, length);
            aTownTypeTraitsImp[i].m_pName = p;
            p += length;
        }
    }
    static TAutoArrayPtr<char> pRandomName(new char[strlen(kRandomTownStr) + 1]);
    if (!pRandomName.get())
#line 922
        throw TAllocationFailure(__FILE__, __LINE__);
    strcpy(pRandomName.get(), kRandomTownStr);
    aTownTypeTraitsImp[kNumTownTypes].m_pName = pRandomName.get();

    enum TTextTableID {
        eTextTableNone = -1,
        eTextTableNeutral,
        eTextTableSpecial,
        eTextTableDwelling,
        kNumTextTables
    };
    const char* akTextTableName[kNumTextTables];
    akTextTableName[eTextTableNeutral] = "bldgneut.txt";
    akTextTableName[eTextTableSpecial] = "bldgspec.txt";
    akTextTableName[eTextTableDwelling] = "dwelling.txt";
    static TAutoArrayPtr<char> apTextTable[kNumTextTables];
    vector<char*> aapBldgTextEntry[kNumTextTables];
    for (unsigned int table = 0; table < kNumTextTables; table++) {
        TResourcePtr<TSpreadsheetResource> pSpreadsheet(ResourceManager::GetSpreadsheet(akTextTableName[table]));
        if (!pSpreadsheet.get())
#line 954
            throw TRuntimeError(__FILE__, __LINE__, string("Unable to load \"") + akTextTableName[table] + "\".");
        unsigned int numRows = pSpreadsheet->GetNumberOfRows();
        int size = 0;
        unsigned int row;
        for (row = 0; row < numRows; row++)
            if (pSpreadsheet->GetNumberOfColumns(row) >= 2)
                size += strlen(pSpreadsheet->GetRow(row)[0]) + strlen(pSpreadsheet->GetRow(row)[1]) + 2;
        apTextTable[table] = TAutoArrayPtr<char>(new char[size]);
        if (!apTextTable[table].get())
#line 969
            throw TAllocationFailure(__FILE__, __LINE__);
        aapBldgTextEntry[table].reserve(numRows);
        char* p = apTextTable[table].get();
        for (row = 0; row < numRows; row++) {
            aapBldgTextEntry[table].push_back(p);
            if (pSpreadsheet->GetNumberOfColumns(row) >= 2) {
                const char* text = pSpreadsheet->GetRow(row)[0];
                unsigned int length = strlen(text) + 1;
                memcpy(p, text, length);
                p += length;
                text = pSpreadsheet->GetRow(row)[1];
                length = strlen(text) + 1;
                memcpy(p, text, length);
                p += length;
            }
        }
    }

    struct TBuildingTextRef {
        TTextTableID m_tableID;
        unsigned int m_index;
    };
    static const TBuildingTextRef akBuildingTextRef[kNumTownTypes + 1][kNumBuildings] = {
        {   // Castle
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 19 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 10 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNone, 0 },
            { eTextTableSpecial, 3 }, { eTextTableSpecial, 9 }, { eTextTableSpecial, 0 }, { eTextTableSpecial, 5 },
            { eTextTableSpecial, 4 }, { eTextTableNone, 0 }, { eTextTableDwelling, 0 }, { eTextTableDwelling, 7 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 1 }, { eTextTableDwelling, 8 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 2 }, { eTextTableDwelling, 9 }, { eTextTableSpecial, 1 }, { eTextTableDwelling, 3 },
            { eTextTableDwelling, 10 }, { eTextTableNone, 0 }, { eTextTableDwelling, 4 }, { eTextTableDwelling, 11 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 5 }, { eTextTableDwelling, 12 }, { eTextTableDwelling, 6 },
            { eTextTableDwelling, 13 }
        },
        {   // Rampart
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 20 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 21 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNeutral, 4 },
            { eTextTableNone, 0 }, { eTextTableSpecial, 20 }, { eTextTableSpecial, 11 }, { eTextTableSpecial, 15 },
            { eTextTableSpecial, 16 }, { eTextTableNone, 0 }, { eTextTableDwelling, 14 }, { eTextTableDwelling, 21 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 15 }, { eTextTableDwelling, 22 }, { eTextTableSpecial, 12 },
            { eTextTableDwelling, 16 }, { eTextTableDwelling, 23 }, { eTextTableNone, 0 }, { eTextTableDwelling, 17 },
            { eTextTableDwelling, 24 }, { eTextTableNone, 0 }, { eTextTableDwelling, 18 }, { eTextTableDwelling, 25 },
            { eTextTableSpecial, 18 }, { eTextTableDwelling, 19 }, { eTextTableDwelling, 26 }, { eTextTableDwelling, 20 },
            { eTextTableDwelling, 27 }
        },
        {   // Tower
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 21 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 32 }, { eTextTableSpecial, 22 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNeutral, 4 },
            { eTextTableNone, 0 }, { eTextTableSpecial, 31 }, { eTextTableSpecial, 27 }, { eTextTableSpecial, 28 },
            { eTextTableSpecial, 26 }, { eTextTableNone, 0 }, { eTextTableDwelling, 28 }, { eTextTableDwelling, 35 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 29 }, { eTextTableDwelling, 36 }, { eTextTableSpecial, 23 },
            { eTextTableDwelling, 30 }, { eTextTableDwelling, 37 }, { eTextTableNone, 0 }, { eTextTableDwelling, 31 },
            { eTextTableDwelling, 38 }, { eTextTableNone, 0 }, { eTextTableDwelling, 32 }, { eTextTableDwelling, 39 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 33 }, { eTextTableDwelling, 40 }, { eTextTableDwelling, 34 },
            { eTextTableDwelling, 41 }
        },
        {   // Inferno
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 22 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 43 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNeutral, 4 },
            { eTextTableNone, 0 }, { eTextTableSpecial, 42 }, { eTextTableSpecial, 37 }, { eTextTableSpecial, 38 },
            { eTextTableSpecial, 39 }, { eTextTableNone, 0 }, { eTextTableDwelling, 42 }, { eTextTableDwelling, 49 },
            { eTextTableSpecial, 34 }, { eTextTableDwelling, 43 }, { eTextTableDwelling, 50 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 44 }, { eTextTableDwelling, 51 }, { eTextTableSpecial, 40 }, { eTextTableDwelling, 45 },
            { eTextTableDwelling, 52 }, { eTextTableNone, 0 }, { eTextTableDwelling, 46 }, { eTextTableDwelling, 53 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 47 }, { eTextTableDwelling, 54 }, { eTextTableDwelling, 48 },
            { eTextTableDwelling, 55 }
        },
        {   // Necropolis
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 23 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 54 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNeutral, 4 },
            { eTextTableSpecial, 47 }, { eTextTableSpecial, 53 }, { eTextTableSpecial, 44 }, { eTextTableSpecial, 48 },
            { eTextTableSpecial, 49 }, { eTextTableNone, 0 }, { eTextTableDwelling, 56 }, { eTextTableDwelling, 63 },
            { eTextTableSpecial, 45 }, { eTextTableDwelling, 57 }, { eTextTableDwelling, 64 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 58 }, { eTextTableDwelling, 65 }, { eTextTableNone, 0 }, { eTextTableDwelling, 59 },
            { eTextTableDwelling, 66 }, { eTextTableNone, 0 }, { eTextTableDwelling, 60 }, { eTextTableDwelling, 67 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 61 }, { eTextTableDwelling, 68 }, { eTextTableDwelling, 62 },
            { eTextTableDwelling, 69 }
        },
        {   // Dungeon
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 24 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 65 }, { eTextTableSpecial, 55 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNeutral, 4 },
            { eTextTableNone, 0 }, { eTextTableSpecial, 64 }, { eTextTableSpecial, 59 }, { eTextTableSpecial, 60 },
            { eTextTableSpecial, 61 }, { eTextTableNone, 0 }, { eTextTableDwelling, 70 }, { eTextTableDwelling, 77 },
            { eTextTableSpecial, 56 }, { eTextTableDwelling, 71 }, { eTextTableDwelling, 78 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 72 }, { eTextTableDwelling, 79 }, { eTextTableNone, 0 }, { eTextTableDwelling, 73 },
            { eTextTableDwelling, 80 }, { eTextTableNone, 0 }, { eTextTableDwelling, 74 }, { eTextTableDwelling, 81 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 75 }, { eTextTableDwelling, 82 }, { eTextTableDwelling, 76 },
            { eTextTableDwelling, 83 }
        },
        {   // Stronghold
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 25 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 76 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNone, 0 }, { eTextTableNone, 0 },
            { eTextTableNone, 0 }, { eTextTableSpecial, 75 }, { eTextTableSpecial, 66 }, { eTextTableSpecial, 70 },
            { eTextTableSpecial, 71 }, { eTextTableSpecial, 72 }, { eTextTableDwelling, 84 }, { eTextTableDwelling, 91 },
            { eTextTableSpecial, 67 }, { eTextTableDwelling, 85 }, { eTextTableDwelling, 92 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 86 }, { eTextTableDwelling, 93 }, { eTextTableNone, 0 }, { eTextTableDwelling, 87 },
            { eTextTableDwelling, 94 }, { eTextTableNone, 0 }, { eTextTableDwelling, 88 }, { eTextTableDwelling, 95 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 89 }, { eTextTableDwelling, 96 }, { eTextTableDwelling, 90 },
            { eTextTableDwelling, 97 }
        },
        {   // Fortress
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 26 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 87 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNone, 0 }, { eTextTableNone, 0 },
            { eTextTableSpecial, 80 }, { eTextTableSpecial, 86 }, { eTextTableSpecial, 82 }, { eTextTableSpecial, 81 },
            { eTextTableSpecial, 77 }, { eTextTableNone, 0 }, { eTextTableDwelling, 98 }, { eTextTableDwelling, 105 },
            { eTextTableSpecial, 78 }, { eTextTableDwelling, 99 }, { eTextTableDwelling, 106 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 100 }, { eTextTableDwelling, 107 }, { eTextTableNone, 0 }, { eTextTableDwelling, 101 },
            { eTextTableDwelling, 108 }, { eTextTableNone, 0 }, { eTextTableDwelling, 102 }, { eTextTableDwelling, 109 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 103 }, { eTextTableDwelling, 110 }, { eTextTableDwelling, 104 },
            { eTextTableDwelling, 111 }
        },
        {   // Random
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 }, { eTextTableNeutral, 7 },
            { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 }, { eTextTableNeutral, 5 }, { eTextTableNeutral, 27 },
            { eTextTableNeutral, 14 }, { eTextTableSpecial, 98 }, { eTextTableSpecial, 88 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNeutral, 4 },
            { eTextTableSpecial, 91 }, { eTextTableSpecial, 97 }, { eTextTableNone, 0 }, { eTextTableNone, 0 },
            { eTextTableNone, 0 }, { eTextTableNone, 0 }, { eTextTableDwelling, 112 }, { eTextTableDwelling, 119 },
            { eTextTableSpecial, 89 }, { eTextTableDwelling, 113 }, { eTextTableDwelling, 120 }, { eTextTableSpecial, 90 },
            { eTextTableDwelling, 114 }, { eTextTableDwelling, 121 }, { eTextTableSpecial, 94 }, { eTextTableDwelling, 115 },
            { eTextTableDwelling, 122 }, { eTextTableSpecial, 95 }, { eTextTableDwelling, 116 }, { eTextTableDwelling, 123 },
            { eTextTableSpecial, 96 }, { eTextTableDwelling, 117 }, { eTextTableDwelling, 124 }, { eTextTableDwelling, 118 },
            { eTextTableDwelling, 125 }
        }
    };
    static TBuildingTraits (* const apBuildingTraits[kNumTownTypes + 1])[kNumBuildings] = {
        &aCastleBuildingTraits, &aRampartBuildingTraits, &aTowerBuildingTraits, &aInfernoBuildingTraits,
        &aNecropolisBuildingTraits, &aDungeonBuildingTraits, &aStrongholdBuildingTraits, &aFortressBuildingTraits,
        &aRandomBuildingTraits
    };
    for (unsigned int type = 0; type < kNumTownTypes + 1; type++) {
        TBuildingTraits* const aBuildingTraits = *apBuildingTraits[type];
        const TBuildingTextRef* const akTypeTextRef = akBuildingTextRef[type];
        for (unsigned int building = 0; building < kNumBuildings; building++) {
            const TBuildingTextRef& kBuildingTextRef = akTypeTextRef[building];
            if (kBuildingTextRef.m_tableID != eTextTableNone) {
#line 1434
                assert(kBuildingTextRef.m_tableID >= 0 && kBuildingTextRef.m_tableID < kNumTextTables);
                assert(kBuildingTextRef.m_index < aapBldgTextEntry[ kBuildingTextRef.m_tableID ].size());
                const char* pName = aapBldgTextEntry[kBuildingTextRef.m_tableID][kBuildingTextRef.m_index];
                aBuildingTraits[building].m_pName = pName;
                aBuildingTraits[building].m_pDescription = pName + strlen(pName) + 1;
            }
        }
    }
    TIndeterminateGeneratorTraits::initialize();
}

TTown::TTown(const TTown& other)
    : TGameObject(other), TPlayableObject(other), _m_bCustomName(other._m_bCustomName),
      _m_bCustomGarrison(other._m_bCustomGarrison), _m_bCustomBuildings(other._m_bCustomBuildings),
      _m_bGroupedFormation(other._m_bGroupedFormation), _m_name(other._m_name), _m_garrison(other._m_garrison),
      _m_aBuildingState(other._m_aBuildingState), _m_disabledSpellsMask(other._m_disabledSpellsMask),
      _m_events(other._m_events), _m_pVisitingHero(NULL)
{
    if (other._m_pVisitingHero != NULL) {
        TGameObject* pClone = other._m_pVisitingHero->clone(operator new);
        if (pClone == NULL)
#line 1466
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pVisitingHero = dynamic_cast<THero*>(pClone);
#line 1469
        assert(_m_pVisitingHero != NULL);
    }
}

TTown::TTown(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TPlayableObject(objType, owner), _m_bCustomName(false), _m_bCustomGarrison(false),
      _m_bCustomBuildings(false), _m_bGroupedFormation(false), _m_pVisitingHero(NULL)
{
#line 1483
    assert(objType.getType() == TOWN || objType.getType() == RANDOM_TOWN);
    assert(objType.getType() == RANDOM_TOWN || getTownType() >= 0 && getTownType() < kNumTownTypes);
    _m_aBuildingState[eBuildingFort].setBBuilt(true);
}

TTown::TTown(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TPlayableObject(objType, pIStream, version), _m_pVisitingHero(NULL)
{
#line 1495
    assert(objType.getType() == TOWN || objType.getType() == RANDOM_TOWN);
    assert(objType.getType() == RANDOM_TOWN || getTownType() >= 0 && getTownType() < kNumTownTypes);
    signed char bCustom;
    *pIStream >> bCustom;
    setBCustomName(bCustom != 0);
    if (_m_bCustomName) {
        string name;
        *pIStream >> name;
        if (name.size() > s_kMaxNameLen)
            name.erase(s_kMaxNameLen);
        setName(name);
    }
    if (version >= 7)
        *pIStream >> bCustom;
    else
        bCustom = 1;
    setBCustomGarrison(bCustom != 0);
    if (_m_bCustomGarrison) {
        TArmy garrison;
        *pIStream >> garrison;
        setGarrison(garrison);
    }
    signed char bGroupedFormation;
    *pIStream >> bGroupedFormation;
    setBGroupedFormation(bGroupedFormation != 0);
    if (version >= 7)
        *pIStream >> bCustom;
    else
        bCustom = 1;
    setBCustomBuildings(bCustom != 0);
    if (_m_bCustomBuildings) {
        unsigned char aBuilt[6];
        unsigned char aDisabled[6];
        *pIStream >> aBuilt >> aDisabled;
        TArray<TBuildingState, kNumBuildings> aBuildingState;
        for (unsigned int building = 0; building < kNumBuildings; building++) {
            unsigned int index = building / 8;
            unsigned char mask = 1 << building % 8;
            aBuildingState[building].setBBuilt((aBuilt[index] & mask) != 0);
            aBuildingState[building].setBDisabled((aDisabled[index] & mask) != 0);
        }
        setBuildingStates(aBuildingState);
    } else {
        signed char bFortBuilt;
        *pIStream >> bFortBuilt;
        TArray<TBuildingState, kNumBuildings> aBuildingState;
        aBuildingState[eBuildingFort].setBBuilt(bFortBuilt != 0);
        setBuildingStates(aBuildingState);
    }
    unsigned char aDisabledSpells[9];
    *pIStream >> aDisabledSpells;
    bitset<kNumSpells> disabledSpellsMask;
    TTownType type = getTownType();
    const TTypeTraits& typeTraits = getTownTypeTraits();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        disabledSpellsMask[spell] = (aDisabledSpells[spell / 8] >> spell % 8) & 1;
        if (disabledSpellsMask[spell]
            && (akSpellTraits[spell].m_school == 0 || !typeTraits.hasMageGuildLevel(akSpellTraits[spell].m_level - 1)
                || type < kNumTownTypes && akSpellTraits[spell].m_townGetsItChance[type] <= 0))
            disabledSpellsMask[spell] = false;
    }
    setDisabledSpellsMask(disabledSpellsMask);
    vector<TTimedEvent> events;
    long numEvents;
    *pIStream >> numEvents;
    events.reserve(numEvents);
    while (numEvents > 0) {
        --numEvents;
        TTimedEvent event;
        *pIStream >> event;
        events.push_back(event);
    }
    setTimedEvents(events);
    signed char aReserved[3];
    *pIStream >> aReserved;
}

TTown::~TTown()
{
    delete _m_pVisitingHero;
}

void TTown::setName(const string& newName)
{
#line 1605
    assert(!_m_bCustomName || std::find_if( newName.begin(), newName.end(), std::not1( std::ptr_fun( ::isspace ) ) ) != newName.end());
    assert(newName.size() <= s_kMaxNameLen);
    assert(newName.find( '\n' ) == std::string::npos);
    assert(newName.find( '\t' ) == std::string::npos);
    _m_name = newName;
}

void TTown::setGarrison(const TArmy& newGarrison)
{
    _m_garrison = newGarrison;
}

void TTown::setBuildingStates(const TArray<TBuildingState, kNumBuildings>& newBuildingStates)
{
    const TBuildingTraits* akBuildingTraits = getTownTypeTraits().m_akBuildingTraits;
    for (unsigned int building = 0; building < kNumBuildings; building++) {
#line 1636
        assert(!newBuildingStates[ building ].getBDisabled() || !newBuildingStates[ building ].getBBuilt());
        assert(!akBuildingTraits[ building ].isDisallowed() || !newBuildingStates[ building ].getBBuilt());
    }
    _m_aBuildingState = newBuildingStates;
}

void TTown::setDisabledSpellsMask(const bitset<kNumSpells>& newMask)
{
    TTownType type = getTownType();
    const TTypeTraits& typeTraits = getTownTypeTraits();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (akSpellTraits[spell].m_school != 0 && newMask[spell]) {
#line 1655
            assert(typeTraits.hasMageGuildLevel( akSpellTraits[ spell ].m_level - 1 ));
            assert(type >= kNumTownTypes || akSpellTraits[ spell ].m_townGetsItChance[ type ] > 0);
        }
    }
    _m_disabledSpellsMask = newMask;
}

void TTown::setTimedEvents(const vector<TTimedEvent>& newTimedEvents)
{
#line 1667
    assert(newTimedEvents.size() <= s_kMaxTimedEvents);
    _m_events = newTimedEvents;
}

void TTown::setVisitingHero(const THero* pNewVisitingHero)
{
    delete _m_pVisitingHero;
    if (pNewVisitingHero != NULL) {
#line 1680
        assert(dynamic_cast< TNonRandomHero const * >( pNewVisitingHero ) != NULL || dynamic_cast< TRandomHero const * >( pNewVisitingHero ) != NULL);
        TGameObject* pClone = pNewVisitingHero->clone(operator new);
        if (pClone == NULL)
#line 1684
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pVisitingHero = dynamic_cast<THero*>(pClone);
#line 1687
        assert(_m_pVisitingHero != NULL);
    } else {
        _m_pVisitingHero = NULL;
    }
}

void TTown::importText(istream* pIStream)
{
#line 1696
    assert(pIStream != NULL);
    string line;
    if (_m_bCustomName) {
        getline(*pIStream, line);
        if (line != string(kNameStr) + ':')
            throw TImportTextFailure();
        getline(*pIStream, line);
        if (line.size() > s_kMaxNameLen)
            line.erase(s_kMaxNameLen);
        replace(line.begin(), line.end(), '\t', ' ');
        if (_m_bCustomName && find_if(line.begin(), line.end(), not1(ptr_fun(isspace))) == line.end())
            throw TImportTextFailure();
        setName(line);
    }
    if (_m_pVisitingHero != NULL && _m_pVisitingHero->hasText()) {
        getline(*pIStream, line);
        if (!line.empty())
            throw TImportTextFailure();
        getline(*pIStream, line);
        char buffer[256];
        sprintf(buffer, (string(kVisitingHeroFmtStr) + ':').c_str(), _m_pVisitingHero->getTypeName().c_str());
        if (line != buffer)
            throw TImportTextFailure();
        _m_pVisitingHero->importText(pIStream);
    }
    if (_m_events.size() != 0) {
        getline(*pIStream, line);
        if (!line.empty())
            throw TImportTextFailure();
        getline(*pIStream, line);
        if (line != string(kTimedEventsStr) + ':')
            throw TImportTextFailure();
        for (vector<TTimedEvent>::iterator iter = _m_events.begin(); iter != _m_events.end(); ++iter) {
            getline(*pIStream, line);
            if (!line.empty())
                throw TImportTextFailure();
            try {
                iter->importText(pIStream);
            } catch (const TTimedEvent::TImportTextFailure& failure) {
                throw TImportTextFailure();
            }
        }
    }
}

TTownType TTown::getTownType() const
{
    return getType() != RANDOM_TOWN ? TTownType(getExtra()) : kNumTownTypes;
}

bool TTown::getBIsBuildingDisabled(TBuilding building) const
{
#line 1767
    assert(building >= 0 && building < kNumBuildings);
    TBuilding requiredBuilding = getTownTypeTraits().m_akBuildingTraits[building].m_building;
    return _m_bCustomBuildings && (_m_aBuildingState[building].getBDisabled()
                                   || requiredBuilding != eBuildingNone && getBIsBuildingDisabled(requiredBuilding));
}

bool TTown::isCustomized() const
{
    bool bSpellsDisabled = false;
    for (int spell = 0; spell < kNumSpells; spell++)
        if (_m_disabledSpellsMask[spell])
            bSpellsDisabled = true;
    return _m_bCustomName || _m_bCustomGarrison || _m_bCustomBuildings || !_m_aBuildingState[eBuildingFort].getBBuilt()
           || _m_bGroupedFormation || bSpellsDisabled || !_m_events.empty() || _m_pVisitingHero != NULL;
}

void TTown::write(TRawOStream* pOStream) const
{
    TPlayableObject::write(pOStream);
    *pOStream << (signed char) _m_bCustomName;
    if (_m_bCustomName)
        *pOStream << _m_name;
    *pOStream << (signed char) _m_bCustomGarrison;
    if (_m_bCustomGarrison)
        *pOStream << getGarrison();
    *pOStream << (signed char) (getBGroupedFormation() ? 1 : 0);
    *pOStream << (signed char) _m_bCustomBuildings;
    if (_m_bCustomBuildings) {
        unsigned char aBuilt[6];
        unsigned char aDisabled[6];
        fill_n(aBuilt, sizeof(aBuilt), 0);
        fill_n(aDisabled, sizeof(aDisabled), 0);
        for (unsigned int building = 0; building < kNumBuildings; building++) {
            unsigned int index = building / 8;
            unsigned char mask = 1 << building % 8;
            if (_m_aBuildingState[building].getBDisabled())
                aDisabled[index] |= mask;
            else if (_m_aBuildingState[building].getBBuilt())
                aBuilt[index] |= mask;
        }
        *pOStream << aBuilt << aDisabled;
    } else {
        *pOStream << (signed char) _m_aBuildingState[eBuildingFort].getBBuilt();
    }
    unsigned char aDisabledSpells[9];
    fill_n(aDisabledSpells, sizeof(aDisabledSpells), 0);
    for (unsigned int spell = 0; spell < kNumSpells; spell++)
        if (_m_disabledSpellsMask[spell])
            aDisabledSpells[spell / 8] |= 1 << spell % 8;
    *pOStream << aDisabledSpells;
    *pOStream << (long) _m_events.size();
    for (vector<TTimedEvent>::const_iterator iter = _m_events.begin(); iter != _m_events.end(); ++iter)
        *pOStream << *iter;
    signed char aReserved[3];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

string TTown::getTypeName() const
{
    if (getType() == TOWN)
        return s_akTypeTraits[getExtra()].m_pName;
    return TGameObject::getTypeName();
}

bool TTown::hasText() const
{
    return _m_bCustomName || _m_pVisitingHero != NULL && _m_pVisitingHero->hasText() || _m_events.size() != 0;
}

void TTown::exportText(ostream* pOStream) const
{
#line 1874
    assert(pOStream != NULL);
    if (_m_bCustomName)
        *pOStream << kNameStr << ':' << '\n' << getName() << '\n';
    if (_m_pVisitingHero != NULL && _m_pVisitingHero->hasText()) {
        char buffer[256];
        sprintf(buffer, (string(kVisitingHeroFmtStr) + ':').c_str(), _m_pVisitingHero->getTypeName().c_str());
        *pOStream << '\n' << buffer << '\n';
        _m_pVisitingHero->exportText(pOStream);
    }
    if (_m_events.size() != 0) {
        *pOStream << '\n' << kTimedEventsStr << ':' << '\n';
        for (vector<TTimedEvent>::const_iterator iter = _m_events.begin(); iter != _m_events.end(); ++iter) {
            *pOStream << '\n';
            iter->exportText(pOStream);
        }
    }
}
