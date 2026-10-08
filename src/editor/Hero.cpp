// Hero.cpp - Loki h3maped object 15: the hero prototypes (the game's hero
// table copied into one prototype per hero, eight per class, and one for
// the random hero), the class, primary skill, secondary skill and mastery
// name tables, and the heroes on the map: specific heroes, random heroes
// and prisons. Assert and throw lines come from the retail immediates.
#include "editor/Hero.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <algorithm>
#include <functional>
#include <iostream.h>
#include <map>
#include <string>
#include <vector>

#include "adventureobjecttype.h"
#include "autoarrayptr.h"
#include "exceptions.h"
#include "herodefs.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "sskilltraits.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"
#include "textresource.h"

namespace {
TObjectTypeTable aHeroObjType(kNumHeroClasses + 1);

THero::TClassTraits aHeroClassTraitsImp[kNumHeroClasses + 1] = {
    THero::TClassTraits(aHeroObjType[eClassKnight], eHeroGier, eTownCastle),
    THero::TClassTraits(aHeroObjType[eClassCleric], eHeroRion, eTownCastle),
    THero::TClassTraits(aHeroObjType[eClassRanger], eHeroMephala, eTownRampart),
    THero::TClassTraits(aHeroObjType[eClassDruid], eHeroCoronius, eTownRampart),
    THero::TClassTraits(aHeroObjType[eClassAlchemist], eHeroPiquedram, eTownTower),
    THero::TClassTraits(aHeroObjType[eClassWizard], eHeroAstral, eTownTower),
    THero::TClassTraits(aHeroObjType[eClassPagan], eHeroFion, eTownInferno),
    THero::TClassTraits(aHeroObjType[eClassHeretic], eHeroAyden, eTownInferno),
    THero::TClassTraits(aHeroObjType[eClassDeathKnight], eHeroStraker, eTownNecropolis),
    THero::TClassTraits(aHeroObjType[eClassNecromancer], eHeroSeptienna, eTownNecropolis),
    THero::TClassTraits(aHeroObjType[eClassOverlord], eHeroLorelei, eTownDungeon),
    THero::TClassTraits(aHeroObjType[eClassWarlock], eHeroAlamar, eTownDungeon),
    THero::TClassTraits(aHeroObjType[eClassBarbarian], eHeroYog, eTownStronghold),
    THero::TClassTraits(aHeroObjType[eClassBattleMage], eHeroGird, eTownStronghold),
    THero::TClassTraits(aHeroObjType[eClassBeastmaster], eHeroBron, eTownFortress),
    THero::TClassTraits(aHeroObjType[eClassWitch], eHeroMirlanda, eTownFortress),
    THero::TClassTraits(aHeroObjType[kNumHeroClasses], eHeroNone, kNumTownTypes)
};

THero::TPrimarySkillTraits aHeroPrimarySkillTraitsImp[kNumPrimarySkills];
THero::TSecondarySkillTraits aHeroSecondarySkillTraitsImp[kNumSecSkills];
THero::TSkillMasteryTraits aHeroSkillMasteryTraitsImp[kNumMasteries];

// A hero's prototype as the game's hero table describes it.
class TCopiedProto : public THeroPrototype {
public:
    TCopiedProto(THeroID heroID);
};

TCopiedProto::TCopiedProto(THeroID heroID)
{
#line 67
    assert(heroID >= 0 && heroID < kNumHeroes);
    const THeroTraits& kHeroTraits = akHeroTraits[heroID];
#line 72
    assert(kHeroTraits.m_1stSkill >= 0 && kHeroTraits.m_1stSkill < kNumSecSkills && kHeroTraits.m_1stSkillLevel >= eMasteryBasic && kHeroTraits.m_1stSkillLevel < eMasteryBasic + kNumMasteries);
#line 75
    assert(kHeroTraits.m_2ndSkill == eSecSkillNone || ( kHeroTraits.m_2ndSkill >= 0 && kHeroTraits.m_2ndSkill < kNumSecSkills && kHeroTraits.m_2ndSkillLevel >= eMasteryBasic && kHeroTraits.m_2ndSkillLevel < eMasteryBasic + kNumMasteries ));
    setName(kHeroTraits.m_name);
    setPortrait(heroID);
    map<TSecondarySkill, TSkillMastery> secondarySkills;
    secondarySkills[kHeroTraits.m_1stSkill] = kHeroTraits.m_1stSkillLevel;
    if (kHeroTraits.m_2ndSkill != eSecSkillNone)
        secondarySkills[kHeroTraits.m_2ndSkill] = kHeroTraits.m_2ndSkillLevel;
    setSecondarySkills(secondarySkills);
    TArtifactContainer artifacts;
    if (kHeroTraits.m_startsWithSpellbook)
        artifacts.setSlot(eArtifactSlotSpellbook, eArtifactSpellbook);
    setArtifacts(artifacts);
}
}

THero::TClassTraits* THero::s_akClassTraits = aHeroClassTraitsImp;
THero::TPrimarySkillTraits* THero::s_akPrimarySkillTraits = aHeroPrimarySkillTraitsImp;
THero::TSecondarySkillTraits* THero::s_akSecondarySkillTraits = aHeroSecondarySkillTraitsImp;
THero::TSkillMasteryTraits* THero::s_akSkillMasteryTraits = aHeroSkillMasteryTraitsImp;

THeroPrototype::THeroPrototype() : _m_portrait(0)
{
}

THeroPrototype::THeroPrototype(const string& name, int portrait,
                               const map<TSecondarySkill, TSkillMastery>& secondarySkills,
                               const TArtifactContainer& artifacts)
    : _m_name(name), _m_portrait(portrait), _m_secondarySkills(secondarySkills), _m_artifacts(artifacts)
{
#line 147
    assert(_m_name.size() <= s_kMaxNameLen);
    assert(_m_artifacts.getBackpack().size() <= s_kMaxBackpackSize);
}

void THeroPrototype::setName(string newName)
{
#line 154
    assert(newName.size() <= s_kMaxNameLen);
    assert(newName.find( '\n' ) == std::string::npos);
    assert(newName.find( '\t' ) == std::string::npos);
    _m_name = newName;
}

void THeroPrototype::setPortrait(int newPortrait)
{
    _m_portrait = newPortrait;
}

void THeroPrototype::setSecondarySkills(const map<TSecondarySkill, TSkillMastery>& newSecondarySkills)
{
#line 169
    assert(newSecondarySkills.size() <= s_kMaxSecSkills);
    _m_secondarySkills = newSecondarySkills;
}

void THeroPrototype::setArmy(const TArmy& newArmy)
{
    _m_army = newArmy;
}

void THeroPrototype::setArtifacts(const TArtifactContainer& newArtifacts)
{
#line 200
    assert(newArtifacts.getBackpack().size() <= s_kMaxBackpackSize);
    _m_artifacts = newArtifacts;
}

void THeroPrototype::TArtifactContainer::setSlot(TArtifactSlot slot, TArtifact artifact)
{
#line 210
    assert(slot >= 0 && slot < kNumArtifactSlots);
    assert(artifact >= 0 && artifact < kNumArtifacts);
    assert(artifactAllowedInSlot( artifact, slot ));
    _m_aSlot[slot] = artifact;
}

void THeroPrototype::TArtifactContainer::setBackpack(const vector<TArtifact>& newBackpack)
{
    for (vector<TArtifact>::const_iterator iter = newBackpack.begin(); iter != newBackpack.end(); ++iter)
#line 223
        assert(*iter >= 0 && *iter < kNumArtifacts);
    _m_backpack = newBackpack;
}

void THero::initialize()
{
#line 247
    assert(kRandomStr != NULL);
    assert(kUnknownStr != NULL);
    static TAutoArrayPtr<char> pRandomName(new char[strlen(kRandomStr) + 1]);
    if (!pRandomName.get())
#line 253
        throw TAllocationFailure(__FILE__, __LINE__);
    strcpy(pRandomName.get(), kRandomStr);
    static TAutoArrayPtr<char> pUnknownName(new char[strlen(kUnknownStr) + 1]);
    if (!pUnknownName.get())
#line 258
        throw TAllocationFailure(__FILE__, __LINE__);
    strcpy(pUnknownName.get(), kUnknownStr);
    aHeroObjType.load("heroes.txt");
#line 263
    assert(aHeroObjType.size() == kNumHeroClasses + 1);
    {
        static TAutoArrayPtr<char> pNames(0);
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("priskill.txt"));
        if (!pTextResource.get())
#line 272
            throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"priskill.txt\".");
        assert(pTextResource->GetNumberOfStrings() >= kNumPrimarySkills);
        int size = 0;
        unsigned int i;
        for (i = 0; i < kNumPrimarySkills; i++)
            size += strlen(pTextResource->GetText(i)) + 1;
        pNames = TAutoArrayPtr<char>(new char[size]);
        if (!pNames.get())
#line 284
            throw TAllocationFailure(__FILE__, __LINE__);
        char* p = pNames.get();
        for (i = 0; i < kNumPrimarySkills; i++) {
            const char* text = pTextResource->GetText(i);
            int length = strlen(text) + 1;
            memcpy(p, text, length);
            aHeroPrimarySkillTraitsImp[i].m_name = p;
            p += length;
        }
    }
    for (unsigned int skill = 0; skill < kNumSecSkills; skill++)
        aHeroSecondarySkillTraitsImp[skill].m_name = akSSkillTraits[skill].m_name;
    {
        static TAutoArrayPtr<char> pNames(0);
        TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("skilllev.txt"));
        if (!pTextResource.get())
#line 310
            throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"skilllev.txt\".");
        assert(pTextResource->GetNumberOfStrings() >= kNumMasteries);
        int size = 0;
        unsigned int i;
        for (i = 0; i < kNumMasteries; i++)
            size += strlen(pTextResource->GetText(i)) + 1;
        pNames = TAutoArrayPtr<char>(new char[size]);
        if (!pNames.get())
#line 322
            throw TAllocationFailure(__FILE__, __LINE__);
        char* p = pNames.get();
        for (i = 0; i < kNumMasteries; i++) {
            const char* text = pTextResource->GetText(i);
            int length = strlen(text) + 1;
            memcpy(p, text, length);
            aHeroSkillMasteryTraitsImp[i].m_name = p;
            p += length;
        }
    }
    static THeroPrototype aKnightPrototypes[] = {
        TCopiedProto(THeroID(0)), TCopiedProto(THeroID(1)), TCopiedProto(THeroID(2)), TCopiedProto(THeroID(3)),
        TCopiedProto(THeroID(4)), TCopiedProto(THeroID(5)), TCopiedProto(THeroID(6)), TCopiedProto(THeroID(7))
    };
    static THeroPrototype aClericPrototypes[] = {
        TCopiedProto(THeroID(8)), TCopiedProto(THeroID(9)), TCopiedProto(THeroID(10)), TCopiedProto(THeroID(11)),
        TCopiedProto(THeroID(12)), TCopiedProto(THeroID(13)), TCopiedProto(THeroID(14)), TCopiedProto(THeroID(15))
    };
    static THeroPrototype aRangerPrototypes[] = {
        TCopiedProto(THeroID(16)), TCopiedProto(THeroID(17)), TCopiedProto(THeroID(18)), TCopiedProto(THeroID(19)),
        TCopiedProto(THeroID(20)), TCopiedProto(THeroID(21)), TCopiedProto(THeroID(22)), TCopiedProto(THeroID(23))
    };
    static THeroPrototype aDruidPrototypes[] = {
        TCopiedProto(THeroID(24)), TCopiedProto(THeroID(25)), TCopiedProto(THeroID(26)), TCopiedProto(THeroID(27)),
        TCopiedProto(THeroID(28)), TCopiedProto(THeroID(29)), TCopiedProto(THeroID(30)), TCopiedProto(THeroID(31))
    };
    static THeroPrototype aAlchemistPrototypes[] = {
        TCopiedProto(THeroID(32)), TCopiedProto(THeroID(33)), TCopiedProto(THeroID(34)), TCopiedProto(THeroID(35)),
        TCopiedProto(THeroID(36)), TCopiedProto(THeroID(37)), TCopiedProto(THeroID(38)), TCopiedProto(THeroID(39))
    };
    static THeroPrototype aWizardPrototypes[] = {
        TCopiedProto(THeroID(40)), TCopiedProto(THeroID(41)), TCopiedProto(THeroID(42)), TCopiedProto(THeroID(43)),
        TCopiedProto(THeroID(44)), TCopiedProto(THeroID(45)), TCopiedProto(THeroID(46)), TCopiedProto(THeroID(47))
    };
    static THeroPrototype aPaganPrototypes[] = {
        TCopiedProto(THeroID(48)), TCopiedProto(THeroID(49)), TCopiedProto(THeroID(50)), TCopiedProto(THeroID(51)),
        TCopiedProto(THeroID(52)), TCopiedProto(THeroID(53)), TCopiedProto(THeroID(54)), TCopiedProto(THeroID(55))
    };
    static THeroPrototype aHereticPrototypes[] = {
        TCopiedProto(THeroID(56)), TCopiedProto(THeroID(57)), TCopiedProto(THeroID(58)), TCopiedProto(THeroID(59)),
        TCopiedProto(THeroID(60)), TCopiedProto(THeroID(61)), TCopiedProto(THeroID(62)), TCopiedProto(THeroID(63))
    };
    static THeroPrototype aDeathKnightPrototypes[] = {
        TCopiedProto(THeroID(64)), TCopiedProto(THeroID(65)), TCopiedProto(THeroID(66)), TCopiedProto(THeroID(67)),
        TCopiedProto(THeroID(68)), TCopiedProto(THeroID(69)), TCopiedProto(THeroID(70)), TCopiedProto(THeroID(71))
    };
    static THeroPrototype aNecromancerPrototypes[] = {
        TCopiedProto(THeroID(72)), TCopiedProto(THeroID(73)), TCopiedProto(THeroID(74)), TCopiedProto(THeroID(75)),
        TCopiedProto(THeroID(76)), TCopiedProto(THeroID(77)), TCopiedProto(THeroID(78)), TCopiedProto(THeroID(79))
    };
    static THeroPrototype aOverlordPrototypes[] = {
        TCopiedProto(THeroID(80)), TCopiedProto(THeroID(81)), TCopiedProto(THeroID(82)), TCopiedProto(THeroID(83)),
        TCopiedProto(THeroID(84)), TCopiedProto(THeroID(85)), TCopiedProto(THeroID(86)), TCopiedProto(THeroID(87))
    };
    static THeroPrototype aWarlockPrototypes[] = {
        TCopiedProto(THeroID(88)), TCopiedProto(THeroID(89)), TCopiedProto(THeroID(90)), TCopiedProto(THeroID(91)),
        TCopiedProto(THeroID(92)), TCopiedProto(THeroID(93)), TCopiedProto(THeroID(94)), TCopiedProto(THeroID(95))
    };
    static THeroPrototype aBarbarianPrototypes[] = {
        TCopiedProto(THeroID(96)), TCopiedProto(THeroID(97)), TCopiedProto(THeroID(98)), TCopiedProto(THeroID(99)),
        TCopiedProto(THeroID(100)), TCopiedProto(THeroID(101)), TCopiedProto(THeroID(102)), TCopiedProto(THeroID(103))
    };
    static THeroPrototype aBattleMagePrototypes[] = {
        TCopiedProto(THeroID(104)), TCopiedProto(THeroID(105)), TCopiedProto(THeroID(106)), TCopiedProto(THeroID(107)),
        TCopiedProto(THeroID(108)), TCopiedProto(THeroID(109)), TCopiedProto(THeroID(110)), TCopiedProto(THeroID(111))
    };
    static THeroPrototype aBeastmasterPrototypes[] = {
        TCopiedProto(THeroID(112)), TCopiedProto(THeroID(113)), TCopiedProto(THeroID(114)), TCopiedProto(THeroID(115)),
        TCopiedProto(THeroID(116)), TCopiedProto(THeroID(117)), TCopiedProto(THeroID(118)), TCopiedProto(THeroID(119))
    };
    static THeroPrototype aWitchPrototypes[] = {
        TCopiedProto(THeroID(120)), TCopiedProto(THeroID(121)), TCopiedProto(THeroID(122)), TCopiedProto(THeroID(123)),
        TCopiedProto(THeroID(124)), TCopiedProto(THeroID(125)), TCopiedProto(THeroID(126)), TCopiedProto(THeroID(127))
    };
    static THeroPrototype aRandomPrototypes[] = {
        THeroPrototype(pUnknownName.get(), -1, map<TSecondarySkill, TSkillMastery>(),
                       THeroPrototype::TArtifactContainer())
    };
    aHeroClassTraitsImp[eClassKnight].m_name = akHeroClassTraits[eClassKnight].m_name;
    aHeroClassTraitsImp[eClassKnight].m_numPrototypes = sizeof(aKnightPrototypes) / sizeof(aKnightPrototypes[0]);
    aHeroClassTraitsImp[eClassKnight].m_aPrototype = aKnightPrototypes;
    aHeroClassTraitsImp[eClassCleric].m_name = akHeroClassTraits[eClassCleric].m_name;
    aHeroClassTraitsImp[eClassCleric].m_numPrototypes = sizeof(aClericPrototypes) / sizeof(aClericPrototypes[0]);
    aHeroClassTraitsImp[eClassCleric].m_aPrototype = aClericPrototypes;
    aHeroClassTraitsImp[eClassRanger].m_name = akHeroClassTraits[eClassRanger].m_name;
    aHeroClassTraitsImp[eClassRanger].m_numPrototypes = sizeof(aRangerPrototypes) / sizeof(aRangerPrototypes[0]);
    aHeroClassTraitsImp[eClassRanger].m_aPrototype = aRangerPrototypes;
    aHeroClassTraitsImp[eClassDruid].m_name = akHeroClassTraits[eClassDruid].m_name;
    aHeroClassTraitsImp[eClassDruid].m_numPrototypes = sizeof(aDruidPrototypes) / sizeof(aDruidPrototypes[0]);
    aHeroClassTraitsImp[eClassDruid].m_aPrototype = aDruidPrototypes;
    aHeroClassTraitsImp[eClassAlchemist].m_name = akHeroClassTraits[eClassAlchemist].m_name;
    aHeroClassTraitsImp[eClassAlchemist].m_numPrototypes = sizeof(aAlchemistPrototypes) / sizeof(aAlchemistPrototypes[0]);
    aHeroClassTraitsImp[eClassAlchemist].m_aPrototype = aAlchemistPrototypes;
    aHeroClassTraitsImp[eClassWizard].m_name = akHeroClassTraits[eClassWizard].m_name;
    aHeroClassTraitsImp[eClassWizard].m_numPrototypes = sizeof(aWizardPrototypes) / sizeof(aWizardPrototypes[0]);
    aHeroClassTraitsImp[eClassWizard].m_aPrototype = aWizardPrototypes;
    aHeroClassTraitsImp[eClassPagan].m_name = akHeroClassTraits[eClassPagan].m_name;
    aHeroClassTraitsImp[eClassPagan].m_numPrototypes = sizeof(aPaganPrototypes) / sizeof(aPaganPrototypes[0]);
    aHeroClassTraitsImp[eClassPagan].m_aPrototype = aPaganPrototypes;
    aHeroClassTraitsImp[eClassHeretic].m_name = akHeroClassTraits[eClassHeretic].m_name;
    aHeroClassTraitsImp[eClassHeretic].m_numPrototypes = sizeof(aHereticPrototypes) / sizeof(aHereticPrototypes[0]);
    aHeroClassTraitsImp[eClassHeretic].m_aPrototype = aHereticPrototypes;
    aHeroClassTraitsImp[eClassDeathKnight].m_name = akHeroClassTraits[eClassDeathKnight].m_name;
    aHeroClassTraitsImp[eClassDeathKnight].m_numPrototypes = sizeof(aDeathKnightPrototypes) / sizeof(aDeathKnightPrototypes[0]);
    aHeroClassTraitsImp[eClassDeathKnight].m_aPrototype = aDeathKnightPrototypes;
    aHeroClassTraitsImp[eClassNecromancer].m_name = akHeroClassTraits[eClassNecromancer].m_name;
    aHeroClassTraitsImp[eClassNecromancer].m_numPrototypes = sizeof(aNecromancerPrototypes) / sizeof(aNecromancerPrototypes[0]);
    aHeroClassTraitsImp[eClassNecromancer].m_aPrototype = aNecromancerPrototypes;
    aHeroClassTraitsImp[eClassOverlord].m_name = akHeroClassTraits[eClassOverlord].m_name;
    aHeroClassTraitsImp[eClassOverlord].m_numPrototypes = sizeof(aOverlordPrototypes) / sizeof(aOverlordPrototypes[0]);
    aHeroClassTraitsImp[eClassOverlord].m_aPrototype = aOverlordPrototypes;
    aHeroClassTraitsImp[eClassWarlock].m_name = akHeroClassTraits[eClassWarlock].m_name;
    aHeroClassTraitsImp[eClassWarlock].m_numPrototypes = sizeof(aWarlockPrototypes) / sizeof(aWarlockPrototypes[0]);
    aHeroClassTraitsImp[eClassWarlock].m_aPrototype = aWarlockPrototypes;
    aHeroClassTraitsImp[eClassBarbarian].m_name = akHeroClassTraits[eClassBarbarian].m_name;
    aHeroClassTraitsImp[eClassBarbarian].m_numPrototypes = sizeof(aBarbarianPrototypes) / sizeof(aBarbarianPrototypes[0]);
    aHeroClassTraitsImp[eClassBarbarian].m_aPrototype = aBarbarianPrototypes;
    aHeroClassTraitsImp[eClassBattleMage].m_name = akHeroClassTraits[eClassBattleMage].m_name;
    aHeroClassTraitsImp[eClassBattleMage].m_numPrototypes = sizeof(aBattleMagePrototypes) / sizeof(aBattleMagePrototypes[0]);
    aHeroClassTraitsImp[eClassBattleMage].m_aPrototype = aBattleMagePrototypes;
    aHeroClassTraitsImp[eClassBeastmaster].m_name = akHeroClassTraits[eClassBeastmaster].m_name;
    aHeroClassTraitsImp[eClassBeastmaster].m_numPrototypes = sizeof(aBeastmasterPrototypes) / sizeof(aBeastmasterPrototypes[0]);
    aHeroClassTraitsImp[eClassBeastmaster].m_aPrototype = aBeastmasterPrototypes;
    aHeroClassTraitsImp[eClassWitch].m_name = akHeroClassTraits[eClassWitch].m_name;
    aHeroClassTraitsImp[eClassWitch].m_numPrototypes = sizeof(aWitchPrototypes) / sizeof(aWitchPrototypes[0]);
    aHeroClassTraitsImp[eClassWitch].m_aPrototype = aWitchPrototypes;
    aHeroClassTraitsImp[kNumHeroClasses].m_name = pRandomName.get();
    aHeroClassTraitsImp[kNumHeroClasses].m_numPrototypes = sizeof(aRandomPrototypes) / sizeof(aRandomPrototypes[0]);
    aHeroClassTraitsImp[kNumHeroClasses].m_aPrototype = aRandomPrototypes;
}

THero::THero(const TObjectType& objType, TPlayer owner, unsigned int protoNum)
    : TGameObject(objType), TPlayableObject(objType, owner), _m_bCustomName(false), _m_bCustomPortrait(false),
      _m_bCustomSecondarySkills(false), _m_bCustomArmy(false), _m_bCustomArtifacts(false), _m_protoNum(protoNum),
      _m_experience(0), _m_bGroupedFormation(false), _m_patrol(-1)
{
}

THero::THero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TPlayableObject(objType, pIStream, version)
{
    signed char bCustom;
    signed char indivID;
    *pIStream >> indivID;
    _m_protoNum = indivID;
    *pIStream >> bCustom;
    _m_bCustomName = bCustom != 0;
    if (_m_bCustomName) {
        string name;
        *pIStream >> name;
        setName(name);
    }
    long experience;
    *pIStream >> experience;
    setExperience(experience);
    *pIStream >> bCustom;
    _m_bCustomPortrait = bCustom != 0;
    if (_m_bCustomPortrait) {
        unsigned char portrait;
        *pIStream >> portrait;
        setPortrait(portrait);
    }
    *pIStream >> bCustom;
    _m_bCustomSecondarySkills = bCustom != 0;
    if (_m_bCustomSecondarySkills) {
        long numSecSkills;
        *pIStream >> numSecSkills;
        map<TSecondarySkill, TSkillMastery> secondarySkills;
#line 660
        assert(numSecSkills <= s_kMaxSecSkills);
        while (numSecSkills > 0) {
            --numSecSkills;
            signed char skill;
            signed char mastery;
            *pIStream >> skill >> mastery;
#line 670
            assert(secondarySkills.find( static_cast< TSecondarySkill >( skill ) ) == secondarySkills.end());
            secondarySkills[TSecondarySkill(skill)] = TSkillMastery(mastery);
        }
        setSecondarySkills(secondarySkills);
    }
    *pIStream >> bCustom;
    _m_bCustomArmy = bCustom != 0;
    if (_m_bCustomArmy) {
        TArmy army;
        *pIStream >> army;
        setArmy(army);
    }
    signed char bGroupedFormation;
    *pIStream >> bGroupedFormation;
    setBGroupedFormation(bGroupedFormation != 0);
    *pIStream >> bCustom;
    _m_bCustomArtifacts = bCustom != 0;
    if (_m_bCustomArtifacts) {
        THeroPrototype::TArtifactContainer artifacts;
        for (unsigned int slot = 0; slot < kNumArtifactSlots; ++slot) {
            signed char artifact;
            *pIStream >> artifact;
            if (artifact != -1)
                artifacts.setSlot(TArtifactSlot(slot), TArtifact(artifact));
        }
        short backpackSize;
        *pIStream >> backpackSize;
        artifacts.getPBackpack()->reserve(backpackSize);
        while (backpackSize-- > 0) {
            signed char artifact;
            *pIStream >> artifact;
            artifacts.getPBackpack()->push_back(TArtifact(artifact));
        }
        if (artifacts.getBackpack().size() > s_kMaxBackpackSize)
            artifacts.getPBackpack()->resize(s_kMaxBackpackSize);
        setArtifacts(artifacts);
    }
    signed char patrol;
    *pIStream >> patrol;
    setPatrol(patrol);
    signed char aReserved[16];
    *pIStream >> aReserved;
}

void THero::setProtoNum(unsigned int newProtoNum)
{
#line 730
    assert(newProtoNum < getClassTraits().m_numPrototypes);
    _m_protoNum = newProtoNum;
}

void THero::setExperience(int newExperience)
{
#line 737
    assert(newExperience >= 0 && newExperience <= s_kMaxExperience);
    _m_experience = newExperience;
}

void THero::importText(istream* pIStream)
{
#line 744
    assert(pIStream != NULL);
    string line;
    if (_m_bCustomName) {
        getline(*pIStream, line);
        if (line != string(kNameStr) + ':')
            throw TImportTextFailure();
        getline(*pIStream, line);
        if (line.size() > THeroPrototype::s_kMaxNameLen)
            line.erase(THeroPrototype::s_kMaxNameLen);
        replace(line.begin(), line.end(), '\t', ' ');
        if (_m_bCustomName && find_if(line.begin(), line.end(), not1(ptr_fun(isspace))) == line.end())
            throw TImportTextFailure();
        setName(line);
    }
}

bool THero::getBHasArtifact(TArtifact whichArtifact) const
{
#line 767
    assert(whichArtifact >= 0 && whichArtifact < kNumArtifacts);
    const THeroPrototype::TArtifactContainer& artifacts = getArtifacts();
    for (unsigned int slot = 0; slot < kNumArtifactSlots; ++slot) {
        if (artifacts.getSlot(TArtifactSlot(slot)) == whichArtifact) {
#line 775
            assert(akArtifactTraits[ whichArtifact ].m_allowableSlotMask[ slot ]);
            return true;
        }
    }
    for (vector<TArtifact>::const_iterator iter = artifacts.getBackpack().begin();
         iter != artifacts.getBackpack().end(); ++iter)
        if (*iter == whichArtifact)
            return true;
    return false;
}

bool THero::isCustomized() const
{
    return _m_bCustomName || _m_bCustomPortrait || _m_bCustomSecondarySkills || _m_bCustomArmy ||
           _m_bCustomArtifacts || _m_experience != 0 || _m_bGroupedFormation || _m_patrol != -1;
}

void THero::write(TRawOStream* pOStream) const
{
    TPlayableObject::write(pOStream);
    *pOStream << (signed char) getIndivID();
    *pOStream << (signed char) _m_bCustomName;
    if (_m_bCustomName)
        *pOStream << getCustomName();
    *pOStream << (const long&) _m_experience;
    *pOStream << (signed char) _m_bCustomPortrait;
    if (_m_bCustomPortrait)
        *pOStream << (unsigned char) getCustomPortrait();
    *pOStream << (signed char) _m_bCustomSecondarySkills;
    if (_m_bCustomSecondarySkills) {
        *pOStream << (long) getCustomSecondarySkills().size();
        for (map<TSecondarySkill, TSkillMastery>::const_iterator iter = getCustomSecondarySkills().begin();
             iter != getCustomSecondarySkills().end(); ++iter)
            *pOStream << (signed char) iter->first << (signed char) iter->second;
    }
    *pOStream << (signed char) _m_bCustomArmy;
    if (_m_bCustomArmy)
        *pOStream << getCustomArmy();
    *pOStream << (signed char) _m_bGroupedFormation;
    *pOStream << (signed char) _m_bCustomArtifacts;
    if (_m_bCustomArtifacts) {
        const THeroPrototype::TArtifactContainer& artifacts = getCustomArtifacts();
        for (unsigned int slot = 0; slot < kNumArtifactSlots; ++slot)
            *pOStream << (signed char) artifacts.getSlot(TArtifactSlot(slot));
        *pOStream << (short) artifacts.getBackpack().size();
        for (vector<TArtifact>::const_iterator iter = artifacts.getBackpack().begin();
             iter != artifacts.getBackpack().end(); ++iter)
            *pOStream << (signed char) *iter;
    }
    *pOStream << (signed char) getPatrol();
    signed char aReserved[16];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

void THero::exportText(ostream* pOStream) const
{
#line 864
    assert(pOStream != NULL);
    if (_m_bCustomName)
        *pOStream << kNameStr << ':' << '\n' << getName() << '\n';
}

TNonRandomHero::TNonRandomHero(const TObjectType& objType, TPlayer owner, unsigned int protoNum)
    : TGameObject(objType), THero(objType, owner, protoNum)
{
#line 878
    assert(objType.getType() == HERO);
    assert(objType.getExtra() >= 0 && objType.getExtra() < kNumHeroClasses);
    setPortrait(getClass() * 8);
}

TNonRandomHero::TNonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), THero(objType, pIStream, version)
{
#line 889
    assert(objType.getType() == HERO);
    assert(objType.getExtra() >= 0 && objType.getExtra() < kNumHeroClasses);
    protectedSetProtoNum(getProtoNum() - s_akClassTraits[getExtra()].m_firstHeroID);
#line 893
    assert(getProtoNum() < s_akClassTraits[ getExtra() ].m_numPrototypes);
    if (!getBCustomPortrait())
        setPortrait(getClass() * 8);
}

string TNonRandomHero::getTypeName() const
{
    return getClassTraits().m_name;
}

TRandomHero::TRandomHero(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), THero(objType, owner, 0)
{
#line 913
    assert(objType.getType() == RANDOM_HERO);
}

TRandomHero::TRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), THero(objType, pIStream, version)
{
#line 921
    assert(objType.getType() == RANDOM_HERO);
    protectedSetProtoNum(0);
}

TPrison::TPrison(const TObjectType& objType, THeroClass heroClass, unsigned int protoNum)
    : TGameObject(objType), THero(objType, ePlayerNone, protoNum), _m_class(heroClass)
{
#line 935
    assert(objType.getType() == PRISON);
    assert(_m_class >= 0 && _m_class < kNumHeroClasses);
    assert(protoNum < s_akClassTraits[ _m_class ].m_numPrototypes);
}

TPrison::TPrison(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), THero(objType, pIStream, version), _m_class(THeroClass(0))
{
#line 946
    assert(objType.getType() == PRISON);
    unsigned int indivID = getProtoNum();
    setClass(THeroClass(indivID / 8));
    protectedSetProtoNum(indivID - s_akClassTraits[_m_class].m_firstHeroID);
#line 953
    assert(getProtoNum() < s_akClassTraits[ _m_class ].m_numPrototypes);
    if (!getBCustomPortrait())
        setPortrait(_m_class * 8);
}

void TPrison::setClass(THeroClass newClass)
{
#line 962
    assert(newClass >= 0 && newClass < kNumHeroClasses);
    _m_class = newClass;
}
