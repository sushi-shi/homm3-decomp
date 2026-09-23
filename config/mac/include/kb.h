// Mac declaration view for the KB compilation group. Game bodies remain in
// src/kb.cpp; these offsets come from the pinned Mac getNextHumanPlayer body.
#ifndef HOMM3_MAC_KB_H
#define HOMM3_MAC_KB_H

#include "va.h"

typedef long long __int64;
enum type_building_id {
    MAGE_GUILD5_ID = 4,
    DOCK_ID = 6,
    CASTLE_FORT_ID = 7,
    CASTLE_CITADEL_ID = 8,
    CASTLE_CASTLE_ID = 9,
    HALL_CAPITOL_ID = 13,
    BLACKSMITH_ID = 16,
    DOCK_WITH_BOAT_ID = 20,
    EXTRA_0_ID = 21,
    EXTRA_1_ID = 22,
    HOLY_GRAIL_ID = 26,
    DWELLING_2_ID = 32
};
enum TTownType { TOWN_CASTLE = 0, TOWN_TOWER = 2 };

extern "C" char* strcpy(char* destination, const char* source);
extern "C" unsigned long strlen(const char* text);
extern "C" int sprintf(char* destination, const char* format, ...);
// CodeWarrior MSL cctype uses its library-owned Mac Roman table and preserves
// EOF. See ../macos/cw/all/MSL_-_C_-_Common_-_Libs/cctype.
extern "C" unsigned char __upper_map[];
inline int toupper(int value)
{
    return value == -1 ? -1 : static_cast<int>(__upper_map[static_cast<unsigned char>(value)]);
}

// Canonical type_dialog_resource is two words. The Mac extended-dialog loop
// reads vector size at +4 and its eight-byte elements through first at +8.
struct type_dialog_resource {
    int m_resource;
    unsigned long m_qualifier;
};

namespace std {
class string {
    struct rep {
        unsigned long m_length;
        char m_beforeData[8];
        char* m_data;
    }* m_handle;
public:
    static const unsigned long npos = ~0UL;
    string();
    string(const string&);
    ~string();
    string& assign(const string&, unsigned long, unsigned long);
    string& operator=(const string& value) { return assign(value, 0, npos); }
    string& assign(const char*, unsigned long);
    string& operator=(const char* value) { return assign(value, strlen(value)); }
    string& append(const char*, unsigned long);
    string& operator+=(const char* value) { return append(value, strlen(value)); }
    string& append(const string&, unsigned long, unsigned long);
    string& operator+=(const string& value) { return append(value, 0, npos); }
    const char* c_str() const { return m_handle->m_data; }
    unsigned long length() const { return m_handle->m_length; }
};
template <class T> class vector {
    char m_beforeFirst[8];
    T* m_first;
public:
    T& operator[](int index) const;
};
template <> class vector<type_dialog_resource> {
    char m_beforeSize[4];
    unsigned long m_size;
    type_dialog_resource* m_first;
public:
    unsigned long size() const { return m_size; }
    type_dialog_resource& operator[](int index) const { return m_first[index]; }
    void clear();
};
template <unsigned long Count> class bitset {
    unsigned long m_words[(Count + 31) / 32];
public:
    bitset() { reset(); }
    bitset& reset();
    bool test(unsigned long position) const;
    bitset& set(unsigned long position, bool value = true);
    class reference {
        bitset* m_owner;
        unsigned long m_position;
    public:
        reference(bitset* owner, unsigned long position)
            : m_owner(owner), m_position(position) {}
        operator bool() const { return m_owner->test(m_position); }
        reference& operator=(bool value) {
            m_owner->set(m_position, value);
            return *this;
        }
    };
    reference operator[](unsigned long position) {
        return reference(this, position);
    }
    bool operator[](unsigned long position) const { return test(position); }
};
}

struct SGameSetupOptions {
    char m_padToDifficulty[56];
    signed char m_difficulty;  // Mac game+0x1ef64, immediately before filename.
    char m_filename[251];
    char m_path[100];
    char m_remaining[0x1cc - 57 - 251 - 100];
};

// Mac loss coordinates are bytes, as in the Dreamcast record; the paired
// loss dispatcher loads them from +1/+2/+3. The hero id and loss flags are
// independently located at +0x1c and +0x22/+0x23.
struct LossConditionStruct {
    signed char m_type;
    unsigned char m_townX;
    unsigned char m_townY;
    unsigned char m_townZ;
    char m_beforeHeroId[0x1c - 4];
    int m_heroId;
    char m_beforeLossFlags[0x22 - 0x20];
    unsigned char m_gameLost;
    signed char m_playerLoser;
#include "inline/loss_condition_ctor.inl"
};
enum {
    LOSS_CONDITION_LOSE_TOWN = 0,
    LOSS_CONDITION_LOSE_HERO = 1,
    LOSS_CONDITION_TIME_LIMIT = 2
};

// Mac DisplayVCWinLoss reads type at +0, artifact at +4, the winner flags
// at +0x48/+0x49, and copies the complete 0x4c-byte condition into its
// network record. The intervening offsets match the canonical map record.
struct VictoryConditionStruct {
    signed char m_type;
    char m_beforeArtifact[3];
    int m_artifactNum;
    int m_creatureType;
    int m_numCreatures;
    int m_resourceType;
    int m_resourceAmount;
    int m_townX;
    int m_townY;
    int m_townZ;
    char m_beforeHeroId[0x34 - 0x20];
    int m_heroId;
    char m_beforeWinFlags[0x48 - 0x38];
    unsigned char m_gameWon;
    signed char m_playerWinner;
    char m_afterWinner[2];
    unsigned char checkForUpgradedTown();
};

enum {
    VICTORY_CONDITION_NONE = -1,
    VICTORY_CONDITION_ARTIFACT = 0,
    VICTORY_CONDITION_TOTAL_CREATURES = 1,
    VICTORY_CONDITION_TOTAL_RESOURCES = 2,
    VICTORY_CONDITION_UPGRADE_TOWN = 3,
    VICTORY_CONDITION_BUILD_GRAIL = 4,
    VICTORY_CONDITION_DEFEAT_HERO = 5,
    VICTORY_CONDITION_CAPTURE_TOWN = 6,
    VICTORY_CONDITION_DEFEAT_MONSTER = 7,
    VICTORY_CONDITION_FLAG_ALL_GENERATORS = 8,
    VICTORY_CONDITION_FLAG_ALL_MINES = 9,
    VICTORY_CONDITION_TRANSPORT_ARTIFACT = 10,
    VICTORY_CONDITION_DEFEAT_ALL_MONSTERS = 11,
    VICTORY_CONDITION_SURVIVE_TIME = 12
};
enum {
    GAME_CAMPAIGN_7 = 7,
    GAME_CAMPAIGN_18 = 18,
    GAME_SCENARIO_1 = 1,
    GAME_SCENARIO_8 = 8,
    GAME_SCENARIO_9 = 9
};

enum eRS_Messages { RS_PLAYER_WON = 1019, RS_PLAYER_LOST = 1020 };
class CNetMsg {
public:
    int m_from;
    int m_dpidFrom;
    int m_subType;
    unsigned long m_size;
    int m_uncompressedSize;
#include "inline/netmsg_cnetmsg_ctor.inl"
};
class CPlayerLostMsg : public CNetMsg {
public:
    int m_loser;
    LossConditionStruct m_lossCondition;
#include "inline/netmsg_player_lost_ctor.inl"
};
class CPlayerWonMsg : public CNetMsg {
public:
    int m_gamePos;
    VictoryConditionStruct m_victoryCondition;
#include "inline/netmsg_player_won_ctor.inl"
};
int transmitRemoteData(CNetMsg* msg, int toWho, bool compressMsg, bool guaranteed);

class NewSMapHeader {
public:
    char m_padToMinHumans[7];
    unsigned char m_minNumHumanPlayers;
    char m_padToTeamInfo[0xd - 8];
    signed char m_teamInfo[8];  // Mac onSameTeam reads game+0x1f105.
    char m_padToVictoryCondition[0x2c - 0x15];
    VictoryConditionStruct m_victoryCondition;
    LossConditionStruct m_lossCondition;  // Mac sender reads playerLoser at game+0x1f193.
    char m_padToMapName[0x2ac - 0x9c];
    std::string m_mapName;  // Mac game+0x1f3a4.
    int get(const char* path, const char* filename, int flags);
};

class TownExtra;
class hero;
class NewmapCell {
    unsigned char m_layout[0x22];
};
class NewfullMap {
    unsigned char m_beforeCellData[0x9c];
    NewmapCell* m_cellData;
    int m_size;
    unsigned char m_hasTwoLevels;
    unsigned char m_afterLevels[3];
    unsigned char m_objectTypeIndex[232 * 12];
public:
    NewmapCell* cell(int x, int y, int z);
    int getNumLevels();
private:
    NewmapCell* zCell(int x, int y, int z);
};
#include "inline/mapcell_get_num_levels.inl"
#include "inline/mapcell_z_cell.inl"
#include "inline/mapcell_cell_xyz.inl"

extern unsigned long long g_bitNumber[];
class town {
public:
    signed char m_id;  // Mac canBuild indexes m_towns with +0, stride 0x15c.
    signed char m_owner;  // Mac reward dialog reads +1.
    unsigned char m_builtThisTurn;  // Mac canBuild checks +2.
private:
    char m_beforeType[1];
public:
    signed char m_type;  // Mac spell scan reads +4.
public:
    unsigned char m_mapX;  // Mac buildBuilding reads +5..+7.
    unsigned char m_mapY;
    unsigned char m_mapZ;
    unsigned char m_dockSite;  // Mac canBuildDock reads +8.
private:
    char m_beforeHeroIds[0xc - 9];
public:
    int m_garrisonHeroId;
    int m_visitingHeroId;
public:
    signed char m_mageLevel;  // Mac spell helper reads +0x14.
private:
    char m_beforeGuildSpells[0x44 - 0x15];
public:
    int m_mageGuildSpells[5][6];  // Five 0x18-byte rows from +0x44.
    signed char m_mageGuildSpellCounts[5];
private:
    char m_beforeName[0xc4 - 0xc1];
public:
    std::string m_name;  // Mac loss dispatcher loads the handle at +0xc4.
    std::bitset<70> m_spells;  // Mac spell scan starts the town set at +0xc8.
private:
    char m_beforeBuilt[0x144 - 0xd4];
public:
    unsigned long long m_built;  // Mac guild-level scan reads +0x144.
    unsigned long long m_active;  // Mac Library check reads +0x14c.
    unsigned long long m_available;  // Mac isLegalBuilding reads +0x154.
public:
    void initializeSpells(const TownExtra* townSetup);
    void setSpellsAvailable();
    unsigned char canBuild(short buildingId) const;
    unsigned char canBuildDock() const;
    unsigned char isLegalBuilding(type_building_id building) const;
#include "inline/town_get_building_mask.inl"
#include "inline/town_has_building.inl"
#include "inline/town_is_castle.inl"
#include "inline/town_is_capitol.inl"
    type_building_id createBuilding(type_building_id building);
    void updateFullBuildingMask();
    void giveSpells(hero* forceHero) const;
    void applySpecialBuildingEffect(hero* townHero);
    type_building_id buildBuilding(int buildingId, unsigned char setBuiltFlag,
                                   unsigned char applySpecialEffect);
};
namespace std {
template <> class vector<town> {
    char m_beforeFirst[8];
    town* m_first;
public:
    town& operator[](int index) const { return m_first[index]; }
};
}
class hero {
    char m_beforeName[0x23];
public:
    enum { NUM_SPELLS = 70 };
    char m_name[13];  // Mac hero stride 0x486, name at +0x23.
    char m_afterName[0x486 - 0x23 - 13];
};

class SCampaign {
public:
    char m_layoutMarker;
    char m_beforeCurrentMap;
    signed char m_currentMap;  // Mac game+0x1ed06 in the campaign text arms.
    char m_beforeCurrentCampaign;
    int m_currentCampaign;  // Mac game+0x1ed08.
    int getScore() const;
    int getTotalTime() const;
};

// The Mac player array starts at game+0x1ff48 and has eight 0x15c-byte
// elements before the town vector at +0x20a28. TOWN canBuild calls the
// separately retained hasCapitol body through this array.
class playerData {
    char m_layout[0x15c];
public:
    bool isLocalHuman() const;
    bool hasCapitol();
};

class game {
public:
    char m_padToSpellDisabled[0x4a];
    unsigned char m_spellDisabledInfo[70];  // Mac spell scan reads game+0x4a.
    char m_padToCampaign[0x1ed04 - 0x4a - 70];
    SCampaign m_campaign;  // Mac score calls use game+0x1ed04.
    char m_padToPlayerDisabled[0x1eec6 - 0x1ed04 - sizeof(SCampaign)];
    char m_playerDisabled[8];  // Mac code loads at game+0x1eec6 and sign-extends.
    unsigned short m_day;
    unsigned short m_week;
    unsigned short m_month;
    char m_padToCheater[0x1ef28 - 0x1eed4];
    char m_isCheater;  // Mac showCongrats reads game+0x1ef28.
    unsigned char m_isTutorial;  // Mac canBuild tests +0x1ef29 without sign extension.
    char m_padToSetup[2];
    SGameSetupOptions m_setup;  // Mac filename +0x1ef65, path +0x1f060.
    NewSMapHeader m_mapHeader;  // Mac get call passes game+0x1f0f8.
    char m_padToWorldMap[0x1f3c0 - 0x1f0f8 - sizeof(NewSMapHeader)];
    NewfullMap m_worldMap;  // Mac cell storage begins at game+0x1f45c.
    playerData m_players[8];
    std::vector<town> m_towns;  // Mac first pointer at game+0x20a30.
    hero m_heroes[156];  // Mac stride 0x486, first hero game+0x20a34.
    #include "game_get_hero.inl"
    #include "inline/game_get_town.inl"
    bool townAlreadyBuiltOn(int townId) const;
    bool isHumanAlly(int playerNum) const;
    bool isHumanTeam(int teamNum) const;
#include "game_get_team.inl"
    bool isHuman(int gamePos) const;  // Mac call 0xe6bf4.
    int getLocalPlayerGamePos() const;
#include "inline/game_on_same_team.inl"
    int getTownId(int x, int y, int z);
    char* getPlayerName(int gamePos);
    short getBaseMapScore() const;
    short getCurrentTurn() const;
    short getMapScore() const;
    void convertObject(NewmapCell* tempCell);
    void setVisibility(int startX, int startY, int z,
                       int playerNum, int radius, int sourceType);
};

#include "inline/game_get_current_turn.inl"
#include "game_town_already_built.inl"
#include "game_is_human_ally.inl"

extern game* g_game;  // TOC 1+0x630 -> pointer storage 1+0x528940.
extern int g_gameOver;
extern bool g_inCampaign;  // TOC 1+0x614 -> flag at 1+0x5270a4.
extern char g_text[];
// Canonical Windows declarations name the same 0x69954c cell differently.
// The Mac loss sender reads the externally bound g_videoPaused storage.
extern int g_videoPaused;
#define g_networkActive69954c g_videoPaused
void sendPlayerLost();
void sendPlayerWon();
unsigned char getTeamNames(int player, char* names);

class mouseManager {
public:
    void hidePointer();
    void showPointer(bool restore);
};
extern mouseManager* g_mouseManager;

class soundManager {
public:
    void startMP3(const char* filename, int loopCount,
                  unsigned char stopSamples);
};
extern soundManager* g_soundManager;

class highScoreManager {
public:
    int addScoreToHighScore(int score, int days, int difficulty,
                            int scoreType, const char* land);
    static int getMonType(int score, int scoreType);
};
extern highScoreManager* g_highScoreManager;
extern const float g_mapScoreDifficultyFactor[];
std::string getCampaignName();
void videoOpen(int id, int x, int y, int w, int h, int a6, bool a7, bool a8);
void videoClose();
void congratsWait(int mode, char* rank, int base, int score, int dayz);

class heroWindowManager {
public:
    char m_padToDialogReturn[0x38];
    int m_dialogReturn;  // Mac single-player helper reads +0x38.
    char m_padToColorCycling[0x44 - 0x3c];
    int m_colorCyclingOn;
    void fadeScreen(int inOut, int speed, unsigned char expectFadein);
};
extern heroWindowManager* g_windowManager;

class TSingleSelectionWindow {
public:
    char m_storage[0x18c4];  // Together with the vptr: stack span 0x18c8.
    TSingleSelectionWindow(int gameMode);
    virtual ~TSingleSelectionWindow();
    virtual void doModal(bool fadeIn);
};

enum { DIALOG_RETURN_CANCEL = 0x7801 };
extern int g_unnamed699274;
extern char g_mapName[];

// The Mac dialog icon setter stores two four-byte string handles at +8/+12,
// then its frame and eight geometry dwords at +0x10..+0x30. Its switch range
// reaches Complete's Conflux/resource ordinal 0x24.

enum EGameResource {
    const_no_resource = -1,
    WOOD, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD,
    ABANDONED, RES_ARTIFACT, RES_SPELL, RES_COLOR,
    RES_GOOD_LUCK, RES_NEUTRAL_LUCK, RES_BAD_LUCK,
    RES_GOOD_MORALE, RES_NEUTRAL_MORALE, RES_BAD_MORALE,
    RES_EXPERIENCE, RES_HERO, RES_ARTIFACT_W_TEXT,
    RES_SECONDARY_SKILL, RES_MONSTER,
    RES_BUILDING_TT_0, RES_BUILDING_TT_1, RES_BUILDING_TT_2,
    RES_BUILDING_TT_3, RES_BUILDING_TT_4, RES_BUILDING_TT_5,
    RES_BUILDING_TT_6, RES_BUILDING_TT_7, RES_BUILDING_TT_8,
    RES_PRIMARY_SKILL_ATTACK, RES_PRIMARY_SKILL_DEFENSE,
    RES_PRIMARY_SKILL_POWER, RES_PRIMARY_SKILL_KNOWLEDGE,
    RES_MANA, RES_SMALL_GOLD
};

struct type_dialog_icon {
    EGameResource m_resource;
    long m_qualifier;
    std::string m_spriteName;
    std::string m_text;
    long m_spriteFrameIndex;
    long m_spriteX;
    long m_spriteY;
    long m_spriteHeight;
    long m_spriteWidth;
    long m_textX;
    long m_textY;
    long m_textHeight;
    long m_textWidth;
    void set(EGameResource resource, long qualifier);
};

// Mac normalDialog places the first icon at +0x28, eight 0x34-byte icons,
// then the dialog selectors at +0x1c8/+0x1cc/+0x1d0. Natural alignment
// supplies the gap after the bool; retail's implicit copy skips those bytes.
// The field identities and order come from the canonical declaration.
enum EMBType {
    NORMAL_DIALOG_DEFAULT = 1,
    NORMAL_DIALOG_YESNO = 2,
    NORMAL_DIALOG_ORDINAL_3 = 3,
    NORMAL_DIALOG_POPUP = 4,
    NORMAL_DIALOG_ORDINAL_5 = 5,
    NORMAL_DIALOG_ORDINAL_6 = 6,
    NORMAL_DIALOG_CHOOSE = 7,
    NORMAL_DIALOG_ORDINAL_8 = 8,
    NORMAL_DIALOG_ORDINAL_9 = 9,
    NORMAL_DIALOG_CHOOSE_OPTIONAL = 10
};

struct TNormalDialogInfo {
    std::string m_dialogText;
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    int m_textWidgetX;
    int m_textWidgetY;
    int m_textWidgetWidth;
    int m_textWidgetHeight;
    bool m_textExpansion;
    type_dialog_icon m_icons[8];
    EMBType m_mbType;
    int m_special;
    int m_timeout;
};

void calculateNormalDialogSize(TNormalDialogInfo& info);
void doNormalDialog(TNormalDialogInfo info);
void normalDialog(const char* text, int mbType, int x, int y,
                  int resType1, int resExtra1, int resType2, int resExtra2,
                  int special, int timeout, int resType3, int resExtra3);

// Table strides and name offsets are visible in the Mac setter's indexed
// loads: artifact 0x20/+0, spell 0x88/+0x10, secondary skill 0x10/+0.
struct TArtifactTraits {
    const char* m_name;
    char m_remaining[0x20 - 4];
};
struct SSpellTraits {
    char m_beforeName[0x10];
    const char* m_name;
    char m_beforeLevel[0x18 - 0x14];
    int m_level;  // Mac spell scan reads +0x18.
    char m_beforeTownProbability[0x44 - 0x1c];
    int m_townProbability[9];  // Mac faction-indexed weights at +0x44.
    char m_remaining[0x88 - 0x68];
};
struct TSSkillTraits {
    const char* m_name;
    const char* m_levelNames[3];
};
extern const TArtifactTraits (&g_artifactTraits)[144];
extern const SSpellTraits (&g_spellTraits)[81];
extern const TSSkillTraits (&g_sSkillTraits)[28];
extern const char* g_townBuildingSpriteNames[9];
extern char* g_playerColorNames[];
extern const char* g_primarySkillNames[4];
extern const char* g_skillMasteryNames[3];
extern const char* g_resourceNames[8];

// The canonical getArmyName header body indexes 0x74-byte Mac creature
// traits, loading singular/plural pointers at +0x14/+0x18.
struct TCreatureTypeTraits {
    char m_beforeName[0x14];
    const char* m_name;
    const char* m_pluralName;
    char m_remaining[0x74 - 0x1c];
};
extern const TCreatureTypeTraits (&g_creatureTypeTraits)[150];
const int g_creatureTypeLast = 0x96;

class TTextResource {
public:
    char m_beforeText[0x1c];
    std::vector<char*> m_text;
#include "inline/textresource_get_text.inl"
#include "inline/textresource_index.inl"
};
extern const TTextResource* g_generalText;

struct MacFontSettings {
    char m_beforeHeight[0x21];
    unsigned char m_height;
};
class font {
public:
    MacFontSettings m_fs;
    int getCharacterWidth(unsigned char value) const;
    int lineLength(const char* text, int width) const;
    int longestLineWidth(const char* text) const;
    int longestWrappedLineWidth(const char* text, int width) const;
    int longestWordLength(const char* text) const;
};
extern font* g_unnamed698a08;
extern font* g_mediumFont;  // Mac TOC 1+0xbc0 -> external pointer 1+0x52708c.
// The owning KB TU defines this constant as 110; the Mac target uses li 110.
static const int g_dialogIconMaxTextWidth = 110;
static const int g_dialogIconMaxRows = 2;
static const int g_dialogIconRowSingle = 1;
static const int g_dialogIconRowPair = 2;
static const int g_dialogIconRowTriple = 3;
static const int g_dialogIconRowQuad = 4;

class CSprite {
public:
    virtual void dispose();
    char m_beforeWidth[0x30 - 4];
    int m_width;
    int m_height;
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
};
namespace ResourceManager {
CSprite* getSprite(const char* name);
}

std::string formatString(const char* format, ...);
const char* getBuildingName(int townType, int buildingId);
#include "inline/creaturetype_get_army_name.inl"
inline int max(int left, int right) { return left > right ? left : right; }
inline int min(int left, int right) { return left < right ? left : right; }
#define HIWORD(value) (static_cast<unsigned short>((static_cast<unsigned long>(value) >> 16)))
#define LOWORD(value) (static_cast<unsigned short>(value))

#endif
