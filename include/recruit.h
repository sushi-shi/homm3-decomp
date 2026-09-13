// recruit.h - prototypes of recruit.cpp (compiland recruit.obj)
#ifndef HOMM3_RECRUIT_H
#define HOMM3_RECRUIT_H

#include <string.h>
#include <va.h>
#include "basemgr.h"
#include "slider.h"
#include "window.h"
#include "palette.h"

class armyGroup;
class button;
class hero;
class iconWidget;
class recruitUnit;
class town;

void getMonsterCost(int monId, int* resCost);
void getUpgradeCost(enum TCreatureType creature, enum TCreatureType upgrade,
    long amount, long* cost);
void quickViewRecruit(town* newTown, int newDwellingIndex);
void quickViewRecruit(enum TCreatureType monType, short* numMon);

// Recruit-window message ids, fixed by the constructor's widget ids and by
// recruitUnit::Main's retail switch tables.
enum ERecruitWidgetId {
    RECRUIT_QUANTITY_ID = 0x20e,
    RECRUIT_MAXIMUM_ID = 0x214,
    RECRUIT_CREATURE_0_ID = 0x21a,
    RECRUIT_CREATURE_1_ID = 0x21b,
    RECRUIT_CREATURE_2_ID = 0x21c,
    RECRUIT_CREATURE_3_ID = 0x21d,
    RECRUIT_STATUS_ID = 0x231,
    RECRUIT_CANCEL_ID = 0x7801,
    RECRUIT_ACCEPT_ID = 0x7802
};

enum ERecruitCreatureSlot {
    RECRUIT_SLOT_0 = 0,
    RECRUIT_SLOT_1 = 1,
    RECRUIT_SLOT_2 = 2,
    RECRUIT_SLOT_3 = 3
};

// The recruit dialog's own window and its quick-view twin. Both derive
// DIRECTLY from heroWindow, not from CHeroWindowEx: their retail
// destructors (0x54fb20 / 0x5516e0) store ONE vptr (0x640c4c /
// 0x640c7c) and then call heroWindow::~heroWindow (0x5fea80) - an
// intermediate class with its own vtable would have stored its vptr
// first. The constructor, add_creature_widgets, Open and Update now close
// TRecruitWindow's own 0x20-byte tail exactly.

// Three widget handles ARE now byte-proven, by recruitUnit::Update's
// enable pass (0x550306..0x5503b7): +0x50 is enabled only when
// numberToBuy != 0 && maxAvail > 0 && !view_only (the confirm
// control), +0x54 on maxAvail > 0 && !view_only, and +0x58 is the
// dialog's slider - the same pass drives its SetResolution/SetState
// slots. Their NAMES are unattested, so they keep ordinal
// placeholders. The constructor proves +0x4c is recruit_info and proves
// +0x50/+0x54 are button pointers: their derived-to-widget conversions
// materialize the exact temporary consumed by vector<widget*>::push_back.
class TRecruitWindow : public heroWindow {
public:
    recruitUnit* m_recruitInfo;
    // Retail ctor 0x54e850 installs RECRUIT_ACCEPT_ID (0x7802) here;
    // update enables it only for a valid, nonzero purchase quantity.
    button* m_acceptButton;
    // Same ctor installs RECRUIT_MAXIMUM_ID (0x214) here.
    button* m_maximumButton;
    // Quantity control: update sets resolution=maxAvail+1 and state=numberToBuy.
    // These three names are role-derived; no original member names recovered.
    slider* m_quantitySlider;
    // Four creature portraits, byte-proven by
    // add_creature_widgets' [this + slot*4 + 0x5c] stores. recruitUnit::Open
    // allocates 0x6c bytes for this class, closing the tail exactly.
    iconWidget* m_creatureWidgets[4];

    TRecruitWindow(int x2, int y2, int altResource,
                   recruitUnit* recruitInfo);
    virtual ~TRecruitWindow();
    void addCreatureWidgets(long startX, long startY, long nameY,
                              TCreatureType creature, long slot);
};
SIZE(TRecruitWindow, 0x6c);

// The recruit dialog's window, .bss 0x69d5e8. Name provisional (the
// gp<Type> house convention); recruitUnit::Open builds it,
// recruitUnit::Close RemoveWindow()s and deletes it, and Update
// broadcasts every widget refresh through it.
extern TRecruitWindow* g_recruitWindow;

// recruit.obj's own .bss 0x69d5f4 - the menu recruitUnit::Open parks
// before switching to the default one, and the menu ::Close puts back.
// Spelled through the HMENU handle tag rather than HMENU so this header
// keeps its promise not to drag <windows.h> into its five consumers.
extern struct HMENU__* g_recruitSavedMenu;

// Retail initialization 0x4ee430 calls ResourceManager::GetPalette
// (0x55b3e0) with the game palette name and stores the result at 0x6aacb0.
// The former SUnnamed6aacb0 was a partial TPalette16 view: its 0x1c-byte
// prefix is resource, followed by 256 16-bit palette entries. Former
// field_5a/field_64 are palette indices 31/36 (normal/selected recruit
// borders); levelUpSelectionColor is index 45; playerColors starts at 64.
// Former pad_00/pad_5c/pad_66/pad_78 belong to the base or palette array.
extern TPalette16* g_unnamed6aacb0;

class TRecruitQuickWindow : public heroWindow {
public:
    TRecruitQuickWindow(int x2, int y2);
    virtual ~TRecruitQuickWindow();
};

// PROVEN layout. Base is baseManager (0x38): all three retail
// constructors (0x551350 / 0x551460 / 0x551560) open with
// `call baseManager::baseManager` (0x44d530) and then store the vptr
// 0x640c70 at [this], and recruitUnit::Close (0x5502d0) parks
// baseManager::status at [this+0x34]. Own fields 0x38..0xbb, size
// 0xbc.

// Every named field below is byte-proven on THIS image; the Dreamcast
// roster (via the Ghidra struct recovery) supplies only the spellings,
// and its offsets line up one-for-one:
//   0x48 type          ctor1/2 store -1, ctor3 stores 0x62, Close
//                      compares 0x62
//   0x4c view_only     ctor1/2 store 0 (byte), ctor3 a setne result,
//                      Update reads it as a byte
//   0x50 monsterType   ctor arg, indexes the creature-record table
//   0x54 numAvail      short*; Update stores available[slot] into it
//   0x58 selectedPos.  ctors zero it, Update reads it as the default
//                      slot and as a widget-code offset
//   0x5c-0x68 MonType1..4 / 0x6c-0x78 available[4] - the ctor arg pairs
//   0x7c thisHero      ctor2 arg; Update passes it to hero::HasArtifact
//   0x84 goldPerTroop / 0x88 altResource / 0x8c resourcesPerTroop -
//                      the UpdateCost outputs
//   0x90 bInTownMainScreen  ctor3 arg, Close reads it
//   0x98 currArmyGroup / 0x9c bCurrArmyGroupIsTownGarrison - ctor1 args
//   0xa4 updateNeeded  Close reads it
//   0xac maxAvail / 0xb0 totalGold / 0xb4 totalResources /
//   0xb8 numberToBuy   Update computes and displays all four
// The gaps (0x38, 0x80, 0x94, 0xa0, 0xa8) stay padding: the Dreamcast
// names them but no retail body reconstructed here touches them.
class recruitUnit : public baseManager {
public:
    // Dreamcast array type 0x3dcc: four ints; NH3API agrees at +0x38.
    int m_currentSpriteFrame[4];
    int m_type;
    unsigned char m_viewOnly;
    TCreatureType m_monsterType;
    short* m_numAvail;
    int m_selectedPosition;
    TCreatureType m_monType1;
    TCreatureType m_monType2;
    TCreatureType m_monType3;
    TCreatureType m_monType4;
    short* m_available[4];
    hero* m_thisHero;
    // Dreamcast primitive pointer 0x474 and NH3API: int* at +0x80.
    int* m_availSource;
    long m_goldPerTroop;
    int m_altResource;
    int m_resourcesPerTroop;
    int m_inTownMainScreen;
    // Dreamcast pointer 0x4811 and NH3API: heroWindow* at +0x94.
    heroWindow* m_errorWin;
    armyGroup* m_currArmyGroup;
    unsigned char m_currArmyGroupIsTownGarrison;
    // Original: addIndex (Dreamcast recruitUnit +0xa0; NH3API agrees).
    // The former pad_a0 started at +0x9d, spanning alignment plus one
    // byte of this int. Natural alignment now places the full member at
    // +0xa0, between the retail +0x9c flag and +0xa4 updateNeeded.
    int m_addIndex;

    recruitUnit(armyGroup* newGroup, unsigned char groupIsTownGarrison,
        TCreatureType monType1, short* numMon1,
        TCreatureType monType2, short* numMon2,
        TCreatureType monType3, short* numMon3,
        TCreatureType monType4, short* numMon4);
    recruitUnit(hero* thisHero,
        TCreatureType monType1, short* numMon1,
        TCreatureType monType2, short* numMon2,
        TCreatureType monType3, short* numMon3,
        TCreatureType monType4, short* numMon4);
    int m_updateNeeded;
    int m_errorExit;
    int m_maxAvail;
    long m_totalGold;
    int m_totalResources;
    int m_numberToBuy;
    // What opened this recruit. The two dialog flavours above leave -1;
    // only the town constructor tags itself, and ::Close tests the tag
    // before it refreshes the town page behind the dialog. The 0x62
    // spelling is retail's own immediate, stored and compared; the name
    // is the house ordinal placeholder.
    enum ERecruitSource {
        RECRUIT_SOURCE_NONE = -1,
        RECRUIT_SOURCE_TOWN = 0x62
    };

    recruitUnit(town* newTown, int newDwellingIndex, int inInTownMainScreen);

    // The definition follows GetMonsterCost in recruit.cpp, as Dreamcast's
    // recruit.cpp line table attests.  It remains inline: retail emits no
    // standalone body and expands the nested GetMonsterCost loop at all four
    // recruitUnit call sites.
    inline void updateCost();
    // The three baseManager slots of vtable 0x640c70. Only Close is
    // reconstructed; the other two are declared so the class is
    // concrete, which is what `new recruitUnit(...)` at the fort page's
    // buy button and the refugee camp's frame-built one in events.obj
    // (0x4a4600) both need.
    virtual int open(int newPriority) OVERRIDE;      // slot 0, 0x54fea0
    virtual void close() OVERRIDE;                   // slot 1, 0x5502d0
    virtual int main(message& msg) OVERRIDE;         // slot 2, 0x550940

    void update(unsigned char newMonster, long slot);
    void setRolloverText(int codeY);
};
SIZE(recruitUnit, 188);

// --- globals ---
// CODEVIEW(E:\gamedcs\recruit.cpp:473, dc 0x119d64) TArtifact SiegeMonsterToSiegeArtifact(TCreatureType siegeMon);
// CODEVIEW(E:\gamedcs\recruit.cpp:693, dc 0x11a2f4) int ExitRecruitUnit(message* msg);

// --- TRecruitQuickWindow ---
// CODEVIEW(E:\gamedcs\recruit.cpp:1219, dc 0x11af24) void TRecruitQuickWindow::TRecruitQuickWindow(int x2, int y2);
// CODEVIEW(E:\gamedcs\recruit.cpp:1221, dc 0x11b570) void* TRecruitQuickWindow::`scalar deleting destructor'(unsigned __flags);

// --- TRecruitWindow ---
// CODEVIEW(E:\gamedcs\recruit.cpp:192, dc 0x118bb4) void TRecruitWindow::TRecruitWindow(int x2, int y2, int altResource, recruitUnit* recruit_info);
// CODEVIEW(E:\gamedcs\recruit.cpp:302, dc 0x119820) void TRecruitWindow::add_creature_widgets(long start_x, long start_y, long name_y, TCreatureType creature, long slot);
// CODEVIEW(E:\gamedcs\recruit.cpp:288, dc 0x11b53c) void* TRecruitWindow::`scalar deleting destructor'(unsigned __flags);

// --- recruitUnit ---
// CODEVIEW(E:\gamedcs\recruit.cpp:511, dc 0x119dcc) void recruitUnit::Update(unsigned char new_monster, long slot);
// CODEVIEW(E:\gamedcs\recruit.cpp:666, dc 0x11a280) void recruitUnit::SetRolloverText(int codeY);
// CODEVIEW(E:\gamedcs\recruit.cpp:704, dc 0x11a30c) int recruitUnit::Main(message* msg);
// CODEVIEW(E:\gamedcs\recruit.cpp:1082, dc 0x11ac7c) void recruitUnit::UpdateCost();
// CODEVIEW(E:\gamedcs\recruit.cpp:1120, dc 0x11ad04) void recruitUnit::recruitUnit(armyGroup* newGroup, unsigned char bGroupIsTownGarrison, TCreatureType _MonType1, short* _numMon1, TCreatureType _MonType2, short* _numMon2, TCreatureType _MonType3, short* _numMon3, TCreatureType _MonType4, short* _numMon4);
// CODEVIEW(E:\gamedcs\recruit.cpp:1158, dc 0x11adb4) void recruitUnit::recruitUnit(hero* _thisHero, TCreatureType _MonType1, short* _numMon1, TCreatureType _MonType2, short* _numMon2, TCreatureType _MonType3, short* _numMon3, TCreatureType _MonType4, short* _numMon4);

#endif  /* HOMM3_RECRUIT_H */
