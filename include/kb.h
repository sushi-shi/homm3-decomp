// kb.h - prototypes of kb.cpp (compiland kb.obj)
#ifndef HOMM3_KB_H
#define HOMM3_KB_H

#include <string>
#include <vector>
#include "border.h"
#include "town.h"

class message;
class TDialogBox;
class VictoryConditionStruct;
class LossConditionStruct;

// Dreamcast supplies the identity, member function roster, and the source
// order consumed by set(). Retail independently fixes the Complete layout:
// two four-byte selectors, two VC6 strings, then seven dwords. The paired
// position fields remain split until a shared point type with an eight-byte
// retail layout is admitted.
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
SIZE(type_dialog_icon, 0x4c);

// Dreamcast CodeView publishes the five named members of EMBType. Complete's
// DoNormalDialog jump table independently proves that the domain remains dense
// through ten and uses the five intervening values too. Their source names did
// not survive, so keep ordinal placeholders for those Complete-only facts.
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

// Dreamcast supplies the record identity, all shared members and the array
// count. Complete adds text_expansion between the text rectangle and icons;
// retail's by-value DoNormalDialog ABI fixes every translated VC6 offset and
// the final 0x2a0 extent. The special members are intentionally implicit:
// CodeView marks them compiler-generated, and retail expands this aggregate's
// teardown while retaining type_dialog_icon's element destructor boundary.
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
    // NH3API confirms the three-byte gap between text_expansion at
    // +0x30 and icons at +0x34. Retail kb.cpp uses the one-byte flag.
    char m_paddingBeforeIcons[3];
    type_dialog_icon m_icons[8];
    EMBType m_mbType;
    int m_special;
    int m_timeout;
};
SIZE(TNormalDialogInfo, 0x2a0);

// The normal-dialog rollover frame has one canonical project-wide model.
// Retail's inlined constructor proves the base extent, derived vtable store
// and the two four-byte tail members.
class type_normal_dialog_frame : public coloredBorderFrame {
public:
    EGameResource m_resource;
    long m_qualifier;

    type_normal_dialog_frame(long x, long y, long w, long h, long id,
                             EGameResource resource, long qualifier);
    virtual bool handleClick(bool downClick,
                                       bool rightClick);
};
SIZE(type_normal_dialog_frame, 0x40);

// homm2's KB timer array survives (DC glTimers: unsigned long[10];
// retail base 0x698998 - button::Select stores slot 2 at 0x6989a0).
// Slot 2's name is the homm2 2.1 KB.h value. UpdateScreen independently
// proves slot 0's adventure-animation role from retail.
enum EKbTimerSlots {
    GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT = 0,
    GLOBAL_BUTTON_REPEAT_TIMER_SLOT = 2
};

// GetTeamNames selects the natural-language conjunction only for a team of
// exactly two active players; larger teams use comma-separated forms.
enum EKbTeamNameCount {
    TEAM_NAMES_PAIR = 2
};

// HandleAppSpecificMenuCommands' command domain. The shared names and values
// come from the original HoMM2 source lineage; Complete retail independently
// proves every retained value through its two jump tables and four range
// guards. The combat-debug members whose source names did not survive keep
// ordinal spellings instead of acquiring guessed semantics.
enum EAppMenuCommand {
    APP_MENU_FORCE_VICTORY = 0x9c7b,
    APP_MENU_FORCE_DEFEAT = 0x9c7c,
    APP_MENU_TOGGLE_VIEW_ALL = 0x9c7e,
    APP_MENU_EXIT = 0x9ccc,
    APP_MENU_CHEAT_REVEAL = 0x9ccd,
    APP_MENU_CHEAT_MOVEMENT = 0x9cce,
    APP_MENU_CHEAT_RESOURCES = 0x9cd0,

    APP_MENU_ARMY_FIRST = 0xa028,
    APP_MENU_ARMY_LAST = 0xa0b9,
    APP_MENU_SECONDARY_FIRST = 0xa410,
    APP_MENU_SECONDARY_LAST = 0xa480,
    APP_MENU_ARTIFACT_FIRST = 0xafc8,
    APP_MENU_ARTIFACT_LAST = 0xb058,

    APP_MENU_COMBAT_ORDINAL_B798 = 0xb798,
    APP_MENU_COMBAT_DESTROY_OPPOSING_ARMY = 0xb799,
    APP_MENU_COMBAT_DESTROY_ACTING_ARMY = 0xb79a,
    APP_MENU_COMBAT_ORDINAL_B79B = 0xb79b,
    APP_MENU_COMBAT_ORDINAL_B79C = 0xb79c,
    APP_MENU_COMBAT_REBUILD_OBSTACLES = 0xb79d,
    APP_MENU_COMBAT_ORDINAL_B79E = 0xb79e,

    APP_MENU_SPELL_ALL = 0xb3b0,
    APP_MENU_SPELL_SCHOOL_FIRST = 0xb3b1,
    APP_MENU_SPELL_SCHOOL_SECOND = 0xb3b2,
    APP_MENU_SPELL_SCHOOL_THIRD = 0xb3b3,
    APP_MENU_SPELL_SCHOOL_FOURTH = 0xb3b4,
    APP_MENU_SPELL_LEVEL_BASE = 0xb3b4,
    APP_MENU_SPELL_LEVEL_ONE = 0xb3b5,
    APP_MENU_SPELL_LEVEL_TWO = 0xb3b6,
    APP_MENU_SPELL_LEVEL_THREE = 0xb3b7,
    APP_MENU_SPELL_LEVEL_FOUR = 0xb3b8,
    APP_MENU_SPELL_LEVEL_FIVE = 0xb3b9,
    APP_MENU_SPELL_LAST = 0xb415
};

enum EAppMenuConstant {
    APP_MENU_REVEAL_COORDINATE = 30,
    APP_MENU_REVEAL_RADIUS = 180,
    APP_MENU_MOVEMENT_BONUS = 299999,
    APP_MENU_RESOURCE_COUNT = 7,
    APP_MENU_RESOURCE_BONUS = 100,
    APP_MENU_GOLD_BONUS = 100000,
    APP_MENU_SECONDARY_LEVELS = 4,
    APP_MENU_ARMY_QUANTITY = 5,
    APP_MENU_SPELL_POINTS = 999
};

enum ECheckEndGameForcedResult {
    END_GAME_FORCE_VICTORY = 1,
    END_GAME_FORCE_DEFEAT = 2
};

// The five columns of the end-of-game score sheet CongratsWait draws.
// Retail dispatches the value line through a five-entry jump table on the
// column ordinal, and each arm is what names the column: the calendar day
// count, the base map score, the difficulty row from the ARRAYTXT table,
// the rated final score, and the rank string ShowCongrats formats.
enum ECongratsColumn {
    CONGRATS_COLUMN_DAYS = 0,
    CONGRATS_COLUMN_BASE_SCORE = 1,
    CONGRATS_COLUMN_DIFFICULTY = 2,
    CONGRATS_COLUMN_SCORE = 3,
    CONGRATS_COLUMN_RANK = 4,
    CONGRATS_COLUMN_COUNT = 5
};

extern unsigned long g_timers[10];

// Retail .bss pointer cell used by both map-extra accessors. The complete
// linearization is ((z * height + y) * width + x), with 16-bit elements.
DATA(0x006989f8) extern unsigned short* g_mapExtra;

// The shared fonts oldmain (0x4ee3e0) loads by name and ShutDown
// (0x4f3690) releases through the resource vtable, in retail's own .bss
// order: 0x698a04 tiny.fnt, 0x698a08 smalfont.fnt, 0x698a0c medfont.fnt,
// 0x698a10 bigfont.fnt, 0x698a14 Calli10R.fnt. Every cell takes the
// return of the same one-argument loader and the name is the string that
// load passes, which fixes both the type and the identity; ShutDown's
// `mov edx,[cell] / call [edx+4]` teardown independently proves they are
// resource pointers. Only the calligraphic cell has a consumer in a
// modeled TU so far - the hill fort constructor's per-slot count widget
// takes its y as `0x7c - font->height`, reading font+0x21, i.e.
// TFontSpec::height at font+0x1c+5. Declare the rest as readers land.
class font;
// DC ?Credits@@3PAPBDA / ?smallFont@@3PAVfont@@A. CreditsWait reads the
// scrolling text from Credits[0] and its closing line from Credits[1]
// (retail .bss 0x6a7700 / 0x6a7704) and draws the latter with smallFont
// (.bss 0x698a08, one of the three fonts ShutDown disposes). Their owning
// compilands are not located yet.
extern const char* g_credits[];
extern font* g_smallFont;
// The first cell of the same run: army::DrawToBuffer (0x43e140) draws
// the troop-count box's number with it, which is the reader the note
// above was waiting on.
DATA(0x00698a14) extern font* g_calligraphicFont;
// The third cell of the same canonical font run.  CWaitForReadyPlayersDlg
// passes it to CAnimatedDlg::Setup.
DATA(0x00698a04) extern font* g_tinyFont;
// The fourth cell of the run (bigfont.fnt): the lobby window's panel
// titles (TSingleSelectionWindow::Update, 0x584550) draw with it.
DATA(0x00698a0c) extern font* g_mediumFont;
DATA(0x00698a10) extern font* g_bigFont;

unsigned short getMapExtra(int x, int y, int z);
unsigned short* getMapExtraPtr(int x, int y, int z);

// Live prototypes (claimed kb.cpp bodies; called from kbwin's
// AppCommand and exec's DoDialog).
void shutDown(const char* inExitMessage);               // 0x4f3690
// DC ?bInShutDown@@3_NA, retail .bss 0x69958d: ShutDown's re-entry
// guard (kb.cpp owns the definition).
extern bool g_inShutDown;
// Dreamcast kb.cpp:4187 tears the CD/serial layer down; Complete's body
// is EMPTY - executive::ShutDownSystem's call lands on the image-wide
// `ret` at 0x5bc690 that /OPT:ICF folded every empty function onto.
void earlyShutDownSystem();
void fileError(const char* buf);                        // 0x4f3a60
// The five .rdata score multipliers game::get_map_score indexes with
// setup.difficulty. Owning TU not located; extern only (the gTownSizeNames
// pattern).
extern const float g_mapScoreDifficultyFactor[];          // 0x67f558
int handleAppSpecificMenuCommands(int idItem);           // 0x4f4350
void cleanUpMenus();                                     // 0x4f4b50
int getNextHumanPlayer(int start);                       // 0x4f4ba0
void normalDialog(const char* text, int mbType, int x, int y,
    int resType1, int resExtra1, int resType2, int resExtra2,
    int special, int timeout, int resType3, int resExtra3);  // 0x4f6570
void normalDialogTimeOut(const char* text, int mbType, int timeOut,
    int x, int y, int resType1, int resExtra1, int resType2,
    int resExtra2, int special, int resType3, int resExtra3); // 0x4f6530
void doNormalDialog(TNormalDialogInfo dialogInfo);              // 0x4f6990
// DC kb.cpp:5385 (dc 0xe5960); retail 0x4f5d80 (1,296 B), unclaimed.
// NormalDialog sizes its info block through it before DoNormalDialog.
void calculateNormalDialogSize(TNormalDialogInfo& dialogInfo);
int eventWindowHandler(message& msg);
TDialogBox* getCurrentNormalDialog();
void extendedDialog(const char* text,
    std::vector<type_dialog_resource>& resources,
    long x, long y, long timeout);
void __fastcall getQuickviewSize(const char* text, int* width,
                                   int* height);                 // 0x4f62a0
// Located kb.cpp bodies kbwin's WinMain / AppWndProc call (bodies not
// yet reconstructed; the declarators match the kbwin call sites).
// WinMain's first gate. The retail row at 0x4ed650 is the Dreamcast
// EarlySetup - it guards on bEarlySetupDone, calls InitMainClasses,
// GetDesktopInfo, ReadPrefs/WritePrefs, ResourceManager::SetPath/Open and
// then the whole LoadGameData table run - NOT InitMainClasses, whose own
// body is the twelve manager `new`s at 0x4edb40.
int earlySetup();                                        // 0x4ed650
void initMainClasses();                                  // 0x4edb40
int oldmain();
void creditsWait();
void showCredits();
void lostGame();
// Retail-only dword paired with the Dreamcast-named giHighMemBuffer in
// oldmain's low-debug-memory defaults. No surviving source symbol names it.
// The image-wide allocation-failure handler: every `new` site that
// null-checks its result calls it (67 B at 0x4f42c0, no args, sprintf
// into gText then ShutDown). DC kb.obj MemError, dc 0xe44f0/64 B,
// kb.cpp:4168 - arity and role both agree.
DATA(0x006994ec) extern int g_unnamed6994ec;
void memError();
void handleRemoteDeadPlayerExit(int dpGamePos, unsigned char showMsg);
int gameUnsaved();                                       // 0x4f4310
void checkEndGame(int forceWin);                        // 0x4f2ce0
bool displayVCWinLoss(VictoryConditionStruct& victoryCondition,
                      int& gameWon, int& gameLost, bool remoteCheck);
unsigned char displayLCWinLoss(LossConditionStruct& lossCondition,
                               int& gameWon, int& gameLost,
                               unsigned char remoteCheck);
// Retail .bss 0x6972b8, an INT that every CheckEndGame caller which then
// wants to keep touching the adventure UI reads immediately afterwards -
// 36 image-wide references, the bulk of them inside kb.obj's own band
// beside CheckEndGame itself. The Dreamcast carries exactly one int
// public that fits the role, `?gbGameOver@@3HA`; the name comes from
// there and the address from retail. NOT claimed - the owning TU is
// kb.cpp and its data band has not landed.

// GATED, and it has to be: ungated, this ONE `extern int` takes
// recruit.obj's recruitUnit::Update 90.84 -> 88.24 - the same function
// and the same two numbers the RS_ERASE_OBJECT enumerator produced last
// round. So the standing note that plain externs are inert is wrong for
// this consumer; kb.h reaches it and its include-set sensitivity counts
// declarations of every kind. Measured both ways 2026-08-14.
extern int g_gameOver;
// PC-only zero-fill storage.  The command-line initialization path clears
// this word and philAI::DoAI is its only reader; no source symbol survives,
// so keep the address-ordinal spelling instead of inventing a role name.
extern int g_unnamed6994f0;
int trueFalseDialogHandler(message* msg);

// kb.cpp's shared text scratch buffer (.bss 0x6973d8 in kb's band;
// kbwin's WinMain sprintf's the already-running message into it,
// strip's DrawIcons the troop counts).
extern char g_text[];

// homm2 gbForegroundApp (KB.cpp) lineage: AppWndProc's WM_ACTIVATEAPP
// arm stores the activation byte here. Retail address 0x6783d0 sits in
// a .data band no claimed TU owns yet - PROVISIONAL owner kb.h until
// the owning TU lands (kbwin is the only known writer).
extern unsigned char g_foregroundApp;

void incProgressBar(unsigned char update);
void showProgressBar();
void drawProgressCount();
void unloadProgressBar();
// CODEVIEW(E:\gamedcs\kb.cpp:481, dc 0xdf6d0) void DeleteMainClasses();
// CODEVIEW(E:\gamedcs\kb.cpp:553, dc 0xdf840) void EarlyShutdown(const char* cTitle, const char* cBody);
// CODEVIEW(E:\gamedcs\kb.cpp:580, dc 0xdf854) void SetupCDRom();
// CODEVIEW(E:\gamedcs\kb.cpp:645, dc 0xdf91c) int EarlySetup();
// CODEVIEW(E:\gamedcs\kb.cpp:814, dc 0xdff5c) void ShowCredits();
// CODEVIEW(E:\gamedcs\kb.cpp:962, dc 0xe0158) int oldmain();
// CODEVIEW(E:\gamedcs\kb.cpp:1841, dc 0xe130c) int DoNewGame();
// CODEVIEW(E:\gamedcs\kb.cpp:1962, dc 0xe15b0) int DoCampaignWindow(unsigned char newGame);
// CODEVIEW(E:\gamedcs\kb.cpp:1988, dc 0xe1630) int DoSinglePlayerWindow();
// CODEVIEW(E:\gamedcs\kb.cpp:2036, dc 0xe1740) int DoMultiPlayerWindow();
// CODEVIEW(E:\gamedcs\kb.cpp:2059, dc 0xe1778) int DoLoadGame();
// CODEVIEW(E:\gamedcs\kb.cpp:2145, dc 0xe1950) int PickLoadGame();
// CODEVIEW(E:\gamedcs\kb.cpp:2174, dc 0xe1990) int InterpretCommandLine();
// Original: NullHandler, kb.cpp:2270.
int nullHandler(message& msg);
// CODEVIEW(E:\gamedcs\kb.cpp:2332, dc 0xe1bf8) int ExitNormalDialog(message* msg);
// CODEVIEW(E:\gamedcs\kb.cpp:2720, dc 0xe24a0) void check_player_loss();
// CODEVIEW(E:\gamedcs\kb.cpp:2867, dc 0xe2930) void SendPlayerWon();
// CODEVIEW(E:\gamedcs\kb.cpp:2878, dc 0xe296c) void SendPlayerLost();
// CODEVIEW(E:\gamedcs\kb.cpp:2888, dc 0xe29a8) unsigned char DisplayVCWinLoss(VictoryConditionStruct* VictoryCondition, int* bGameWon, int* bGameLost, unsigned char remoteCheck);
// CODEVIEW(E:\gamedcs\kb.cpp:3419, dc 0xe34c4) int GetEnemyCount();
// CODEVIEW(E:\gamedcs\kb.cpp:3440, dc 0xe3558) unsigned char DisplayLCWinLoss(LossConditionStruct* LossCondition, int* bGameWon, int* bGameLost, unsigned char remoteCheck);
// CODEVIEW(E:\gamedcs\kb.cpp:3763, dc 0xe3a04) void InitVars();
// CODEVIEW(E:\gamedcs\kb.cpp:4102, dc 0xe4330) void ShowCongrats(int hsType);
// CODEVIEW(E:\gamedcs\kb.cpp:4187, dc 0xe4530) void EarlyShutDownSystem();
// CODEVIEW(E:\gamedcs\kb.cpp:4214, dc 0xe45dc) unsigned char LoadGameData();
// CODEVIEW(E:\gamedcs\kb.cpp:4781, dc 0xe519c) int GetNextHumanPlayer(int start);
// CODEVIEW(E:\gamedcs\kb.cpp:4855, dc 0xe52a8) int CheckMem();
// CODEVIEW(E:\gamedcs\kb.cpp:5206, dc 0xe5960) void CalculateNormalDialogSize(TNormalDialogInfo& dialog_info);
// CODEVIEW(E:\gamedcs\kb.cpp:5478, dc 0xe5f60) TDialogBox* GetCurrentNormalDialog();
// CODEVIEW(E:\gamedcs\kb.cpp:5499, dc 0xe5fbc) void NormalDialog(const char* cText, int iMBType, int x, int y, int iResType1, int iResExtra1, int iResType2, int iResExtra2, int iSpecial, int iTimeout, int iResType3, int iResExtra3);
// CODEVIEW(E:\gamedcs\kb.cpp:5909, dc 0xe6cf0) void extended_dialog(const char* text, std::vector<type_dialog_resource,std::allocator<type_dialog_resource>* resources, long x, long y, long timeout);
// CODEVIEW(E:\gamedcs\kb.cpp:5954, dc 0xe6e2c) unsigned short GetMapExtra(int x, int y, int z);
// CODEVIEW(E:\gamedcs\kb.cpp:5960, dc 0xe6e50) unsigned short* GetMapExtraPtr(int x, int y, int z);

// --- CCombatTypeMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:770, dc 0xe704c) void CCombatTypeMsg::CCombatTypeMsg(int quick);

// --- CLogFile ---
// CODEVIEW(E:\gamedcs\remote.h:235, dc 0xe709c) void CLogFile::InitLogFile();

// --- CMCDeadPlayer ---
// CODEVIEW(E:\gamedcs\netmsg.h:705, dc 0xe7024) void CMCDeadPlayer::CMCDeadPlayer(int playerPos);

// --- CNormalWinMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:862, dc 0xe7074) void CNormalWinMsg::CNormalWinMsg(int gamePos);

// --- CPlayerDeadMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:472, dc 0xe6f34) void CPlayerDeadMsg::CPlayerDeadMsg(int who);

// --- CPlayerLostMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:519, dc 0xe6fbc) void CPlayerLostMsg::CPlayerLostMsg(int loser, LossConditionStruct& lossConditionStruct);

// --- CPlayerWonMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:505, dc 0xe6f5c) void CPlayerWonMsg::CPlayerWonMsg(int gamePos, VictoryConditionStruct* victoryConditionStruct);

// --- CTimer ---
// CODEVIEW(E:\gamedcs\timer.h:33, dc 0xe70a0) void CTimer::enable();

// --- TNormalDialogInfo ---
// CODEVIEW(E:\gamedcs\kb.cpp:5466, dc 0xe742c) void TNormalDialogInfo::TNormalDialogInfo();
// CODEVIEW(E:\gamedcs\kb.cpp:5466, dc 0xe7460) void TNormalDialogInfo::~TNormalDialogInfo();
// CODEVIEW(E:\gamedcs\kb.cpp:5528, dc 0xe74d8) void TNormalDialogInfo::TNormalDialogInfo(const TNormalDialogInfo* __that);

// --- advManager ---
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe7114) void* advManager::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe7204) void advManager::~advManager();

// --- combatManager ---
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe70e0) void* combatManager::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe71b0) void combatManager::~combatManager();

// --- game ---
// CODEVIEW(E:\gamedcs\kb.cpp:3830, dc 0xe3b84) void game::ShowLuckInfo(hero* thisHero, int iMBType);

// --- highScoreManager ---
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe7148) void* highScoreManager::`scalar deleting destructor'(unsigned __flags);

// --- mouseManager ---
// CODEVIEW(E:\gamedcs\MouseMgr.h:215, dc 0xe6eb4) mouseManager::EPointerSet mouseManager::GetSet();

// --- playerData ---
// CODEVIEW(E:\gamedcs\kb.cpp:1837, dc 0xe724c) playerData* playerData::operator=(const playerData* __that);

// --- searchArray ---
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe70ac) void* searchArray::`scalar deleting destructor'(unsigned __flags);

// --- soundManager ---
// CODEVIEW(E:\gamedcs\SoundMgr.h:124, dc 0xe6ebc) void soundManager::~soundManager();
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe717c) void* soundManager::`scalar deleting destructor'(unsigned __flags);

// --- std ---
// CODEVIEW(E:\gamedcs\kb.cpp:542, dc 0xe7234) void std::set<enum SpellID,std::less<enum SpellID>,std::allocator<enum SpellID> >::~set<enum SpellID,std::less<enum SpellID>,std::allocator<enum SpellID> >();
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xe75bc) void std::vector<resource *,std::allocator<resource *> >::~vector<resource *,std::allocator<resource *> >();
// CODEVIEW(..\stlport\stl_tree.h:440, dc 0xe75e4) void std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::~_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >();
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xe7608) void std::vector<combatManager::TObstacle,std::allocator<combatManager::TObstacle> >::~vector<combatManager::TObstacle,std::allocator<combatManager::TObstacle> >();
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xe7630) void std::_Vector_base<resource *,std::allocator<resource *> >::~_Vector_base<resource *,std::allocator<resource *> >();
// CODEVIEW(..\stlport\stl_tree.h:263, dc 0xe7660) void std::_Rb_tree_base<enum SpellID,std::allocator<enum SpellID> >::~_Rb_tree_base<enum SpellID,std::allocator<enum SpellID> >();
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xe7688) void std::_Vector_base<combatManager::TObstacle,std::allocator<combatManager::TObstacle> >::~_Vector_base<combatManager::TObstacle,std::allocator<combatManager::TObstacle> >();
// CODEVIEW(..\stlport\stl_string.h:179, dc 0xe76c8) void std::_STL_alloc_proxy<resource * *,resource *,std::allocator<resource *> >::~_STL_alloc_proxy<resource * *,resource *,std::allocator<resource *> >();
// CODEVIEW(..\stlport\stl_string.h:179, dc 0xe76e0) void std::_STL_alloc_proxy<std::_Rb_tree_node<enum SpellID> *,std::_Rb_tree_node<enum SpellID>,std::allocator<enum SpellID> >::~_STL_alloc_proxy<std::_Rb_tree_node<enum SpellID> *,std::_Rb_tree_node<enum SpellID>,std::allocator<enum SpellID> >();
// CODEVIEW(..\stlport\stl_string.h:179, dc 0xe76f8) void std::_STL_alloc_proxy<combatManager::TObstacle *,combatManager::TObstacle,std::allocator<combatManager::TObstacle> >::~_STL_alloc_proxy<combatManager::TObstacle *,combatManager::TObstacle,std::allocator<combatManager::TObstacle> >();
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xe7710) void std::_STL_alloc_proxy<resource * *,resource *,std::allocator<resource *> >::deallocate(resource** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xe773c) void std::allocator<resource *>::deallocate(resource** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.c:207, dc 0xe7758) std::vector<type_point,std::allocator<type_point>* std::vector<type_point,std::allocator<type_point> >::operator=(const std::vector<type_point,std::allocator<type_point>* __x);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xe7834) void std::destroy(resource** __first, resource** __last);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xe7864) std::allocator<resource* std::__stl_alloc_rebind(std::allocator<resource* __a, resource** __formal);
// CODEVIEW(..\stlport\stl_vector.h:199, dc 0xe7868) unsigned std::vector<type_point,std::allocator<type_point> >::capacity();
// CODEVIEW(..\stlport\stl_vector.h:514, dc 0xe7874) std::vector<type_point,std::allocator<type_point> >::_M_allocate_and_copy(unsigned __n, const type_point* __first, const type_point* __last);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xe78ac) type_point* std::copy(const type_point* __first, const type_point* __last, type_point* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xe78fc) type_point* std::uninitialized_copy(const type_point* __first, const type_point* __last, type_point* __result);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xe7934) resource** std::value_type(resource** __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xe7938) void std::__destroy(resource** __first, resource** __last, resource** __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xe7954) type_point* std::__copy(const type_point* __first, const type_point* __last, type_point* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xe7988) type_point* std::__uninitialized_copy(const type_point* __first, const type_point* __last, type_point* __result, type_point* __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xe79a4) void std::__destroy_aux(resource** __first, resource** __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xe79d4) type_point* std::__uninitialized_copy_aux(const type_point* __first, const type_point* __last, type_point* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xe7a10) void std::destroy(resource** __pointer);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xe7a2c) void std::__destroy_aux();

// --- type_dialog_icon ---
// CODEVIEW(E:\gamedcs\kb.cpp:4897, dc 0xe52b8) void type_dialog_icon::set(EGameResource _resource, long _qualifier);
// CODEVIEW(E:\gamedcs\kb.cpp:5466, dc 0xe7490) void type_dialog_icon::type_dialog_icon();
// CODEVIEW(E:\gamedcs\kb.cpp:5466, dc 0xe74b4) void type_dialog_icon::~type_dialog_icon();
// CODEVIEW(E:\gamedcs\kb.cpp:5528, dc 0xe755c) void type_dialog_icon::type_dialog_icon(const type_dialog_icon* __that);

// --- type_normal_dialog_frame ---
// CODEVIEW(E:\gamedcs\kb.cpp:2442, dc 0xe1de4) void type_normal_dialog_frame::type_normal_dialog_frame(long _x, long _y, long _w, long _h, long _id, EGameResource _resource, long _qualifier);
// CODEVIEW(E:\gamedcs\kb.cpp:2452, dc 0xe1e58) unsigned char type_normal_dialog_frame::handle_click(unsigned char down_click, unsigned char right_click);
// CODEVIEW(E:\gamedcs\kb.cpp:2445, dc 0xe73e0) void* type_normal_dialog_frame::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\kb.cpp:2445, dc 0xe7414) void type_normal_dialog_frame::~type_normal_dialog_frame();

#endif  /* HOMM3_KB_H */
