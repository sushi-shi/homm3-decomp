// TownTypeTraits.cpp - the town type traits (h3maped 0x4c86ae..0x4cb8a8):
// each town type's building traits and creature generators, named from
// towntype.txt, the building spreadsheets and crgenerc.txt. Loki keeps them
// at the top of Town.cpp; the Windows release moves them to their own
// object, adds Conflux, and drops the asserts.
#include "editor/stdafx.h"

#include <string.h>
#include <vector>

#include "va.h"
#include "autoarrayptr.h"
#include "creaturetype.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/MapEditorText.h"
#include "editor/Town.h"

namespace {
// A generator of one town type's creatures.
class TAbsoluteGeneratorTraits : public TTown::TGeneratorTraits {
public:
    TAbsoluteGeneratorTraits(TCreatureType baseType, TCreatureType upgradeType)
        : _m_baseType(baseType), _m_upgradeType(upgradeType)
    {
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
    TIndeterminateGeneratorTraits(TTown::TGeneratorType type) : _m_type(type) {}

    static void initialize();

    virtual const char* getBaseCreatureName() const { return _s_akBaseName[_m_type]; }
    virtual const char* getUpgradeCreatureName() const { return _s_akUpgradeName[_m_type]; }

private:
    static const char* _s_akBaseName[TTown::s_kNumGeneratorTypes];
    static const char* _s_akUpgradeName[TTown::s_kNumGeneratorTypes];

    TTown::TGeneratorType _m_type;
};

DATA(0x005a5824) const char* TIndeterminateGeneratorTraits::_s_akBaseName[TTown::s_kNumGeneratorTypes];
DATA(0x005a67e4) const char* TIndeterminateGeneratorTraits::_s_akUpgradeName[TTown::s_kNumGeneratorTypes];

VA(0x004c86ae, 0x162)
void TIndeterminateGeneratorTraits::initialize()
{
    DATA_COMPGEN_GUARD(0x005a51f8, generatorNamesGuard, pNames)
    VA_COMPGEN(0x004c8810, 0x16, STATIC_DTOR, pNames)
    DATA(0x005a6a58) static TAutoArrayPtr<char> pNames(0);
    {
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("crgenerc.txt"));
        if (!pTextResource.get())
            throw TRuntimeError();
        int size = 0;
        unsigned int i;
        for (i = 0; i < TTown::s_kNumGeneratorTypes * 2; i++)
            size += strlen(pTextResource->GetText(i)) + 1;
        pNames = TAutoArrayPtr<char>(new char[size]);
        if (!pNames.get())
            throw TAllocationFailure();
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

DATA(0x005a5430) TTown::TBuildingTraits aCastleBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingShipyard), TTown::TBuildingTraits(eBuildingTavern),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingDwelling3), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004c882b, 0x36b, STATIC_CTOR, aCastleBuildingTraits)

DATA(0x005a5a70) const TAbsoluteGeneratorTraits castleGenerator1(CREATURE_PIKEMAN, CREATURE_HALBERDIER);
VA_COMPGEN(0x004c8ba9, 0x1c, STATIC_CTOR, castleGenerator1)
DATA(0x005a6150) const TAbsoluteGeneratorTraits castleGenerator2(CREATURE_ARCHER, CREATURE_MARKSMAN);
VA_COMPGEN(0x004c8be7, 0x1f, STATIC_CTOR, castleGenerator2)
DATA(0x005a6190) const TAbsoluteGeneratorTraits castleGenerator3(CREATURE_GRIFFIN, CREATURE_ROYAL_GRIFFIN);
VA_COMPGEN(0x004c8c06, 0x1f, STATIC_CTOR, castleGenerator3)
DATA(0x005a6160) const TAbsoluteGeneratorTraits castleGenerator4(CREATURE_SWORDSMAN, CREATURE_CRUSADER);
VA_COMPGEN(0x004c8c25, 0x1f, STATIC_CTOR, castleGenerator4)
DATA(0x005a63b0) const TAbsoluteGeneratorTraits castleGenerator5(CREATURE_MONK, CREATURE_ZEALOT);
VA_COMPGEN(0x004c8c44, 0x1f, STATIC_CTOR, castleGenerator5)
DATA(0x005a6460) const TAbsoluteGeneratorTraits castleGenerator6(CREATURE_CAVALIER, CREATURE_CHAMPION);
VA_COMPGEN(0x004c8c63, 0x1f, STATIC_CTOR, castleGenerator6)
DATA(0x005a64e0) const TAbsoluteGeneratorTraits castleGenerator7(CREATURE_ANGEL, CREATURE_ARCHANGEL);
VA_COMPGEN(0x004c8c82, 0x1f, STATIC_CTOR, castleGenerator7)

DATA(0x005449d0) const TTown::TGeneratorTraits* const apCastleGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &castleGenerator1, &castleGenerator2, &castleGenerator3, &castleGenerator4, &castleGenerator5, &castleGenerator6, &castleGenerator7
};

DATA(0x005a6820) TTown::TBuildingTraits aRampartBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingMageGuild4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingSpecial1),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingDwelling5), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004c8ca6, 0x36b, STATIC_CTOR, aRampartBuildingTraits)

DATA(0x005a5218) const TAbsoluteGeneratorTraits rampartGenerator1(CREATURE_CENTAUR, CREATURE_CENTAUR_CAPTAIN);
VA_COMPGEN(0x004c9011, 0x1f, STATIC_CTOR, rampartGenerator1)
DATA(0x005a67c8) const TAbsoluteGeneratorTraits rampartGenerator2(CREATURE_DWARF, CREATURE_BATTLE_DWARF);
VA_COMPGEN(0x004c9030, 0x1f, STATIC_CTOR, rampartGenerator2)
DATA(0x005a65b8) const TAbsoluteGeneratorTraits rampartGenerator3(CREATURE_WOOD_ELF, CREATURE_GRAND_ELF);
VA_COMPGEN(0x004c904f, 0x1f, STATIC_CTOR, rampartGenerator3)
DATA(0x005a6a38) const TAbsoluteGeneratorTraits rampartGenerator4(CREATURE_PEGASUS, CREATURE_SILVER_PEGASUS);
VA_COMPGEN(0x004c906e, 0x1f, STATIC_CTOR, rampartGenerator4)
DATA(0x005a6a28) const TAbsoluteGeneratorTraits rampartGenerator5(CREATURE_DENDROID_GUARD, CREATURE_DENDROID_SOLDIER);
VA_COMPGEN(0x004c908d, 0x1f, STATIC_CTOR, rampartGenerator5)
DATA(0x005a64f0) const TAbsoluteGeneratorTraits rampartGenerator6(CREATURE_UNICORN, CREATURE_WAR_UNICORN);
VA_COMPGEN(0x004c90ac, 0x1f, STATIC_CTOR, rampartGenerator6)
DATA(0x005a61b0) const TAbsoluteGeneratorTraits rampartGenerator7(CREATURE_GREEN_DRAGON, CREATURE_GOLD_DRAGON);
VA_COMPGEN(0x004c90cb, 0x1f, STATIC_CTOR, rampartGenerator7)

DATA(0x005449ec) const TTown::TGeneratorTraits* const apRampartGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &rampartGenerator1, &rampartGenerator2, &rampartGenerator3, &rampartGenerator4, &rampartGenerator5, &rampartGenerator6, &rampartGenerator7
};

DATA(0x005a5850) TTown::TBuildingTraits aTowerBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingMageGuild4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild1),
    TTown::TBuildingTraits(eBuildingCastle), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004c90ef, 0x367, STATIC_CTOR, aTowerBuildingTraits)

DATA(0x005a6510) const TAbsoluteGeneratorTraits towerGenerator1(CREATURE_GREMLIN, CREATURE_MASTER_GREMLIN);
VA_COMPGEN(0x004c9456, 0x1f, STATIC_CTOR, towerGenerator1)
DATA(0x005a64c0) const TAbsoluteGeneratorTraits towerGenerator2(CREATURE_STONE_GARGOYLE, CREATURE_OBSIDIAN_GARGOYLE);
VA_COMPGEN(0x004c9475, 0x1f, STATIC_CTOR, towerGenerator2)
DATA(0x005a6a18) const TAbsoluteGeneratorTraits towerGenerator3(CREATURE_STONE_GOLEM, CREATURE_IRON_GOLEM);
VA_COMPGEN(0x004c9494, 0x1f, STATIC_CTOR, towerGenerator3)
DATA(0x005a67b8) const TAbsoluteGeneratorTraits towerGenerator4(CREATURE_MAGE, CREATURE_ARCH_MAGE);
VA_COMPGEN(0x004c94b3, 0x1f, STATIC_CTOR, towerGenerator4)
DATA(0x005a6530) const TAbsoluteGeneratorTraits towerGenerator5(CREATURE_GENIE, CREATURE_MASTER_GENIE);
VA_COMPGEN(0x004c94d2, 0x1f, STATIC_CTOR, towerGenerator5)
DATA(0x005a6800) const TAbsoluteGeneratorTraits towerGenerator6(CREATURE_NAGA_SENTINEL, CREATURE_NAGA_QUEEN);
VA_COMPGEN(0x004c94f1, 0x1f, STATIC_CTOR, towerGenerator6)
DATA(0x005a5628) const TAbsoluteGeneratorTraits towerGenerator7(CREATURE_GIANT, CREATURE_TITAN);
VA_COMPGEN(0x004c9510, 0x1f, STATIC_CTOR, towerGenerator7)

DATA(0x00544a08) const TTown::TGeneratorTraits* const apTowerGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &towerGenerator1, &towerGenerator2, &towerGenerator3, &towerGenerator4, &towerGenerator5, &towerGenerator6, &towerGenerator7
};

DATA(0x005a5238) TTown::TBuildingTraits aInfernoBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingMageGuild4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingFort),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingDwelling3), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004c9534, 0x364, STATIC_CTOR, aInfernoBuildingTraits)

DATA(0x005a63e0) const TAbsoluteGeneratorTraits infernoGenerator1(CREATURE_IMP, CREATURE_FAMILIAR);
VA_COMPGEN(0x004c9898, 0x1f, STATIC_CTOR, infernoGenerator1)
DATA(0x005a6180) const TAbsoluteGeneratorTraits infernoGenerator2(CREATURE_GOG, CREATURE_MAGOG);
VA_COMPGEN(0x004c98b7, 0x1f, STATIC_CTOR, infernoGenerator2)
DATA(0x005a5a90) const TAbsoluteGeneratorTraits infernoGenerator3(CREATURE_HELL_HOUND, CREATURE_CERBERUS);
VA_COMPGEN(0x004c98d6, 0x1f, STATIC_CTOR, infernoGenerator3)
DATA(0x005a5a50) const TAbsoluteGeneratorTraits infernoGenerator4(CREATURE_DEMON, CREATURE_HORNED_DEMON);
VA_COMPGEN(0x004c98f5, 0x1f, STATIC_CTOR, infernoGenerator4)
DATA(0x005a5a40) const TAbsoluteGeneratorTraits infernoGenerator5(CREATURE_PIT_FIEND, CREATURE_PIT_LORD);
VA_COMPGEN(0x004c9914, 0x1f, STATIC_CTOR, infernoGenerator5)
DATA(0x005a5840) const TAbsoluteGeneratorTraits infernoGenerator6(CREATURE_EFREETI, CREATURE_EFREET_SULTAN);
VA_COMPGEN(0x004c9933, 0x1f, STATIC_CTOR, infernoGenerator6)
DATA(0x005a5cc0) const TAbsoluteGeneratorTraits infernoGenerator7(CREATURE_DEVIL, CREATURE_ARCH_DEVIL);
VA_COMPGEN(0x004c9952, 0x1f, STATIC_CTOR, infernoGenerator7)

DATA(0x00544a24) const TTown::TGeneratorTraits* const apInfernoGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &infernoGenerator1, &infernoGenerator2, &infernoGenerator3, &infernoGenerator4, &infernoGenerator5, &infernoGenerator6, &infernoGenerator7
};

DATA(0x005a65c8) TTown::TBuildingTraits aNecropolisBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingMageGuild4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingMageGuild1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004c9976, 0x364, STATIC_CTOR, aNecropolisBuildingTraits)

DATA(0x005a6480) const TAbsoluteGeneratorTraits necropolisGenerator1(CREATURE_SKELETON, CREATURE_SKELETON_WARRIOR);
VA_COMPGEN(0x004c9cda, 0x1f, STATIC_CTOR, necropolisGenerator1)
DATA(0x005a64b0) const TAbsoluteGeneratorTraits necropolisGenerator2(CREATURE_WALKING_DEAD, CREATURE_ZOMBIE);
VA_COMPGEN(0x004c9cf9, 0x1f, STATIC_CTOR, necropolisGenerator2)
DATA(0x005a6500) const TAbsoluteGeneratorTraits necropolisGenerator3(CREATURE_WIGHT, CREATURE_WRAITH);
VA_COMPGEN(0x004c9d18, 0x1f, STATIC_CTOR, necropolisGenerator3)
DATA(0x005a67d8) const TAbsoluteGeneratorTraits necropolisGenerator4(CREATURE_VAMPIRE, CREATURE_VAMPIRE_LORD);
VA_COMPGEN(0x004c9d37, 0x1f, STATIC_CTOR, necropolisGenerator4)
DATA(0x005a6810) const TAbsoluteGeneratorTraits necropolisGenerator5(CREATURE_LICH, CREATURE_POWER_LICH);
VA_COMPGEN(0x004c9d56, 0x1f, STATIC_CTOR, necropolisGenerator5)
DATA(0x005a6120) const TAbsoluteGeneratorTraits necropolisGenerator6(CREATURE_BLACK_KNIGHT, CREATURE_DREAD_KNIGHT);
VA_COMPGEN(0x004c9d75, 0x1f, STATIC_CTOR, necropolisGenerator6)
DATA(0x005a6140) const TAbsoluteGeneratorTraits necropolisGenerator7(CREATURE_BONE_DRAGON, CREATURE_GHOST_DRAGON);
VA_COMPGEN(0x004c9d94, 0x1f, STATIC_CTOR, necropolisGenerator7)

DATA(0x00544a40) const TTown::TGeneratorTraits* const apNecropolisGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &necropolisGenerator1, &necropolisGenerator2, &necropolisGenerator3, &necropolisGenerator4, &necropolisGenerator5, &necropolisGenerator6, &necropolisGenerator7
};

DATA(0x005a61c0) TTown::TBuildingTraits aDungeonBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingMageGuild4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004c9db8, 0x363, STATIC_CTOR, aDungeonBuildingTraits)

DATA(0x005a5f10) const TAbsoluteGeneratorTraits dungeonGenerator1(CREATURE_TROGLODYTE, CREATURE_INFERNAL_TROGLODYTE);
VA_COMPGEN(0x004ca11b, 0x1f, STATIC_CTOR, dungeonGenerator1)
DATA(0x005a6110) const TAbsoluteGeneratorTraits dungeonGenerator2(CREATURE_HARPY, CREATURE_HARPY_HAG);
VA_COMPGEN(0x004ca13a, 0x1f, STATIC_CTOR, dungeonGenerator2)
DATA(0x005a5ef0) const TAbsoluteGeneratorTraits dungeonGenerator3(CREATURE_BEHOLDER, CREATURE_EVIL_EYE);
VA_COMPGEN(0x004ca159, 0x1f, STATIC_CTOR, dungeonGenerator3)
DATA(0x005a5cf0) const TAbsoluteGeneratorTraits dungeonGenerator4(CREATURE_MEDUSA, CREATURE_MEDUSA_QUEEN);
VA_COMPGEN(0x004ca178, 0x1f, STATIC_CTOR, dungeonGenerator4)
DATA(0x005a6430) const TAbsoluteGeneratorTraits dungeonGenerator5(CREATURE_MINOTAUR, CREATURE_MINOTAUR_KING);
VA_COMPGEN(0x004ca197, 0x1f, STATIC_CTOR, dungeonGenerator5)
DATA(0x005a63f0) const TAbsoluteGeneratorTraits dungeonGenerator6(CREATURE_MANTICORE, CREATURE_SCORPICORE);
VA_COMPGEN(0x004ca1b6, 0x1f, STATIC_CTOR, dungeonGenerator6)
DATA(0x005a63d0) const TAbsoluteGeneratorTraits dungeonGenerator7(CREATURE_RED_DRAGON, CREATURE_BLACK_DRAGON);
VA_COMPGEN(0x004ca1d5, 0x1f, STATIC_CTOR, dungeonGenerator7)

DATA(0x00544a5c) const TTown::TGeneratorTraits* const apDungeonGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &dungeonGenerator1, &dungeonGenerator2, &dungeonGenerator3, &dungeonGenerator4, &dungeonGenerator5, &dungeonGenerator6, &dungeonGenerator7
};

DATA(0x005a5ad0) TTown::TBuildingTraits aStrongholdBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingBlacksmith), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004ca1f9, 0x35f, STATIC_CTOR, aStrongholdBuildingTraits)

DATA(0x005a5cd0) const TAbsoluteGeneratorTraits strongholdGenerator1(CREATURE_GOBLIN, CREATURE_HOBGOBLIN);
VA_COMPGEN(0x004ca558, 0x1f, STATIC_CTOR, strongholdGenerator1)
DATA(0x005a5ab0) const TAbsoluteGeneratorTraits strongholdGenerator2(CREATURE_WOLF_RIDER, CREATURE_WOLF_RAIDER);
VA_COMPGEN(0x004ca577, 0x1f, STATIC_CTOR, strongholdGenerator2)
DATA(0x005a64d0) const TAbsoluteGeneratorTraits strongholdGenerator3(CREATURE_ORC, CREATURE_ORC_CHIEFTAIN);
VA_COMPGEN(0x004ca596, 0x1f, STATIC_CTOR, strongholdGenerator3)
DATA(0x005a6520) const TAbsoluteGeneratorTraits strongholdGenerator4(CREATURE_OGRE, CREATURE_OGRE_MAGE);
VA_COMPGEN(0x004ca5b5, 0x1f, STATIC_CTOR, strongholdGenerator4)
DATA(0x005a64a0) const TAbsoluteGeneratorTraits strongholdGenerator5(CREATURE_ROC, CREATURE_THUNDERBIRD);
VA_COMPGEN(0x004ca5d4, 0x1f, STATIC_CTOR, strongholdGenerator5)
DATA(0x005a6490) const TAbsoluteGeneratorTraits strongholdGenerator6(CREATURE_CYCLOPS, CREATURE_CYCLOPS_KING);
VA_COMPGEN(0x004ca5f3, 0x1f, STATIC_CTOR, strongholdGenerator6)
DATA(0x005a6420) const TAbsoluteGeneratorTraits strongholdGenerator7(CREATURE_BEHEMOTH, CREATURE_ANCIENT_BEHEMOTH);
VA_COMPGEN(0x004ca612, 0x1f, STATIC_CTOR, strongholdGenerator7)

DATA(0x00544a78) const TTown::TGeneratorTraits* const apStrongholdGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &strongholdGenerator1, &strongholdGenerator2, &strongholdGenerator3, &strongholdGenerator4, &strongholdGenerator5, &strongholdGenerator6, &strongholdGenerator7
};

DATA(0x005a5f20) TTown::TBuildingTraits aFortressBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingFort),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004ca636, 0x35c, STATIC_CTOR, aFortressBuildingTraits)

DATA(0x005a6170) const TAbsoluteGeneratorTraits fortressGenerator1(CREATURE_GNOLL, CREATURE_GNOLL_MARAUDER);
VA_COMPGEN(0x004ca992, 0x1f, STATIC_CTOR, fortressGenerator1)
DATA(0x005a6450) const TAbsoluteGeneratorTraits fortressGenerator2(CREATURE_LIZARDMAN, CREATURE_LIZARD_WARRIOR);
VA_COMPGEN(0x004ca9b1, 0x1f, STATIC_CTOR, fortressGenerator2)
DATA(0x005a6470) const TAbsoluteGeneratorTraits fortressGenerator3(CREATURE_SERPENT_FLY, CREATURE_DRAGON_FLY);
VA_COMPGEN(0x004ca9d0, 0x1f, STATIC_CTOR, fortressGenerator3)
DATA(0x005a6400) const TAbsoluteGeneratorTraits fortressGenerator4(CREATURE_BASILISK, CREATURE_GREATER_BASILISK);
VA_COMPGEN(0x004ca9ef, 0x1f, STATIC_CTOR, fortressGenerator4)
DATA(0x005a5f00) const TAbsoluteGeneratorTraits fortressGenerator5(CREATURE_GORGON, CREATURE_MIGHTY_GORGON);
VA_COMPGEN(0x004caa0e, 0x1f, STATIC_CTOR, fortressGenerator5)
DATA(0x005a6130) const TAbsoluteGeneratorTraits fortressGenerator6(CREATURE_WYVERN, CREATURE_WYVERN_MONARCH);
VA_COMPGEN(0x004caa2d, 0x1f, STATIC_CTOR, fortressGenerator6)
DATA(0x005a5ac0) const TAbsoluteGeneratorTraits fortressGenerator7(CREATURE_HYDRA, CREATURE_CHAOS_HYDRA);
VA_COMPGEN(0x004caa4c, 0x1f, STATIC_CTOR, fortressGenerator7)

DATA(0x00544a94) const TTown::TGeneratorTraits* const apFortressGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &fortressGenerator1, &fortressGenerator2, &fortressGenerator3, &fortressGenerator4, &fortressGenerator5, &fortressGenerator6, &fortressGenerator7
};

DATA(0x005a5d00) TTown::TBuildingTraits aConfluxBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingMageGuild4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004caa70, 0x364, STATIC_CTOR, aConfluxBuildingTraits)

DATA(0x005a6410) const TAbsoluteGeneratorTraits confluxGenerator1(CREATURE_PIXIE, CREATURE_SPRITE);
VA_COMPGEN(0x004cadd4, 0x1f, STATIC_CTOR, confluxGenerator1)
DATA(0x005a6440) const TAbsoluteGeneratorTraits confluxGenerator2(CREATURE_WATER_ELEMENTAL, CREATURE_ICE_ELEMENTAL);
VA_COMPGEN(0x004cadf3, 0x1f, STATIC_CTOR, confluxGenerator2)
DATA(0x005a63c0) const TAbsoluteGeneratorTraits confluxGenerator3(CREATURE_EARTH_ELEMENTAL, CREATURE_MAGMA_ELEMENTAL);
VA_COMPGEN(0x004cae12, 0x1f, STATIC_CTOR, confluxGenerator3)
DATA(0x005a61a0) const TAbsoluteGeneratorTraits confluxGenerator4(CREATURE_AIR_ELEMENTAL, CREATURE_STORM_ELEMENTAL);
VA_COMPGEN(0x004cae31, 0x1f, STATIC_CTOR, confluxGenerator4)
DATA(0x005a5a80) const TAbsoluteGeneratorTraits confluxGenerator5(CREATURE_FIRE_ELEMENTAL, CREATURE_ENERGY_ELEMENTAL);
VA_COMPGEN(0x004cae50, 0x1f, STATIC_CTOR, confluxGenerator5)
DATA(0x005a5aa0) const TAbsoluteGeneratorTraits confluxGenerator6(CREATURE_PSYCHIC_ELEMENTAL, CREATURE_MAGIC_ELEMENTAL);
VA_COMPGEN(0x004cae6f, 0x1f, STATIC_CTOR, confluxGenerator6)
DATA(0x005a5a60) const TAbsoluteGeneratorTraits confluxGenerator7(CREATURE_FIREBIRD, CREATURE_PHOENIX);
VA_COMPGEN(0x004cae8e, 0x1f, STATIC_CTOR, confluxGenerator7)

DATA(0x00544ab0) const TTown::TGeneratorTraits* const apConfluxGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &confluxGenerator1, &confluxGenerator2, &confluxGenerator3, &confluxGenerator4, &confluxGenerator5, &confluxGenerator6, &confluxGenerator7
};

DATA(0x005a5638) TTown::TBuildingTraits aRandomBuildingTraits[kNumBuildings] = {
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingTownHall),
    TTown::TBuildingTraits(eBuildingCityHall), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingFort), TTown::TBuildingTraits(eBuildingCitadel),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingMarketplace),
    TTown::TBuildingTraits(eBuildingMarketplace), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingMageGuild1), TTown::TBuildingTraits(eBuildingMageGuild2),
    TTown::TBuildingTraits(eBuildingMageGuild3), TTown::TBuildingTraits(eBuildingMageGuild4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling1),
    TTown::TBuildingTraits(eBuildingDwelling1), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling2), TTown::TBuildingTraits(eBuildingDwelling2),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling3),
    TTown::TBuildingTraits(eBuildingDwelling3), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling4), TTown::TBuildingTraits(eBuildingDwelling4),
    TTown::TBuildingTraits(eBuildingNone), TTown::TBuildingTraits(eBuildingDwelling5),
    TTown::TBuildingTraits(eBuildingDwelling5), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling6), TTown::TBuildingTraits(eBuildingNone),
    TTown::TBuildingTraits(eBuildingDwelling7)
};
VA_COMPGEN(0x004caeb2, 0x366, STATIC_CTOR, aRandomBuildingTraits)

DATA(0x005a5620) const TIndeterminateGeneratorTraits randomGenerator1(TTown::TGeneratorType(0));
VA_COMPGEN(0x004cb218, 0x12, STATIC_CTOR, randomGenerator1)
DATA(0x005a5428) const TIndeterminateGeneratorTraits randomGenerator2(TTown::TGeneratorType(1));
VA_COMPGEN(0x004cb240, 0x15, STATIC_CTOR, randomGenerator2)
DATA(0x005a5228) const TIndeterminateGeneratorTraits randomGenerator3(TTown::TGeneratorType(2));
VA_COMPGEN(0x004cb255, 0x15, STATIC_CTOR, randomGenerator3)
DATA(0x005a5230) const TIndeterminateGeneratorTraits randomGenerator4(TTown::TGeneratorType(3));
VA_COMPGEN(0x004cb26a, 0x15, STATIC_CTOR, randomGenerator4)
DATA(0x005a5ce8) const TIndeterminateGeneratorTraits randomGenerator5(TTown::TGeneratorType(4));
VA_COMPGEN(0x004cb27f, 0x15, STATIC_CTOR, randomGenerator5)
DATA(0x005a5ce0) const TIndeterminateGeneratorTraits randomGenerator6(TTown::TGeneratorType(5));
VA_COMPGEN(0x004cb294, 0x15, STATIC_CTOR, randomGenerator6)
DATA(0x005a6a10) const TIndeterminateGeneratorTraits randomGenerator7(TTown::TGeneratorType(6));
VA_COMPGEN(0x004cb2a9, 0x15, STATIC_CTOR, randomGenerator7)

DATA(0x00544acc) const TTown::TGeneratorTraits* const apRandomGeneratorTraits[TTown::s_kNumGeneratorTypes] = {
    &randomGenerator1, &randomGenerator2, &randomGenerator3, &randomGenerator4, &randomGenerator5, &randomGenerator6, &randomGenerator7
};
DATA(0x005a6540) TTown::TTypeTraits aTownTypeTraitsImp[kNumTownTypes + 1] = {
    TTown::TTypeTraits(aCastleBuildingTraits, apCastleGeneratorTraits),
    TTown::TTypeTraits(aRampartBuildingTraits, apRampartGeneratorTraits),
    TTown::TTypeTraits(aTowerBuildingTraits, apTowerGeneratorTraits),
    TTown::TTypeTraits(aInfernoBuildingTraits, apInfernoGeneratorTraits),
    TTown::TTypeTraits(aNecropolisBuildingTraits, apNecropolisGeneratorTraits),
    TTown::TTypeTraits(aDungeonBuildingTraits, apDungeonGeneratorTraits),
    TTown::TTypeTraits(aStrongholdBuildingTraits, apStrongholdGeneratorTraits),
    TTown::TTypeTraits(aFortressBuildingTraits, apFortressGeneratorTraits),
    TTown::TTypeTraits(aConfluxBuildingTraits, apConfluxGeneratorTraits),
    TTown::TTypeTraits(aRandomBuildingTraits, apRandomGeneratorTraits)
};
VA_COMPGEN(0x004cb2c3, 0x106, STATIC_CTOR, aTownTypeTraitsImp)
}

DATA(0x00592e18) const TTown::TTypeTraits* akTownTypeTraits = aTownTypeTraitsImp;

VA(0x004cb3c9, 0x1e)
bool TTown::TTypeTraits::hasMageGuildLevel(unsigned int level) const
{
    DATA(0x00544ae8) static const TBuilding akGuildLevelBuilding[] = {
        eBuildingMageGuild1, eBuildingMageGuild2, eBuildingMageGuild3, eBuildingMageGuild4, eBuildingMageGuild5
    };
    return !m_akBuildingTraits[akGuildLevelBuilding[level]].isDisallowed();
}

VA(0x004cb3e7, 0x481)
void InitializeTownTypeTraitsTable()
{
    DATA_COMPGEN_GUARD(0x005a6a50, townTypeTraitsGuard, pTypeNames)
    {
        VA_COMPGEN(0x004cb892, 0x16, STATIC_DTOR, pTypeNames)
        DATA(0x005a51f0) static TAutoArrayPtr<char> pTypeNames(0);
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("towntype.txt"));
        if (!pTextResource.get())
            throw TRuntimeError();
        int size = 0;
        unsigned int i;
        for (i = 0; i < kNumTownTypes; i++)
            size += strlen(pTextResource->GetText(i)) + 1;
        pTypeNames = TAutoArrayPtr<char>(new char[size]);
        if (!pTypeNames.get())
            throw TAllocationFailure();
        char* p = pTypeNames.get();
        for (i = 0; i < kNumTownTypes; i++) {
            const char* text = pTextResource->GetText(i);
            int length = strlen(text) + 1;
            memcpy(p, text, length);
            aTownTypeTraitsImp[i].m_pName = p;
            p += length;
        }
    }
    VA_COMPGEN(0x004cb87c, 0x16, STATIC_DTOR, pRandomName)
    DATA(0x005a6a48) static TAutoArrayPtr<char> pRandomName(new char[strlen(kRandomTownStr) + 1]);
    if (!pRandomName.get())
        throw TAllocationFailure();
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
    VA_COMPGEN(0x004cb868, 0x14, STATIC_DTOR, apTextTable)
    DATA(0x005a5200) static TAutoArrayPtr<char> apTextTable[kNumTextTables];
    std::vector<char*> aapBldgTextEntry[kNumTextTables];
    for (unsigned int table = 0; table < kNumTextTables; table++) {
        TResourcePtr<TSpreadsheetResource> pSpreadsheet(ResourceManager::GetSpreadsheet(akTextTableName[table]));
        if (!pSpreadsheet.get())
            throw TRuntimeError();
        unsigned int numRows = pSpreadsheet->GetNumberOfRows();
        int size = 0;
        unsigned int row;
        for (row = 0; row < numRows; row++)
            if (pSpreadsheet->GetNumberOfColumns(row) >= 2)
                size += strlen(pSpreadsheet->GetRow(row)[0]) + strlen(pSpreadsheet->GetRow(row)[1]) + 2;
        apTextTable[table] = TAutoArrayPtr<char>(new char[size]);
        if (!apTextTable[table].get())
            throw TAllocationFailure();
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
    DATA(0x00544b00) static const TBuildingTextRef akBuildingTextRef[kNumTownTypes + 1][kNumBuildings] = {
        {   // Castle
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 19 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 10 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 }, { eTextTableNone, 0 },
            { eTextTableSpecial, 3 }, { eTextTableSpecial, 9 }, { eTextTableSpecial, 0 },
            { eTextTableSpecial, 5 }, { eTextTableSpecial, 4 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 0 }, { eTextTableDwelling, 7 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 1 }, { eTextTableDwelling, 8 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 2 }, { eTextTableDwelling, 9 }, { eTextTableSpecial, 1 },
            { eTextTableDwelling, 3 }, { eTextTableDwelling, 10 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 4 }, { eTextTableDwelling, 11 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 5 }, { eTextTableDwelling, 12 }, { eTextTableDwelling, 6 },
            { eTextTableDwelling, 13 }
        },
        {   // Rampart
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 20 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 21 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 },
            { eTextTableNeutral, 4 }, { eTextTableNone, 0 }, { eTextTableSpecial, 20 },
            { eTextTableSpecial, 11 }, { eTextTableSpecial, 15 }, { eTextTableSpecial, 16 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 14 }, { eTextTableDwelling, 21 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 15 }, { eTextTableDwelling, 22 },
            { eTextTableSpecial, 12 }, { eTextTableDwelling, 16 }, { eTextTableDwelling, 23 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 17 }, { eTextTableDwelling, 24 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 18 }, { eTextTableDwelling, 25 },
            { eTextTableSpecial, 18 }, { eTextTableDwelling, 19 }, { eTextTableDwelling, 26 },
            { eTextTableDwelling, 20 }, { eTextTableDwelling, 27 }
        },
        {   // Tower
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 21 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 32 }, { eTextTableSpecial, 22 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 },
            { eTextTableNeutral, 4 }, { eTextTableNone, 0 }, { eTextTableSpecial, 31 },
            { eTextTableSpecial, 27 }, { eTextTableSpecial, 28 }, { eTextTableSpecial, 26 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 28 }, { eTextTableDwelling, 35 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 29 }, { eTextTableDwelling, 36 },
            { eTextTableSpecial, 23 }, { eTextTableDwelling, 30 }, { eTextTableDwelling, 37 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 31 }, { eTextTableDwelling, 38 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 32 }, { eTextTableDwelling, 39 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 33 }, { eTextTableDwelling, 40 },
            { eTextTableDwelling, 34 }, { eTextTableDwelling, 41 }
        },
        {   // Inferno
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 22 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 43 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 },
            { eTextTableNeutral, 4 }, { eTextTableNone, 0 }, { eTextTableSpecial, 42 },
            { eTextTableSpecial, 37 }, { eTextTableSpecial, 38 }, { eTextTableSpecial, 39 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 42 }, { eTextTableDwelling, 49 },
            { eTextTableSpecial, 34 }, { eTextTableDwelling, 43 }, { eTextTableDwelling, 50 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 44 }, { eTextTableDwelling, 51 },
            { eTextTableSpecial, 40 }, { eTextTableDwelling, 45 }, { eTextTableDwelling, 52 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 46 }, { eTextTableDwelling, 53 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 47 }, { eTextTableDwelling, 54 },
            { eTextTableDwelling, 48 }, { eTextTableDwelling, 55 }
        },
        {   // Necropolis
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 23 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 54 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 },
            { eTextTableNeutral, 4 }, { eTextTableSpecial, 47 }, { eTextTableSpecial, 53 },
            { eTextTableSpecial, 44 }, { eTextTableSpecial, 48 }, { eTextTableSpecial, 49 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 56 }, { eTextTableDwelling, 63 },
            { eTextTableSpecial, 45 }, { eTextTableDwelling, 57 }, { eTextTableDwelling, 64 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 58 }, { eTextTableDwelling, 65 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 59 }, { eTextTableDwelling, 66 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 60 }, { eTextTableDwelling, 67 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 61 }, { eTextTableDwelling, 68 },
            { eTextTableDwelling, 62 }, { eTextTableDwelling, 69 }
        },
        {   // Dungeon
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 24 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 65 }, { eTextTableSpecial, 55 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 },
            { eTextTableNeutral, 4 }, { eTextTableNone, 0 }, { eTextTableSpecial, 64 },
            { eTextTableSpecial, 59 }, { eTextTableSpecial, 60 }, { eTextTableSpecial, 61 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 70 }, { eTextTableDwelling, 77 },
            { eTextTableSpecial, 56 }, { eTextTableDwelling, 71 }, { eTextTableDwelling, 78 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 72 }, { eTextTableDwelling, 79 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 73 }, { eTextTableDwelling, 80 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 74 }, { eTextTableDwelling, 81 },
            { eTextTableNone, 0 }, { eTextTableDwelling, 75 }, { eTextTableDwelling, 82 },
            { eTextTableDwelling, 76 }, { eTextTableDwelling, 83 }
        },
        {   // Stronghold
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 25 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 76 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNone, 0 }, { eTextTableNone, 0 },
            { eTextTableNone, 0 }, { eTextTableSpecial, 75 }, { eTextTableSpecial, 66 },
            { eTextTableSpecial, 70 }, { eTextTableSpecial, 71 }, { eTextTableSpecial, 72 },
            { eTextTableDwelling, 84 }, { eTextTableDwelling, 91 }, { eTextTableSpecial, 67 },
            { eTextTableDwelling, 85 }, { eTextTableDwelling, 92 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 86 }, { eTextTableDwelling, 93 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 87 }, { eTextTableDwelling, 94 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 88 }, { eTextTableDwelling, 95 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 89 }, { eTextTableDwelling, 96 }, { eTextTableDwelling, 90 },
            { eTextTableDwelling, 97 }
        },
        {   // Fortress
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 26 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 87 }, { eTextTableNone, 0 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNone, 0 }, { eTextTableNone, 0 },
            { eTextTableSpecial, 80 }, { eTextTableSpecial, 86 }, { eTextTableSpecial, 82 },
            { eTextTableSpecial, 81 }, { eTextTableSpecial, 77 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 98 }, { eTextTableDwelling, 105 }, { eTextTableSpecial, 78 },
            { eTextTableDwelling, 99 }, { eTextTableDwelling, 106 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 100 }, { eTextTableDwelling, 107 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 101 }, { eTextTableDwelling, 108 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 102 }, { eTextTableDwelling, 109 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 103 }, { eTextTableDwelling, 110 }, { eTextTableDwelling, 104 },
            { eTextTableDwelling, 111 }
        },
        {   // Conflux
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 27 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 98 }, { eTextTableSpecial, 88 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 },
            { eTextTableNeutral, 4 }, { eTextTableSpecial, 91 }, { eTextTableSpecial, 97 },
            { eTextTableSpecial, 92 }, { eTextTableNone, 0 }, { eTextTableNone, 0 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 112 }, { eTextTableDwelling, 119 }, { eTextTableSpecial, 89 },
            { eTextTableDwelling, 113 }, { eTextTableDwelling, 120 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 114 }, { eTextTableDwelling, 121 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 115 }, { eTextTableDwelling, 122 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 116 }, { eTextTableDwelling, 123 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 117 }, { eTextTableDwelling, 124 }, { eTextTableDwelling, 118 },
            { eTextTableDwelling, 125 }
        },
        {   // Random
            { eTextTableNeutral, 11 }, { eTextTableNeutral, 12 }, { eTextTableNeutral, 13 },
            { eTextTableNeutral, 7 }, { eTextTableNeutral, 8 }, { eTextTableNeutral, 9 },
            { eTextTableNeutral, 5 }, { eTextTableNeutral, 28 }, { eTextTableNeutral, 14 },
            { eTextTableSpecial, 109 }, { eTextTableSpecial, 99 }, { eTextTableNeutral, 0 },
            { eTextTableNeutral, 1 }, { eTextTableNeutral, 2 }, { eTextTableNeutral, 3 },
            { eTextTableNeutral, 4 }, { eTextTableSpecial, 102 }, { eTextTableSpecial, 108 },
            { eTextTableNone, 0 }, { eTextTableNone, 0 }, { eTextTableNone, 0 }, { eTextTableNone, 0 },
            { eTextTableDwelling, 126 }, { eTextTableDwelling, 133 }, { eTextTableSpecial, 100 },
            { eTextTableDwelling, 127 }, { eTextTableDwelling, 134 }, { eTextTableSpecial, 101 },
            { eTextTableDwelling, 128 }, { eTextTableDwelling, 135 }, { eTextTableSpecial, 105 },
            { eTextTableDwelling, 129 }, { eTextTableDwelling, 136 }, { eTextTableSpecial, 106 },
            { eTextTableDwelling, 130 }, { eTextTableDwelling, 137 }, { eTextTableSpecial, 107 },
            { eTextTableDwelling, 131 }, { eTextTableDwelling, 138 }, { eTextTableDwelling, 132 },
            { eTextTableDwelling, 139 }
        }    };
    DATA(0x005457d0) static TTown::TBuildingTraits (* const apBuildingTraits[kNumTownTypes + 1])[kNumBuildings] = {
        &aCastleBuildingTraits, &aRampartBuildingTraits, &aTowerBuildingTraits, &aInfernoBuildingTraits,
        &aNecropolisBuildingTraits, &aDungeonBuildingTraits, &aStrongholdBuildingTraits, &aFortressBuildingTraits,
        &aConfluxBuildingTraits, &aRandomBuildingTraits
    };
    for (unsigned int type = 0; type < kNumTownTypes + 1; type++) {
        TTown::TBuildingTraits* const aBuildingTraits = *apBuildingTraits[type];
        const TBuildingTextRef* const akTypeTextRef = akBuildingTextRef[type];
        for (unsigned int building = 0; building < kNumBuildings; building++) {
            const TBuildingTextRef& kBuildingTextRef = akTypeTextRef[building];
            if (kBuildingTextRef.m_tableID != eTextTableNone) {
                const char* pName = aapBldgTextEntry[kBuildingTextRef.m_tableID][kBuildingTextRef.m_index];
                aBuildingTraits[building].m_pName = pName;
                aBuildingTraits[building].m_pDescription = pName + strlen(pName) + 1;
            }
        }
    }
    TIndeterminateGeneratorTraits::initialize();
}
