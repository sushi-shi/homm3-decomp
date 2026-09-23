// Shared Mac declaration view for the admitted hero compilation group.
// Game bodies and initialized tables come from src/hero.cpp. Grow this view
// with reviewed Mac ABI evidence; do not put reconstructed bodies here.
#ifndef HOMM3_MAC_HERO_H
#define HOMM3_MAC_HERO_H

#include "va.h"
#include "spellschool.h"
#include "magicterrain.h"
#include "message_record.h"
typedef unsigned long size_t;

extern "C" int abs(int);
extern "C" int sprintf(char*, const char*, ...);
extern "C" char* strncpy(char*, const char*, unsigned long);
extern "C" unsigned long strlen(const char*);
extern "C" void* memset(void*, int, unsigned long);
class heroWindow {
public:
    int broadcastMessage(message& msg);
    int widgetSetStatus(int id, int status);
    int widgetClearStatus(int id, int status);
};
class CHeroWindowEx : public heroWindow {};
class CAdvPopup : public CHeroWindowEx {};
class THeroScreenWindow : public CAdvPopup {};
class widget {
public:
    enum {
        WIDGET_SET_TEXT = 3,
        WIDGET_SET_ICON_FRAME = 4,
        WIDGET_SET_STATUS = 5,
        WIDGET_CLEAR_STATUS = 6,
        WIDGET_DRAWN = 4
    };
};
enum { MESSAGE_WIDGET = 0x200, CREATURE_NONE = -1 };
extern THeroScreenWindow* g_heroScreenWindow;
extern int g_heroScreenArmyStripLive;
extern int g_heroScreenArmySlot;
extern char g_text[];
extern const char g_emptyRolloverText[];
enum TSkillMastery { eMasteryInvalid = -1, eMasteryNone = 0, eMasteryBasic = 1, eMasteryAdvanced = 2, eMasteryExpert = 3 };
enum { HOLY_GRAIL_ID = 26, TOWN_CASTLE = 0, TOWN_RAMPART = 1 };
enum TSecondarySkill {
    eSecSkillNone = -1,
    eSecSkillLeadership = 6,
    eSecSkillLuck = 9,
    eSecSkillWisdom = 7,
    eSecSkillSchoolOfFireMagic = 14,
    eSecSkillSchoolOfAirMagic = 15,
    eSecSkillSchoolOfWaterMagic = 16,
    eSecSkillSchoolOfEarthMagic = 17,
    kNumSecSkills = 28
};
enum THeroClass {
    classCleric = 1, classDruid = 3, classWizard = 5, classHeretic = 7,
    classNecromancer = 9, classWarlock = 11, classBattleMage = 13,
    classWitch = 15
};
// Mac getSkillAward indexes 0x40-byte class rows and byte chances at +0x18.
struct THeroClassTraits {
    int m_townType;
    const char* m_className;
    float m_aggression;
    signed char m_initialPrimarySkill[4];
    signed char m_gainPrimarySkillChance[4];
    signed char m_gainPrimarySkillChance10P[4];
    signed char m_gainSecondarySkillChance[28];
    char pad1[0x40 - 0x18 - 28];
};
extern const THeroClassTraits (&g_heroClasses)[18];
struct THeroTraits {
    int m_sex;
    int m_race;
    int m_heroClass;
    int m_firstSkill;
    int m_firstSkillLevel;
    int m_secondSkill;
    int m_secondSkillLevel;
    unsigned char m_startsWithSpellbook;
    char pad0[3];
    int m_startingSpell;
    char pad1[0x40 - 0x24];
    const char* m_defaultName;
    char pad2[0x5c - 0x44];
};
extern const THeroTraits (&g_heroTraits)[156];
extern bool g_inCampaign;
extern char g_campaignDisabledSkills[kNumSecSkills];
int random(int minimum, int maximum);
enum { NUM_SPELLS = 70 };
enum {
    g_startLevelCampaign = 8,
    g_startLevelScenario = 3,
    g_startLevelHeroId = 151,
    g_startLevelBonus = 5
};
enum TArtifact { ARTIFACT_NONE = -1, ARTIFACT_SPELLBOOK = 0,
                 ARTIFACT_SPELL_SCROLL = 1, ARTIFACT_CATAPULT = 3 };
enum SpellID { MAC_SPELL_PLACEHOLDER = 0 };
enum TArtifactSlot { MAC_ARTIFACT_SLOT_PLACEHOLDER = 0 };
struct type_artifact {
    TArtifact m_artifactId;
    int m_extra;
#include "type_artifact_constructors.inl"
};
namespace std {
template <class T> class allocator {
public:
    // Pinned MSL memory: explicit empty allocator<T> default constructor.
    allocator() {}
};
// Exact std::fill template from the pinned CodeWarrior Pro 6 MSL
// MSL_-_C++_-_Common_-_Libs/algorithm (the installed header's fill body).
template <class ForwardIterator, class T>
inline void fill(ForwardIterator first, ForwardIterator last, const T& value)
{
    for (; first != last; ++first)
        *first = value;
}
template <class OutputIterator, class Size, class T>
inline void fill_n(OutputIterator first, Size n, const T& value)
{
    for (; n > 0; ++first, --n)
        *first = value;
}
class string {
    struct rep {
        unsigned long m_length;
        char m_beforeData[8];
        char* m_data;
    }* m_handle;
public:
    static const unsigned long npos = ~0UL;
    // Pinned MSL basic_string default constructor takes a default-created
    // allocator by const reference; the Mac caller passes its stack address.
    explicit string(const allocator<char>& a = allocator<char>());
    string(const string&);
    ~string();
    // Pinned MSL string inlines operator=(s) -> assign(s) ->
    // assign(s, traits::length(s)). char_traits<char>::length calls strlen.
    string& operator=(const char* s) { return assign(s); }
    string& assign(const char* s) { return assign(s, strlen(s)); }
    string& assign(const char*, unsigned long);
    string& assign(const string&, unsigned long, unsigned long);
    string& append(const char*, unsigned long);
    string& append(const string&, unsigned long, unsigned long);
    // Pinned MSL string: operator+= forwards through these append overloads.
    string& append(const char* s) { return append(s, strlen(s)); }
    string& append(const string& s) { return append(s, 0, npos); }
    string& operator+=(const char* s) { return append(s); }
    string& operator+=(const string& s) { return append(s); }
    const char* c_str() const { return m_handle->m_data; }
};
// Pinned CodeWarrior MSL vector has distinct POD and non-POD paths. The
// non-POD town indexer exposes its canonical *(data() + n) body here; the
// POD char* indexer retains the independently observed 0x948 library call.
template <class T> class vector {
    char m_capacityAndSize[8];
    T* m_first;
public:
    // Pinned MSL vector: __vector_imp's indexer goes through data(), whose
    // result is a reference to the stored pointer, not a pointer value.
    T*& data() { return m_first; }
    T* const& data() const { return m_first; }
    T& operator[](size_t n) { return *(data() + n); }
    const T& operator[](size_t n) const { return *(data() + n); }
};
template <> class vector<char*> {
    char m_capacityAndSize[8];
    char** m_first;
public:
    char*& operator[](size_t n);
    char* const& operator[](size_t n) const;
};
template <unsigned long Count> class bitset {
    unsigned long m_words[(Count + 31) / 32];
public:
    bool test(unsigned long index) const;
    bitset& set(unsigned long index, bool value = true);
    bitset& reset();
    bitset& reset(unsigned long index) { return set(index, false); }
    bool any() const;
    bool none() const;
    class reference {
        bitset* m_owner;
        unsigned long m_index;
    public:
        reference(bitset* owner, unsigned long index)
            : m_owner(owner), m_index(index) {}
        operator bool() const { return m_owner->test(m_index); }
        reference& operator=(bool value) {
            m_owner->set(m_index, value);
            return *this;
        }
    };
    reference operator[](unsigned long index) {
        return reference(this, index);
    }
};
}
// removeArtifact and equipArtifact index Mac 0x20-byte artifact traits at
// +0x08 (slot class), +0x14 (combination index), and +0x1d (spell flag).
struct TArtifactTraits {
    const char* m_name;
    int m_cost;
    int m_allowableSlotMask;
    int m_artifactClass;
    const char* m_description;
    int m_comboType;
    int m_targetCombo;
    unsigned char m_disabled;
    unsigned char m_givesSpells;
    char m_paddingAfterGivesSpells[2];
};
struct TCombinationArtifact {
    int m_artifactId;
    std::bitset<144> m_components;
};
extern const TArtifactTraits (&g_artifactTraits)[144];
extern const TCombinationArtifact* g_combinationArtifacts;
extern const signed char g_artifactPrimarySkillBonuses[][4];
class TTextResource {
public:
    char m_beforeText[0x1c];
    std::vector<char*> m_text;
#include "inline/textresource_get_text.inl"
#include "inline/textresource_index.inl"
};
extern const TTextResource* g_generalText;
extern const char* g_moraleTexts[42];
extern const char* g_luckTexts[25];
extern unsigned long long g_bitNumber[];
const char* getBuildingName(int townType, int buildingId);
std::string formatString(const char* format, ...);
void normalDialog(const char* text, int mbType, int x, int y,
                  int resType1, int resExtra1, int resType2, int resExtra2,
                  int special, int timeout, int resType3, int resExtra3);
void checkEndGame(int forceWin);
enum { DIALOG_RETURN_ACCEPT = 0x7805 };
class heroWindowManager {
public:
    char m_beforeDialogReturn[0x38];
    int m_dialogReturn;
};
extern heroWindowManager* g_windowManager;
// The Mac siege-weapon switch tests 0x91..0x94 and maps the last three
// creatures to artifact IDs 4, 6 and 5.
enum {
    CREATURE_CATAPULT = 0x91,
    CREATURE_BALLISTA = 0x92,
    CREATURE_FIRST_AID_TENT = 0x93,
    CREATURE_AMMO_CART = 0x94,
    ARTIFACT_BALLISTA = 4,
    ARTIFACT_AMMO_CART = 5,
    ARTIFACT_FIRST_AID_TENT = 6
};
struct mac_equipped_artifact {
    int m_artifactId;
    char pad[4];
};
struct mac_army_group {
    enum { ARMY_GROUP_SLOT_COUNT = 7 };
    int m_armies[7];
    int m_numTroops[7];
};
typedef mac_army_group armyGroup;
struct type_point {
    short m_x : 10;
    short m_y : 10;
    short m_z : 4;
};
class HeroExtra;
#pragma options align=packed
class type_obscuring_object {
public:
    short m_x;
    short m_y;
    short m_z;
    char m_obscuringPrefix[0x18 - 6];
    void initialize();
};
class hero : public type_obscuring_object {
public:
    short m_mana;
    int m_id;
    int m_order;
    signed char m_owner;
    char m_name[13];
    int m_heroClass;
    unsigned char m_portrait;
    int m_pathTargetX;
    int m_pathTargetY;
    short m_pathTargetZ;
    short m_lastMagicSchoolLevel;
    short m_targetDistance;
    unsigned char m_targetIsCritical;
    unsigned char m_patrolX;
    unsigned char m_patrolY;
    signed char m_patrolRadius;
    unsigned char m_facing;
    unsigned char m_formation;
    int m_maxMovePoints;
    int m_movePoints;
    int m_experience;
    short m_level;
    unsigned long m_trainingGroundsFlags;
    unsigned long m_defenseTowerFlags;
    unsigned long m_gardenOfRevelationFlags;
    unsigned long m_mercCampFlags;
    unsigned long m_powerSchoolFlags;
    unsigned long m_treeOfKnowledgeFlags;
    unsigned long m_libraryFlags;
    unsigned long m_arenaFlags;
    unsigned long m_magicSchoolFlags;
    unsigned long m_warSchoolFlags;
    unsigned long m_universityFlags;
    unsigned long m_shrine1Flags;
    unsigned long m_shrine2Flags;
    unsigned long m_shrine3Flags;
    unsigned char m_levelSeed;
    unsigned char m_lastWisdom;
    // Mac updateArmies indexes IDs at this+0x91 and troop counts at +0xad.
    mac_army_group m_army;
    signed char m_skillLevel[28];
    unsigned char m_skillOrder[28];
    int m_skillCount;
    unsigned int m_flags;
    float m_turnExperienceToRvRatio;
    signed char m_dWalkSpellsCast;
    int m_disguiseLevel;
    int m_flightLevel;
    int m_waterWalkLevel;
    signed char m_moraleBonus;
    signed char m_luckBonus;
    unsigned char m_isSleeping;
    int m_bounty;
    std::bitset<48> m_townSpecialGrantedMask;
    int m_visionsPower;
    // Retail Mac uses a stride of eight and reads each ID at this+0x12d.
    type_artifact m_equipped[19];
    unsigned char m_artifactSlotCounts[15];
    type_artifact m_backpack[64];
    signed char m_backpackCount;
    int m_sex;
    unsigned char m_hasCustomName;
    std::string m_customName;
    unsigned char m_inSpellbook[70];
    unsigned char m_availableSpells[70];
    signed char m_stats[4];
    float m_aggression;
    char pad10[0x486 - 0x472];
    enum { kPatrolNone = 0xff, kFacingE = 2 };
    enum { LEVEL_UP_CAMPAIGN_OVERRIDE = 14, LEVEL_UP_OVERRIDE_HERO_ID = 45 };

    static int getExperience(int level);
    static int getExperienceIncrement(int level);
    void updateArmies();
    void destroySiegeWeaponArtifact(int creatureType);
    void removeArtifact(long slot);
#include "inline/hero_adjust_primary_skill.inl"
    void updateSpellList();
    void initialize(short index);
    void initialize(const HeroExtra* setup);
    int giveSS(int whichSS, int numLevelsToGive);
#include "inline/hero_set_primary_skill.inl"
    void addSpell(int whichSpell);
#include "inline/hero_get_artifact.inl"
    unsigned char equipArtifact(const type_artifact* artifact, long slot);
    unsigned char addToBackpack(const type_artifact* artifact, long slot);
    unsigned char heroFn004DBE80(int combination);
    unsigned char heroFn004DBF30(int combination, long slot);
    unsigned char giveArtifact(const type_artifact* artifact,
                               unsigned char announce, unsigned char checkEnd);
    int giveExperience(int howMuch, int checkForLevelUp, int showCapWindow);
    static int getLevel(int experience);
    void checkLevel();
    std::string getMoraleDescription() const;
    std::string getLuckDescription() const;
    unsigned char isWieldingArtifact(int whichArtifact) const;
    int getMorale(const hero* otherHero, unsigned char onCursedGround,
                  unsigned char ignoreNullifyMorale) const;
    int getLuck(const hero* otherHero, unsigned char onCursedGround,
                unsigned char ignoreNullifyLuck) const;
#include "inline/hero_get_primary_skill.inl"
    float getIntelligenceFactor() const;
#include "inline/hero_get_max_mana.inl"
    int getMobility(unsigned char seaMovement) const;
    int getMobility() const;
    bool isLevelUpCampaignOverride() const;
    TSkillMastery getSpellSchoolLevel(TSpellSchool schoolMask, int magicTerrain) const;
    TSpellSchool getHighestSchool(TSpellSchool schoolMask) const;
    unsigned char isInPatrolRadius(type_point point) const;
};
// The Mac setup record shares its leading 0x307 bytes with the retail
// HeroExtra. Its four-byte string shifts the later fields back by 0x0c.
class HeroExtra {
public:
    signed char m_owner;
    char pad0[3];
    int m_id;
    int m_objRef;
    unsigned char m_hasCustomName;
    char m_nameBuffer[13];
    unsigned char m_customExperience;
    char pad1;
    int m_experience;
    unsigned char m_customPortraitNumber;
    unsigned char m_portraitNumber;
    unsigned char m_customSecondarySkills;
    char pad2;
    int m_numSecondarySkills;
    char m_secondarySkill[8];
    char m_secondarySkillLevel[8];
    unsigned char m_customArmies;
    char pad3[3];
    int m_armies[7];
    short m_numTroops[7];
    unsigned char m_groupFormation;
    unsigned char m_customArtifacts;
    type_artifact m_artifacts[19];
    type_artifact m_backpack[64];
    unsigned char m_numInBackpack;
    type_point m_location;
    signed char m_patrolRadius;
    unsigned char m_customName;
    char pad4;
    std::string m_name;
    int m_sex;
    unsigned char m_customSpells;
    char pad5[3];
    std::bitset<70> m_spells;
    unsigned char m_customPrimarySkills;
    signed char m_primarySkills[4];
};
struct mac_campaign {
    char pad0[0x1ed06];
    unsigned char m_currentMap;
    char pad1;
    int m_currentCampaign;
};
struct playerData {
    char m_beforeNumTowns[0x3e];
    signed char m_numTowns;
    char m_beforeTownIds;
    signed char m_townIds[0x48];
    char m_beforeIsHuman[0xde - 0x88];
    unsigned char m_isHuman;
    char m_beforeAssembled[0xe4 - 0xdf];
    std::bitset<12> m_assembledCombinations;
    char m_afterAssembled[0x15c - 0xe8];
};
// Mac getMoraleDescription reads the town type at +4 and two active-mask
// words at +0x14c/+0x150, with 0x15c bytes between vector entries.
class town {
public:
    char m_beforeType[4];
    signed char m_type;
    char m_beforeBuilt[0x144 - 5];
    unsigned long long m_built;
    unsigned long long m_active;
    char m_afterActive[0x15c - 0x154];
#include "inline/town_has_building.inl"
};
struct VictoryConditionStruct {
    unsigned char checkForArtifactWin();
};
struct MacMapHeader {
    char m_beforeMaxHeroLevel[0xb];
    unsigned char m_maxHeroLevel;
    char m_beforeVictory[0x2c - 0xc];
    VictoryConditionStruct m_victoryCondition;
};
class game {
public:
    mac_campaign m_campaign;
    char m_beforeVersion[0x1ef24 - 0x1ed0c];
    int m_f1f698;
    char m_beforeMapHeader[0x1f0f8 - 0x1ef28];
    MacMapHeader m_mapHeader;
    char m_beforePlayers[0x1ff48 - 0x1f0f8 - sizeof(MacMapHeader)];
    playerData m_players[8];
    std::vector<town> m_towns;
    hero m_heroes[156];
    char pad1[0x4d2fc - (0x20a34 + 156 * 0x486)];
    char m_ssDisabled[28];
    bool isLocalHuman(int gamePos) const;
    int getLocalPlayerGamePos() const;
#include "inline/game_get_town.inl"
};
extern game* g_game;
#pragma options align=reset
#endif
