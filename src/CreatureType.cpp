// CreatureType.cpp of the Loki port (Loki object 9): the RoE creature
// traits table, its static columns and the crtraits.txt loader.
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "creaturetype.h"

#include "resourcemanager.h"
#include "textresource.h"

// Each town's seven unupgraded creatures, dwelling by dwelling.
static const TCreatureType akBaseCreatures[kNumTownTypes][kNumCreatureTypesPerTown] = {
    { eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, eCreatureSwordsman, eCreatureMonk, eCreatureCavalier, eCreatureAngel },
    { eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, eCreaturePegasus, eCreatureTreefolk, eCreatureUnicorn, eCreatureGreenDragon },
    { eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, eCreatureMage, eCreatureGenie, eCreatureNagaSentinel, eCreatureLesserTitan },
    { eCreatureImp, eCreatureGog, eCreatureHellHound, eCreatureSingleHornedDemon, eCreaturePitFiend, eCreatureEfreet, eCreatureDevil },
    { eCreatureSkeleton, eCreatureZombie, eCreatureWight, eCreatureVampire, eCreatureLich, eCreatureBlackKnight, eCreatureBoneDragon },
    { eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, eCreatureMedusa, eCreatureMinotaur, eCreatureManticore, eCreatureRedDragon },
    { eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, eCreatureOgre, eCreatureRoc, eCreatureCyclops, eCreatureYoungBehemoth },
    { eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureCopperGorgon, eCreatureSerpentFly, eCreatureBasilisk, eCreatureWyvern, eCreatureHydra },
};

// The static columns are compiled in; crtraits.txt fills the names, costs and
// combat values (InitializeCreatureTypeTraits).
static TCreatureTypeTraits aCreatureTypeTraitsImp[kNumCreatureAndSiegeWeaponTypes] = {
    { eTownCastle, 0, "pike", "cpkman.def", creatureAlive },
    { eTownCastle, 0, "halb", "chalbd.def", creatureAlive },
    { eTownCastle, 1, "lcrs", "clcbow.def", creatureShootingArmy | creatureAlive },
    { eTownCastle, 1, "hcrs", "chcbow.def", creatureShootingArmy | creatureAlive | creatureTwoAttacks },
    { eTownCastle, 2, "grif", "cgriff.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownCastle, 2, "rgrf", "crgrif.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownCastle, 3, "swrd", "csword.def", creatureAlive },
    { eTownCastle, 3, "crus", "ccrusd.def", creatureAlive | creatureTwoAttacks },
    { eTownCastle, 4, "monk", "cmonkk.def", creatureShootingArmy | creatureAlive },
    { eTownCastle, 4, "zelt", "czealt.def", creatureShootingArmy | creatureAlive | creatureNoMeleePenalty },
    { eTownCastle, 5, "cava", "ccavlr.def", creatureDoubleWide | creatureAlive },
    { eTownCastle, 5, "chmp", "cchamp.def", creatureDoubleWide | creatureAlive },
    { eTownCastle, 6, "angl", "cangel.def", creatureFlyingArmy | creatureAlive | creatureKing2 },
    { eTownCastle, 6, "aagl", "crangl.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive | creatureKing2 },
    { eTownRampart, 0, "cntr", "ccentr.def", creatureDoubleWide | creatureAlive },
    { eTownRampart, 0, "ecnt", "cecent.def", creatureDoubleWide | creatureAlive },
    { eTownRampart, 1, "dwrf", "cdwarf.def", creatureAlive },
    { eTownRampart, 1, "bdrf", "cbdwar.def", creatureAlive },
    { eTownRampart, 2, "welf", "celf.def", creatureShootingArmy | creatureAlive },
    { eTownRampart, 2, "gelf", "cgrelf.def", creatureShootingArmy | creatureAlive | creatureTwoAttacks },
    { eTownRampart, 3, "pega", "cpegas.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownRampart, 3, "apeg", "capegs.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownRampart, 4, "tree", "ctree.def", creatureAlive },
    { eTownRampart, 4, "btre", "cbtree.def", creatureAlive },
    { eTownRampart, 5, "unic", "cunico.def", creatureDoubleWide | creatureAlive },
    { eTownRampart, 5, "wunc", "cwunic.def", creatureDoubleWide | creatureAlive },
    { eTownRampart, 6, "grdr", "cgdrag.def", creatureDoubleWide | creatureFlyingArmy | creatureHasExtendedAttack | creatureAlive | creatureKing1 },
    { eTownRampart, 6, "godr", "cddrag.def", creatureDoubleWide | creatureFlyingArmy | creatureHasExtendedAttack | creatureAlive | creatureKing1 },
    { eTownTower, 0, "agrm", "cgrema.def", creatureAlive },
    { eTownTower, 0, "mgrm", "cgremm.def", creatureShootingArmy | creatureAlive },
    { eTownTower, 1, "sgrg", "cgargo.def", creatureFlyingArmy },
    { eTownTower, 1, "ogrg", "cogarg.def", creatureFlyingArmy },
    { eTownTower, 2, "sglm", "csgole.def", creatureImmuneToMindSpells | creatureNoMorale },
    { eTownTower, 2, "iglm", "cigole.def", creatureImmuneToMindSpells | creatureNoMorale },
    { eTownTower, 3, "mage", "cmage.def", creatureShootingArmy | creatureAlive | creatureNoMeleePenalty },
    { eTownTower, 3, "amag", "camage.def", creatureShootingArmy | creatureAlive | creatureShootsRay | creatureNoMeleePenalty },
    { eTownTower, 4, "geni", "cgenie.def", creatureFlyingArmy | creatureAlive },
    { eTownTower, 4, "calf", "csulta.def", creatureFlyingArmy | creatureAlive },
    { eTownTower, 5, "nsen", "cnaga.def", creatureDoubleWide | creatureAlive | creatureFreeAttack },
    { eTownTower, 5, "ngrd", "cnagag.def", creatureDoubleWide | creatureAlive | creatureFreeAttack },
    { eTownTower, 6, "ltit", "cltita.def", creatureAlive | creatureKing3 | creatureImmuneToMindSpells },
    { eTownTower, 6, "gtit", "cgtita.def", creatureShootingArmy | creatureAlive | creatureKing3 | creatureImmuneToMindSpells | creatureNoMeleePenalty },
    { eTownInferno, 0, "impp", "cimp.def", creatureAlive },
    { eTownInferno, 0, "fmlr", "cfamil.def", creatureAlive },
    { eTownInferno, 1, "gogg", "cgog.def", creatureShootingArmy | creatureAlive },
    { eTownInferno, 1, "mgog", "cmagog.def", creatureShootingArmy | creatureAlive | creatureFireballAttack },
    { eTownInferno, 2, "hhnd", "chhoun.def", creatureDoubleWide | creatureAlive },
    { eTownInferno, 2, "cerb", "ccerbu.def", creatureDoubleWide | creatureAlive | creatureFreeAttack | creatureMultiHeaded },
    { eTownInferno, 3, "shdm", "cohdem.def", creatureAlive },
    { eTownInferno, 3, "dhdm", "cthdem.def", creatureAlive },
    { eTownInferno, 4, "pfnd", "cpfien.def", creatureAlive },
    { eTownInferno, 4, "pfoe", "cpfoe.def", creatureAlive },
    { eTownInferno, 5, "efrt", "cefree.def", creatureFlyingArmy | creatureAlive | creatureImmuneToFireSpells },
    { eTownInferno, 5, "esul", "cefres.def", creatureFlyingArmy | creatureAlive | creatureImmuneToFireSpells },
    { eTownInferno, 6, "devl", "cdevil.def", creatureAlive | creatureKing2 | creatureFreeAttack },
    { eTownInferno, 6, "advl", "cadevl.def", creatureAlive | creatureKing2 | creatureFreeAttack },
    { eTownNecropolis, 0, "skel", "cskele.def", creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 0, "sklw", "cwskel.def", creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 1, "zomb", "czombi.def", creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 1, "zmbl", "czomlo.def", creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 2, "wght", "cwight.def", creatureFlyingArmy | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 2, "wrth", "cwrait.def", creatureFlyingArmy | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 3, "vamp", "cvamp.def", creatureFlyingArmy | creatureImmuneToMindSpells | creatureFreeAttack | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 3, "nosf", "cnosfe.def", creatureFlyingArmy | creatureImmuneToMindSpells | creatureFreeAttack | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 4, "lich", "clich.def", creatureShootingArmy | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead | creatureFireballAttack },
    { eTownNecropolis, 4, "plch", "cplich.def", creatureShootingArmy | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead | creatureFireballAttack },
    { eTownNecropolis, 5, "bknt", "cbknig.def", creatureDoubleWide | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 5, "blrd", "cblord.def", creatureDoubleWide | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 6, "bodr", "cndrgn.def", creatureDoubleWide | creatureFlyingArmy | creatureKing1 | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownNecropolis, 6, "ghdr", "chdrgn.def", creatureDoubleWide | creatureFlyingArmy | creatureKing1 | creatureImmuneToMindSpells | creatureNoMorale | creatureUndead },
    { eTownDungeon, 0, "trog", "ctrogl.def", creatureAlive },
    { eTownDungeon, 0, "itrg", "citrog.def", creatureAlive },
    { eTownDungeon, 1, "harp", "charpy.def", creatureFlyingArmy | creatureAlive },
    { eTownDungeon, 1, "hhag", "charph.def", creatureFlyingArmy | creatureAlive | creatureFreeAttack },
    { eTownDungeon, 2, "bhdr", "cbehol.def", creatureShootingArmy | creatureAlive | creatureShootsRay | creatureNoMeleePenalty },
    { eTownDungeon, 2, "evli", "ceveye.def", creatureShootingArmy | creatureAlive | creatureShootsRay | creatureNoMeleePenalty },
    { eTownDungeon, 3, "medu", "cmedus.def", creatureDoubleWide | creatureShootingArmy | creatureAlive | creatureNoMeleePenalty },
    { eTownDungeon, 3, "medq", "cmeduq.def", creatureDoubleWide | creatureShootingArmy | creatureAlive | creatureNoMeleePenalty },
    { eTownDungeon, 4, "mino", "cminot.def", creatureAlive },
    { eTownDungeon, 4, "mink", "cminok.def", creatureAlive },
    { eTownDungeon, 5, "mant", "cmcore.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownDungeon, 5, "scrp", "ccmcor.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownDungeon, 6, "rddr", "crdrgn.def", creatureDoubleWide | creatureFlyingArmy | creatureHasExtendedAttack | creatureAlive | creatureKing1 },
    { eTownDungeon, 6, "bkdr", "cbdrgn.def", creatureDoubleWide | creatureFlyingArmy | creatureHasExtendedAttack | creatureAlive | creatureKing1 },
    { eTownStronghold, 0, "gbln", "cgobli.def", creatureAlive },
    { eTownStronghold, 0, "hgob", "chgobl.def", creatureAlive },
    { eTownStronghold, 1, "gwrd", "cbwlfr.def", creatureDoubleWide | creatureAlive },
    { eTownStronghold, 1, "hgwr", "cuwlfr.def", creatureDoubleWide | creatureAlive | creatureTwoAttacks },
    { eTownStronghold, 2, "oorc", "corc.def", creatureShootingArmy | creatureAlive },
    { eTownStronghold, 2, "orcc", "corcch.def", creatureShootingArmy | creatureAlive },
    { eTownStronghold, 3, "ogre", "cogre.def", creatureAlive },
    { eTownStronghold, 3, "ogrm", "cogmag.def", creatureAlive },
    { eTownStronghold, 4, "rocc", "croc.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownStronghold, 4, "tbrd", "ctbird.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownStronghold, 5, "ccyc", "ccyclr.def", creatureShootingArmy | creatureAlive | creatureCatapult },
    { eTownStronghold, 5, "cycl", "ccycllor.def", creatureShootingArmy | creatureAlive | creatureCatapult },
    { eTownStronghold, 6, "ybmh", "cybehe.def", creatureDoubleWide | creatureAlive | creatureKing1 },
    { eTownStronghold, 6, "bmth", "cabehe.def", creatureDoubleWide | creatureAlive | creatureKing1 },
    { eTownFortress, 0, "gnol", "cgnoll.def", creatureAlive },
    { eTownFortress, 0, "gnlm", "cgnolm.def", creatureAlive },
    { eTownFortress, 1, "pliz", "cpliza.def", creatureShootingArmy | creatureAlive },
    { eTownFortress, 1, "aliz", "caliza.def", creatureShootingArmy | creatureAlive },
    { eTownFortress, 4, "cgor", "ccgorg.def", creatureDoubleWide | creatureAlive },
    { eTownFortress, 4, "bgor", "cbgog.def", creatureDoubleWide | creatureAlive },
    { eTownFortress, 2, "dfly", "cdrfly.def", creatureFlyingArmy | creatureAlive },
    { eTownFortress, 2, "fdfl", "cdrfir.def", creatureFlyingArmy | creatureAlive },
    { eTownFortress, 3, "basl", "cbasil.def", creatureDoubleWide | creatureAlive },
    { eTownFortress, 3, "gbas", "cgbasi.def", creatureDoubleWide | creatureAlive },
    { eTownFortress, 5, "wyvn", "cwyver.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownFortress, 5, "wyvm", "cwyvmn.def", creatureDoubleWide | creatureFlyingArmy | creatureAlive },
    { eTownFortress, 6, "hydr", "chydra.def", creatureDoubleWide | creatureAlive | creatureKing1 | creatureFreeAttack | creatureMultiHeaded },
    { eTownFortress, 6, "chyd", "cchydr.def", creatureDoubleWide | creatureAlive | creatureKing1 | creatureFreeAttack | creatureMultiHeaded },
    { eTownNeutral, 3, "aelm", "caelem.def", creatureNoMorale },
    { eTownNeutral, 4, "eelm", "ceelem.def", creatureNoMorale },
    { eTownNeutral, 3, "felm", "cfelem.def", creatureImmuneToFireSpells | creatureNoMorale },
    { eTownNeutral, 3, "welm", "cwelem.def", creatureDoubleWide | creatureNoMorale },
    { eTownNeutral, 4, "gglm", "cggole.def", creatureImmuneToMindSpells | creatureNoMorale },
    { eTownNeutral, 5, "dglm", "cdgole.def", creatureImmuneToMindSpells | creatureNoMorale },
    { eTownNeutral, 0, "cata", "smcata.def", creatureDoubleWide | creatureShootingArmy | creatureCatapult | creatureSiegeWeapon | creatureImmuneToMindSpells | creatureNoMorale },
    { eTownNeutral, 4, "ball", "smbal.def", creatureDoubleWide | creatureShootingArmy | creatureSiegeWeapon | creatureImmuneToMindSpells | creatureNoMeleePenalty | creatureFreeAttack | creatureNoMorale },
    { eTownNeutral, 0, "faid", "smtent.def", creatureDoubleWide | creatureSiegeWeapon | creatureImmuneToMindSpells | creatureNoMorale },
    { eTownNeutral, 0, "cart", "smcart.def", creatureSiegeWeapon | creatureImmuneToMindSpells | creatureNoMorale },
};

const TCreatureTypeTraits (&akCreatureTypeTraits)[kNumCreatureAndSiegeWeaponTypes] = aCreatureTypeTraitsImp;

TCreatureType GetBaseCreature(TTownType townType, int baseCreatureNbr)
{
#line 201
    assert(townType != eTownNeutral);
    assert(baseCreatureNbr >= 0 && baseCreatureNbr < kNumCreatureTypesPerTown);
    if (baseCreatureNbr < 0 || baseCreatureNbr >= kNumCreatureTypesPerTown)
        return eCreaturePikeman;
    return akBaseCreatures[townType][baseCreatureNbr];
}

bool IsBaseCreature(TCreatureType type)
{
    for (int town = 0; town < kNumTownTypes; town++)
        for (int nbr = 0; nbr < kNumCreatureTypesPerTown; nbr++)
            if (type == akBaseCreatures[town][nbr])
                return true;
    return false;
}

bool IsSiegeWeapon(TCreatureType type)
{
    switch (type) {
    case eCreatureCatapult:
    case eCreatureBallista:
    case eCreatureFirstAidTent:
    case eCreatureAmmoCart:
        return true;
    default:
        return false;
    }
}

TCreatureType UpgradedCreatureType(TCreatureType type)
{
    if (!IsBaseCreature(type))
        return eCreatureNone;
    return TCreatureType(type + 1);
}

static void InitializeCreatureTypeTraits(int id, const vector<char*>& resource);

bool InitializeCreatureTypeTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet("crtraits.txt");
    if (!resource)
        return false;
    if (resource->GetNumberOfRows() < 151) {
        ResourceManager::Dispose(resource);
        return false;
    }

    int i;
    int row = 0;
    int id = 0;
    row += 2;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 14; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 6; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 4; i++) {
        InitializeCreatureTypeTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    ResourceManager::Dispose(resource);
    return true;
}

namespace {

// Owns one loaded string; the loader keeps the pointers in the traits rows.
class TAutoStrPtr {
public:
    TAutoStrPtr() : str(0) {}
    ~TAutoStrPtr() { delete[] str; }
    void set(char* newStr) { str = newStr; }
    char* get() const { return str; }

private:
    char* str;
};

}

static void InitializeCreatureTypeTraits(int id, const vector<char*>& resource)
{
#line 413
    assert(id >= 0 && id < kNumCreatureAndSiegeWeaponTypes);
    assert(resource.size() >= 23);
    TCreatureTypeTraits* const traits = &aCreatureTypeTraitsImp[id];

    static TAutoStrPtr aNameAutoStrs[kNumCreatureAndSiegeWeaponTypes];
    aNameAutoStrs[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(aNameAutoStrs[id].get(), resource[0]);
    traits->m_name = aNameAutoStrs[id].get();

    static TAutoStrPtr aPluralAutoStrs[kNumCreatureAndSiegeWeaponTypes];
    aPluralAutoStrs[id].set(new char[strlen(resource[1]) + 1]);
    strcpy(aPluralAutoStrs[id].get(), resource[1]);
    traits->m_plural_name = aPluralAutoStrs[id].get();

    traits->cost[0] = atoi(resource[2]);
    traits->cost[1] = atoi(resource[3]);
    traits->cost[2] = atoi(resource[4]);
    traits->cost[3] = atoi(resource[5]);
    traits->cost[4] = atoi(resource[6]);
    traits->cost[5] = atoi(resource[7]);
    traits->cost[6] = atoi(resource[8]);
    traits->baseFightValue = atoi(resource[9]);
    traits->AI_value = atoi(resource[10]);
    traits->growthRate = atoi(resource[11]);
    traits->horde_growth_rate = atoi(resource[12]);
    traits->hitPoints = atoi(resource[13]);
    traits->speed = atoi(resource[14]);
    traits->attackSkill = atoi(resource[15]);
    traits->defenseSkill = atoi(resource[16]);
    traits->damageLowBound = atoi(resource[17]);
    traits->damageHighBound = atoi(resource[18]);
    traits->numShots = atoi(resource[19]);
    traits->wanderingLow = atoi(resource[20]);
    traits->wanderingHigh = atoi(resource[21]);

    static TAutoStrPtr special_ability_strings[kNumCreatureAndSiegeWeaponTypes];
    special_ability_strings[id].set(new char[strlen(resource[22]) + 1]);
    strcpy(special_ability_strings[id].get(), resource[22]);
    traits->special_ability = special_ability_strings[id].get();
}
