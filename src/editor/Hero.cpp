// Hero.cpp - the heroes (h3maped 0x449dec..0x44cc85; Loki h3maped object
// 15): the hero prototypes and their binary form, the hero, class and
// secondary skill tables, and the heroes on the map: placeholders, random
// heroes, specific heroes and prisons.
//
// The Windows editors read a hero by the map's version and write it by the
// edition; Armageddon's Blade added the link id, the biography and the
// sex, Shadow of Death the spells and primary skills.
#include "editor/stdafx.h"

#include <algorithm>
#include <functional>

#include "va.h"
#include "herotraits.h"
#include "resourceptr.h"
#include "sskilltraits.h"
#include "textresource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"

const unsigned int kNumHeroReserved = 16;

VA(0x0044a035, 0x27)
TObjectTypeTable::TObjectTypeTable(unsigned int numTypes) : m_objectTypes(numTypes)
{
}

namespace {
DATA(0x0059e690) TObjectTypeTable aHeroObjType(kNumHeroClasses + 1);

DATA(0x0059e6a0) THero::TTraits aHeroTraitsImp[kNumHeroes];

DATA(0x0059f060) THero::TClassTraits aHeroClassTraitsImp[kNumHeroClasses + 1] = {
    THero::TClassTraits(aHeroObjType.m_objectTypes[0], TOWN_CASTLE),
    THero::TClassTraits(aHeroObjType.m_objectTypes[1], TOWN_CASTLE),
    THero::TClassTraits(aHeroObjType.m_objectTypes[2], TOWN_RAMPART),
    THero::TClassTraits(aHeroObjType.m_objectTypes[3], TOWN_RAMPART),
    THero::TClassTraits(aHeroObjType.m_objectTypes[4], TOWN_TOWER),
    THero::TClassTraits(aHeroObjType.m_objectTypes[5], TOWN_TOWER),
    THero::TClassTraits(aHeroObjType.m_objectTypes[6], TOWN_INFERNO),
    THero::TClassTraits(aHeroObjType.m_objectTypes[7], TOWN_INFERNO),
    THero::TClassTraits(aHeroObjType.m_objectTypes[8], TOWN_NECROPOLIS),
    THero::TClassTraits(aHeroObjType.m_objectTypes[9], TOWN_NECROPOLIS),
    THero::TClassTraits(aHeroObjType.m_objectTypes[10], TOWN_DUNGEON),
    THero::TClassTraits(aHeroObjType.m_objectTypes[11], TOWN_DUNGEON),
    THero::TClassTraits(aHeroObjType.m_objectTypes[12], TOWN_STRONGHOLD),
    THero::TClassTraits(aHeroObjType.m_objectTypes[13], TOWN_STRONGHOLD),
    THero::TClassTraits(aHeroObjType.m_objectTypes[14], TOWN_FORTRESS),
    THero::TClassTraits(aHeroObjType.m_objectTypes[15], TOWN_FORTRESS),
    THero::TClassTraits(aHeroObjType.m_objectTypes[16], TOWN_CONFLUX),
    THero::TClassTraits(aHeroObjType.m_objectTypes[17], TOWN_CONFLUX),
    THero::TClassTraits(aHeroObjType.m_objectTypes[18], kNumTownTypes)
};

DATA(0x0059f274) THero::TSecondarySkillTraits aHeroSecondarySkillTraitsImp[kNumSecSkills];
}

DATA(0x005857d4) THero::TTraits* THero::s_akTraits = aHeroTraitsImp;
DATA(0x005857d8) THero::TClassTraits* THero::s_akClassTraits = aHeroClassTraitsImp;
DATA(0x005857dc) THero::TSecondarySkillTraits* THero::s_akSecondarySkillTraits = aHeroSecondarySkillTraitsImp;
DATA(0x0059f2e8) THero::TPrimarySkillTraits* THero::s_akPrimarySkillTraits = akHeroPrimarySkillTraits;
DATA(0x0059e66c) THero::TSkillMasteryTraits* THero::s_akSkillMasteryTraits = akHeroSkillMasteryTraits;

VA(0x0044a2a6, 0x5b)
void THeroPrototype::TSecondarySkills::read(TRawIStream* pIStream, int version)
{
    long numSkills;
    *pIStream >> numSkills;
    for (unsigned int count = numSkills; count > 0; count--) {
        signed char skill;
        signed char mastery;
        *pIStream >> skill;
        *pIStream >> mastery;
        insert(value_type(TSecondarySkill(skill), TSkillMastery(mastery)));
    }
}

VA(0x0044a301, 0x66)
void THeroPrototype::TSecondarySkills::write(TRawOStream* pOStream, int version) const
{
    *pOStream << static_cast<long>(size());
    for (const_iterator iter = begin(); iter != end(); ++iter) {
        *pOStream << static_cast<signed char>(iter->first);
        *pOStream << static_cast<signed char>(iter->second);
    }
}

VA(0x0044a375, 0xb1)
void THeroPrototype::TArtifactContainer::read(TRawIStream* pIStream, int version)
{
    int numSlots = (version >= 27 ? 1 : 0) + kNumArtifactSlots;
    for (int slot = 0; slot < numSlots; slot++) {
        int artifact;
        if (version >= 21) {
            short value;
            *pIStream >> value;
            artifact = value;
        } else {
            signed char value;
            *pIStream >> value;
            artifact = value;
        }
        if (artifact != ARTIFACT_NONE)
            _m_aSlot[slot] = TArtifact(artifact);
    }
    short numBackpack;
    *pIStream >> numBackpack;
    for (unsigned int count = numBackpack; count > 0; count--) {
        int artifact;
        if (version >= 21) {
            short value;
            *pIStream >> value;
            artifact = value;
        } else {
            signed char value;
            *pIStream >> value;
            artifact = value;
        }
        _m_backpack.insert(TArtifact(artifact));
    }
}

VA(0x0044a426, 0xaf)
void THeroPrototype::TArtifactContainer::write(TRawOStream* pOStream, int version) const
{
    int numSlots = (version >= GAME_VERSION_SOD ? 1 : 0) + kNumArtifactSlots;
    for (int slot = 0; slot < numSlots; slot++) {
        TArtifact artifact = _m_aSlot[slot];
        if (version >= GAME_VERSION_AB)
            *pOStream << static_cast<short>(artifact);
        else
            *pOStream << static_cast<signed char>(artifact);
    }
    *pOStream << static_cast<short>(_m_backpack.size());
    for (std::multiset<TArtifact>::const_iterator iter = _m_backpack.begin(); iter != _m_backpack.end(); ++iter) {
        if (version >= GAME_VERSION_AB)
            *pOStream << static_cast<short>(*iter);
        else
            *pOStream << static_cast<signed char>(*iter);
    }
}

VA(0x0044a4d5, 0xf)
void THeroPrototype::TSpells::read(TRawIStream* pIStream, int version)
{
    readBitset(*pIStream, *this);
}

VA(0x0044a4e4, 0xf)
void THeroPrototype::TSpells::write(TRawOStream* pOStream, int version) const
{
    writeBitset(*pOStream, *this);
}

VA(0x0044a4f3, 0x29)
void THeroPrototype::TPrimarySkills::read(TRawIStream* pIStream, int version)
{
    for (unsigned int i = 0; i < kNumPrimarySkills; i++) {
        unsigned char value;
        *pIStream >> value;
        (*this)[i] = value;
    }
}

VA(0x0044a51c, 0x28)
void THeroPrototype::TPrimarySkills::write(TRawOStream* pOStream, int version) const
{
    for (unsigned int i = 0; i < kNumPrimarySkills; i++)
        *pOStream << static_cast<unsigned char>((*this)[i]);
}

VA(0x0044a544, 0x3e)
void THeroPrototype::setName(const std::string& newName)
{
    if (newName != getName())
        _m_pImpl->m_name = newName;
}

VA(0x0044a582, 0x3e)
void THeroPrototype::setBiography(const std::string& newBiography)
{
    if (newBiography != getBiography())
        _m_pImpl->m_biography = newBiography;
}

VA(0x0044a5c0, 0x4e)
void THeroPrototype::setSecondarySkills(const TSecondarySkills& newSecondarySkills)
{
    if (newSecondarySkills != getSecondarySkills())
        *_m_pImpl->m_pSecondarySkills = newSecondarySkills;
}

VA(0x0044a60e, 0x4e)
void THeroPrototype::setArtifacts(const TArtifactContainer& newArtifacts)
{
    if (newArtifacts != getArtifacts())
        *_m_pImpl->m_pArtifacts = newArtifacts;
}

VA(0x0044a65c, 0x36)
void THeroPrototype::setSpells(const TSpells& newSpells)
{
    if (newSpells != getSpells())
        _m_pImpl->m_spells = newSpells;
}

void THeroPrototype::setPrimarySkills(const TArray<int, kNumPrimarySkills>& newPrimarySkills)
{
    if (!(newPrimarySkills == getPrimarySkills()))
        _m_pImpl->m_aPrimarySkill = newPrimarySkills;
}

VA(0x0044a6de, 0x23)
void THeroPrototype::setSex(int newSex)
{
    if (newSex != getSex())
        _m_pImpl->m_sex = newSex;
}

VA(0x0044a701, 0x23)
void THeroPrototype::setPortrait(int newPortrait)
{
    if (newPortrait != getPortrait())
        _m_pImpl->m_portrait = newPortrait;
}

VA(0x0044a724, 0x23)
void THeroPrototype::setExperience(int newExperience)
{
    if (newExperience != getExperience())
        _m_pImpl->m_experience = newExperience;
}

VA(0x0044a747, 0x33)
void THeroPrototype::setAvailability(const TPlayerMask& newAvailability)
{
    if (newAvailability != getAvailability())
        _m_pImpl->m_availability = newAvailability;
}

VA(0x0044a77a, 0x108)
bool operator==(const THeroPrototype& lhs, const THeroPrototype& rhs)
{
    return lhs._m_pImpl.get() == rhs._m_pImpl.get()
           || ((lhs._m_pImpl->m_pSecondarySkills.get() == rhs._m_pImpl->m_pSecondarySkills.get()
                || *lhs._m_pImpl->m_pSecondarySkills == *rhs._m_pImpl->m_pSecondarySkills)
               && (lhs._m_pImpl->m_pArtifacts.get() == rhs._m_pImpl->m_pArtifacts.get()
                   || *lhs._m_pImpl->m_pArtifacts == *rhs._m_pImpl->m_pArtifacts)
               && lhs._m_pImpl->m_name == rhs._m_pImpl->m_name
               && lhs._m_pImpl->m_biography == rhs._m_pImpl->m_biography
               && lhs._m_pImpl->m_spells == rhs._m_pImpl->m_spells
               && lhs._m_pImpl->m_aPrimarySkill == rhs._m_pImpl->m_aPrimarySkill
               && lhs._m_pImpl->m_sex == rhs._m_pImpl->m_sex && lhs._m_pImpl->m_portrait == rhs._m_pImpl->m_portrait
               && lhs._m_pImpl->m_experience == rhs._m_pImpl->m_experience
               && lhs._m_pImpl->m_availability == rhs._m_pImpl->m_availability);
}

namespace {

// Each hero's prototype from the game's hero table, its biography from
// HeroBios.txt, and the class tables' names and heroes.
VA(0x0044a8b5, 0x3b0)
void initializeTraitsTable()
{
    aHeroObjType.load("heroes.txt");
    for (int heroClass = 0; heroClass < kNumHeroClasses; heroClass++)
        aHeroClassTraitsImp[heroClass].m_name = akHeroClassTraits[heroClass].m_name;
    aHeroClassTraitsImp[kNumHeroClasses].m_name = kRandomStr;
    TResourcePtr<TTextResource> pBiographies(ResourceManager::GetText("HeroBios.txt"));
    if (!pBiographies.get())
        throw TRuntimeError();
    for (THeroID heroID = 0; heroID < kNumHeroes; heroID++) {
        const THeroTraits& heroTraits = akHeroTraits[heroID];
        THero::TTraits& traits = aHeroTraitsImp[heroID];
        traits.m_class = heroTraits.m_class;
        traits.m_prototype.setName(heroTraits.m_defaultName);
        traits.m_prototype.setBiography(pBiographies->GetText(heroID));
        traits.m_prototype.setSex(heroTraits.m_sex);
        traits.m_prototype.setPortrait(heroID);
        traits.m_bSpecial = heroTraits.m_availability.m_special;
        if (heroTraits.m_1stSkill != eSecSkillNone) {
            THeroPrototype::TSecondarySkills secondarySkills;
            secondarySkills.insert(std::make_pair(heroTraits.m_1stSkill, heroTraits.m_1stSkillLevel));
            if (heroTraits.m_2ndSkill != eSecSkillNone)
                secondarySkills.insert(std::make_pair(heroTraits.m_2ndSkill, heroTraits.m_2ndSkillLevel));
            traits.m_prototype.setSecondarySkills(secondarySkills);
        }
        if (heroTraits.m_startsWithSpellbook) {
            THeroPrototype::TArtifactContainer artifacts;
            artifacts.setSlot(eArtifactSlotSpellbook, ARTIFACT_SPELLBOOK);
            traits.m_prototype.setArtifacts(artifacts);
        }
        if (heroTraits.m_startingSpell != SPELL_NONE) {
            THeroPrototype::TSpells spells;
            spells.set(heroTraits.m_startingSpell);
            traits.m_prototype.setSpells(spells);
        }
        {
            const THeroClassTraits& classTraits = akHeroClassTraits[heroTraits.m_class];
            TArray<int, kNumPrimarySkills> aPrimarySkill(0);
            std::copy(classTraits.m_initialPrimarySkill, classTraits.m_initialPrimarySkill + kNumPrimarySkills,
                      aPrimarySkill.begin());
            traits.m_prototype.setPrimarySkills(aPrimarySkill);
        }
        std::bitset<3> gameVersions(0);
        gameVersions.set(GAME_VERSION_ROE, heroTraits.m_abAvailableIn[0] != 0);
        gameVersions.set(GAME_VERSION_AB, heroTraits.m_abAvailableIn[1] != 0);
        gameVersions.set(GAME_VERSION_SOD, heroTraits.m_abAvailableIn[1] != 0);
        traits.m_gameVersions = gameVersions;
        aHeroClassTraitsImp[heroTraits.m_class].m_heroes.insert(heroID);
    }
}

}

VA(0x0044ac65, 0x7c)
TBasicHero::TBasicHero(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TPlayableObject(objType, owner)
{
}

VA(0x0044ace1, 0x7f)
TBasicHero::TBasicHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TPlayableObject(objType, pIStream, version)
{
}

VA(0x0044ad60, 0x4)
THeroID TBasicHero::getHeroID() const
{
    return -1;
}

VA(0x0044ad64, 0x81)
THeroPlaceholder::THeroPlaceholder(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TBasicHero(objType, owner), _m_powerRank(1), _m_heroID(-1)
{
}

VA(0x0044ade5, 0xbc)
THeroPlaceholder::THeroPlaceholder(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TBasicHero(objType, pIStream, version)
{
    unsigned char heroID;
    *pIStream >> heroID;
    _m_heroID = heroID != 0xff ? heroID : -1;
    if (getHeroID() == -1) {
        signed char powerRank;
        *pIStream >> powerRank;
        _m_powerRank = powerRank;
    }
}

VA(0x0044aea1, 0x4)
THeroID THeroPlaceholder::getHeroID() const
{
    return _m_heroID;
}

void THeroPlaceholder::setPowerRank(unsigned int newPowerRank)
{
    _m_powerRank = newPowerRank;
}

VA(0x0044aea5, 0x1f)
bool THeroPlaceholder::isCustomized() const
{
    return getHeroID() != -1 || _m_powerRank != 1;
}

VA(0x0044aec4, 0x4d)
void THeroPlaceholder::write(TRawOStream* pOStream, int version) const
{
    TPlayableObject::write(pOStream, version);
    *pOStream << static_cast<unsigned char>(_m_heroID);
    if (getHeroID() == -1)
        *pOStream << static_cast<signed char>(_m_powerRank);
}

VA(0x0044af11, 0x2b)
void THero::initialize()
{
    initializeTraitsTable();
    InitializePrimarySkillTraitsTable();
    for (unsigned int skill = 0; skill < kNumSecSkills; skill++)
        aHeroSecondarySkillTraitsImp[skill].m_name = akSSkillTraits[skill].m_name;
    InitializeSkillMasteryTraitsTable();
}

VA(0x0044af51, 0x149)
THero::THero(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TLinkableObject(objType), TBasicHero(objType, owner),
      _m_bCustomName(false), _m_bCustomPortrait(false), _m_bCustomSecondarySkills(false),
      _m_bCustomArmy(false), _m_bCustomArtifacts(false), _m_bCustomBiography(false),
      _m_bCustomSpells(false), _m_bCustomPrimarySkills(false), _m_bCustomExperience(false),
      _m_portrait(0), _m_aPrimarySkill(0), _m_experience(0), _m_bGroupedFormation(false),
      _m_patrolRadius(-1), _m_sex(-1)
{
}

VA(0x0044b09a, 0x470)
void THero::read(TRawIStream* pIStream, int version)
{
    if (version >= 15)
        TLinkableObject::read(pIStream, version);
    TPlayableObject::read(pIStream, version);
    unsigned char heroID;
    *pIStream >> heroID;
    setStoredHeroID(heroID);
    signed char flag;
    *pIStream >> flag;
    if (_m_bCustomName = flag != 0) {
        std::string name;
        *pIStream >> name;
        _m_name = name;
    }
    if (version >= 23) {
        *pIStream >> flag;
        if (_m_bCustomExperience = flag != 0) {
            long experience;
            *pIStream >> experience;
            _m_experience = experience;
        } else if (version == 23) {
            long experience;
            *pIStream >> experience;
        }
    } else {
        long experience;
        *pIStream >> experience;
        if (_m_bCustomExperience = experience != 0)
            _m_experience = experience;
    }
    *pIStream >> flag;
    if (_m_bCustomPortrait = flag != 0) {
        unsigned char portrait;
        *pIStream >> portrait;
        if (version < 16) {
            if (portrait == 0x80)
                portrait = 0x92;
            else if (portrait == 0x81)
                portrait = 0x9c;
        }
        _m_portrait = portrait;
    }
    *pIStream >> flag;
    if (_m_bCustomSecondarySkills = flag != 0) {
        THeroPrototype::TSecondarySkills secondarySkills;
        secondarySkills.read(pIStream, version);
        _m_secondarySkills = secondarySkills;
    }
    *pIStream >> flag;
    if (_m_bCustomArmy = flag != 0)
        _m_army = TArmy(pIStream, version);
    signed char bGroupedFormation;
    *pIStream >> bGroupedFormation;
    _m_bGroupedFormation = bGroupedFormation != 0;
    *pIStream >> flag;
    if (_m_bCustomArtifacts = flag != 0)
        _m_artifacts = THeroPrototype::TArtifactContainer(pIStream, version);
    signed char patrolRadius;
    *pIStream >> patrolRadius;
    _m_patrolRadius = patrolRadius;
    if (version >= 18) {
        *pIStream >> flag;
        if (_m_bCustomBiography = flag != 0) {
            std::string biography;
            *pIStream >> biography;
            _m_biography = biography;
        }
        signed char sex;
        *pIStream >> sex;
        _m_sex = sex;
        if (version >= 22) {
            *pIStream >> flag;
            if (_m_bCustomSpells = flag != 0) {
                std::bitset<kNumSpells> spells;
                readBitset(*pIStream, spells);
                _m_spells = spells;
            }
        } else {
            signed char spell;
            *pIStream >> spell;
            int startingSpell = spell;
            if (_m_bCustomSpells = startingSpell != -2) {
                std::bitset<kNumSpells> spells;
                if (startingSpell != SPELL_NONE)
                    spells.set(startingSpell);
                _m_spells = spells;
            }
        }
    }
    if (version >= 22) {
        *pIStream >> flag;
        if (_m_bCustomPrimarySkills = flag != 0)
            _m_aPrimarySkill = THeroPrototype::TPrimarySkills(pIStream, version);
    }
    signed char aReserved[kNumHeroReserved];
    *pIStream >> aReserved;
}

VA(0x0044b50a, 0x17)
void THero::setName(const std::string& newName)
{
    _m_name = newName;
}

VA(0x0044b521, 0xa)
void THero::setPortrait(int newPortrait)
{
    _m_portrait = newPortrait;
}

VA(0x0044b52b, 0xf)
void THero::setSecondarySkills(const THeroPrototype::TSecondarySkills& newSecondarySkills)
{
    _m_secondarySkills = newSecondarySkills;
}

VA(0x0044b53a, 0x2c)
void THero::setArmy(const TArmy& newArmy)
{
    _m_army = newArmy;
}

VA(0x0044b566, 0xf)
void THero::setArtifacts(const THeroPrototype::TArtifactContainer& newArtifacts)
{
    _m_artifacts = newArtifacts;
}

VA(0x0044b575, 0x1a)
void THero::setBiography(const std::string& newBiography)
{
    _m_biography = newBiography;
}

VA(0x0044b58f, 0x14)
void THero::setSpells(const std::bitset<kNumSpells>& newSpells)
{
    _m_spells = newSpells;
}

VA(0x0044b5a3, 0x28)
void THero::setPrimarySkills(const TArray<int, kNumPrimarySkills>& newPrimarySkills)
{
    _m_aPrimarySkill = newPrimarySkills;
}

VA(0x0044b5cb, 0xd)
void THero::setExperience(int newExperience)
{
    _m_experience = newExperience;
}

VA(0x0044b5d8, 0xd)
void THero::setSex(int newSex)
{
    _m_sex = newSex;
}

VA(0x0044b5e5, 0x321)
void THero::importText(std::istream* pIStream, EGameVersion version)
{
    std::string line;
    if (_m_bCustomName) {
        getline(*pIStream, line);
        if (line != std::string(kNameStr) + ':')
            throw TImportTextFailure();
        getline(*pIStream, line);
        if (line.size() > THeroPrototype::s_kMaxNameLen)
            line.erase(THeroPrototype::s_kMaxNameLen);
        replace(line.begin(), line.end(), '\t', ' ');
        if (_m_bCustomName && find_if(line.begin(), line.end(), not1(ptr_fun(isspace))) == line.end())
            throw TImportTextFailure();
        _m_name = line;
    }
    if (version >= GAME_VERSION_AB && _m_bCustomBiography) {
        getline(*pIStream, line);
        if (line != std::string(kBiographyStr) + ':')
            throw TImportTextFailure();
        getline(*pIStream, line);
        replace(line.begin(), line.end(), '\t', '\n');
        _m_biography = line;
    }
}

VA(0x0044b91e, 0x59)
bool THero::hasArtifact(TArtifact artifact) const
{
    if (!_m_bCustomArtifacts)
        return false;
    const THeroPrototype::TArtifactContainer& artifacts = getArtifacts();
    for (unsigned int slot = 0; slot < kNumArtifactSlots + 1; slot++)
        if (artifacts.getSlot(TArtifactSlot(slot)) == artifact)
            return true;
    const std::multiset<TArtifact>& backpack = artifacts.getBackpack();
    return find(backpack.begin(), backpack.end(), artifact) != backpack.end();
}

VA(0x0044b977, 0x2a)
bool THero::isCustomized() const
{
    return _m_bCustomName || _m_bCustomPortrait || _m_bCustomSecondarySkills || _m_bCustomArmy
           || _m_bCustomArtifacts || _m_bCustomBiography || _m_bCustomSpells || _m_bCustomPrimarySkills
           || _m_bCustomExperience || _m_bGroupedFormation || _m_patrolRadius != -1 || _m_sex != -1;
}

VA(0x0044b9a1, 0x311)
void THero::write(TRawOStream* pOStream, int version) const
{
    if (version >= GAME_VERSION_AB)
        TLinkableObject::write(pOStream, version);
    TPlayableObject::write(pOStream, version);
    *pOStream << getStoredHeroID();
    *pOStream << static_cast<signed char>(_m_bCustomName);
    if (_m_bCustomName)
        *pOStream << _m_name;
    if (version >= GAME_VERSION_SOD) {
        *pOStream << static_cast<signed char>(_m_bCustomExperience);
        if (_m_bCustomExperience)
            *pOStream << static_cast<long>(_m_experience);
    } else {
        int experience = _m_bCustomExperience ? _m_experience : 0;
        *pOStream << experience;
    }
    *pOStream << static_cast<signed char>(_m_bCustomPortrait);
    if (_m_bCustomPortrait) {
        int portrait = _m_portrait;
        if (version == GAME_VERSION_ROE) {
            if (portrait == 0x92)
                portrait = 0x80;
            else if (portrait == 0x9c)
                portrait = 0x81;
        }
        *pOStream << static_cast<unsigned char>(portrait);
    }
    *pOStream << static_cast<signed char>(_m_bCustomSecondarySkills);
    if (_m_bCustomSecondarySkills)
        _m_secondarySkills.write(pOStream, version);
    *pOStream << static_cast<signed char>(_m_bCustomArmy);
    if (_m_bCustomArmy)
        _m_army.write(pOStream, version);
    *pOStream << static_cast<signed char>(_m_bGroupedFormation);
    *pOStream << static_cast<signed char>(_m_bCustomArtifacts);
    if (_m_bCustomArtifacts)
        _m_artifacts.write(pOStream, version);
    *pOStream << static_cast<signed char>(_m_patrolRadius);
    if (version >= GAME_VERSION_AB) {
        *pOStream << static_cast<signed char>(_m_bCustomBiography);
        if (_m_bCustomBiography)
            *pOStream << _m_biography;
        *pOStream << static_cast<signed char>(_m_sex);
        if (version >= GAME_VERSION_SOD) {
            *pOStream << static_cast<signed char>(_m_bCustomSpells);
            if (_m_bCustomSpells)
                writeBitset(*pOStream, _m_spells);
        } else if (_m_bCustomSpells) {
            if (_m_spells.none()) {
                *pOStream << static_cast<signed char>(SPELL_NONE);
            } else {
                unsigned int spell = 0;
                while (!_m_spells.test(spell))
                    spell++;
                *pOStream << static_cast<signed char>(spell);
            }
        } else {
            *pOStream << static_cast<signed char>(-2);
        }
    }
    if (version >= GAME_VERSION_SOD) {
        *pOStream << static_cast<signed char>(_m_bCustomPrimarySkills);
        if (_m_bCustomPrimarySkills)
            for (unsigned int i = 0; i < kNumPrimarySkills; i++)
                *pOStream << static_cast<unsigned char>(_m_aPrimarySkill[i]);
    }
    signed char aReserved[kNumHeroReserved];
    fill_n(aReserved, sizeof(aReserved), 0);
    *pOStream << aReserved;
}

VA(0x0044bcb2, 0x136)
void THero::exportText(std::ostream* pOStream, EGameVersion version) const
{
    if (_m_bCustomName)
        *pOStream << kNameStr << ':' << '\n' << _m_name << '\n';
    if (version >= GAME_VERSION_AB && _m_bCustomBiography) {
        std::string biography = _m_biography;
        replace(biography.begin(), biography.end(), '\n', '\t');
        *pOStream << kBiographyStr << ':' << '\n' << biography << '\n';
    }
}

VA(0x0044bde8, 0x8d)
TRandomHero::TRandomHero(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), THero(objType, owner)
{
}

VA(0x0044be75, 0x9d)
TRandomHero::TRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), THero(objType, ePlayerNone)
{
    read(pIStream, version);
}

VA(0x0044bf12, 0x3)
unsigned char TRandomHero::getStoredHeroID() const
{
    return 0xff;
}

VA(0x0044bf15, 0x93)
TIdentifiedHero::TIdentifiedHero(const TObjectType& objType)
    : TGameObject(objType), THero(objType, ePlayerNone), _m_heroID(-1)
{
}

VA(0x0044bfa8, 0x96)
TIdentifiedHero::TIdentifiedHero(const TObjectType& objType, TPlayer owner, THeroID heroID)
    : TGameObject(objType), THero(objType, owner), _m_heroID(heroID)
{
}

VA(0x0044c03e, 0x7)
THeroID TIdentifiedHero::getHeroID() const
{
    return _m_heroID;
}

VA(0x0044c045, 0xe)
void TIdentifiedHero::setStoredHeroID(unsigned char heroID)
{
    setHeroID(heroID);
}

VA(0x0044c053, 0x7)
unsigned char TIdentifiedHero::getStoredHeroID() const
{
    return _m_heroID;
}

VA(0x0044c05a, 0xb8)
TNonRandomHero::TNonRandomHero(const TObjectType& objType, TPlayer owner, THeroID heroID)
    : TGameObject(objType), TIdentifiedHero(objType, owner, heroID)
{
    setPortrait(*s_akClassTraits[getHeroClass()].m_heroes.begin());
}

VA(0x0044c112, 0xc9)
TNonRandomHero::TNonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TIdentifiedHero(objType)
{
    read(pIStream, version);
    if (!getBCustomPortrait())
        setPortrait(*s_akClassTraits[getHeroClass()].m_heroes.begin());
}

VA(0x0044c1db, 0xd)
void TIdentifiedHero::setHeroID(THeroID newHeroID)
{
    _m_heroID = newHeroID;
}

VA(0x0044c1e8, 0x50)
std::string TNonRandomHero::getTypeName() const
{
    return s_akClassTraits[getHeroClass()].m_name;
}

VA(0x0044c238, 0x8f)
TPrison::TPrison(const TObjectType& objType, THeroID heroID)
    : TGameObject(objType), TIdentifiedHero(objType, ePlayerNone, heroID)
{
}

VA(0x0044c2c7, 0x9b)
TPrison::TPrison(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TIdentifiedHero(objType)
{
    read(pIStream, version);
}
