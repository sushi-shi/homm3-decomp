#include <va.h>
#include <stdlib.h>
#include <stdarg.h>
#include "misc.h"
#include "kbwin.h"

// Thunk-form timeGetTime (rel32, same as mousemgr's CheckUpdate); the
// plain declaration and the per-TU import-form doctrine live in
// winmm_thunks.h. Kept AFTER the windows.h-bearing includes so the
// plain declaration downgrades mmsystem.h's dllimport for this TU.
#include "winmm_thunks.h"

// The CRT stream/memory surface, and the prefs block this TU owns.
// Both are reviewed headers rather than .cpp-local text: crt_stdio.h
// carries the rationale for declaring fopen/sprintf/memset by hand
// instead of including <stdio.h>, and prefs.h explains why the block's
// definition does not live in misc.h.
#include "crt_stdio.h"
#include "prefs.h"
#include "wingraph.h"

// The dialog FileSize raises when the open fails. Free /Gr row at
// retail 0x4f3a60, inside kb.obj's carve bracket and UNCLAIMED, so the
// name is a house ordinal placeholder (the town.h Unnamed526d20
// precedent). Body: sprintf's a text-table format against the incoming
// filename into a 500-byte frame buffer, then NormalDialog.
void unnamed4f3a60(char* filename);

// Use the timer during video playback so the game RNG sequence stays unchanged.
VA(0x0050b1d0, 0x54)  // dc 0xfd81c
int safeRandom(int min, int max)
{
    if (!g_videoPaused) {
        if (max == min)
            return max;
        if (max < min)
            return min;
        return min + rand() % (max - min + 1);
    }
    if (max == min)
        return max;
    if (max < min)
        return min;
    return min + static_cast<int>(timeGetTime()
                                  % static_cast<unsigned>(max - min + 1));
}

VA(0x0050b230, 0x28)  // dc 0xfd868
int random(int min, int max)
{
    if (min == max)
        return max;
    if (max < min)
        return min;
    return min + rand() % (max - min + 1);
}

// E:\gamedcs\misc.cpp:151
DC_ONLY(0xfd8a4, 0xB4)
void generateUniqueSystemID()
{
    long value;
    const char* characters =
        DATA_COMPGEN(0x0067fe68, uniqueSystemIDCharacters,
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

    memset(g_unnamed698758.m_name, 0, sizeof(g_unnamed698758.m_name));
    value = random(1, 999999) + GameTime::get();
    g_unnamed698758.m_name[2] = characters[value % 36];
    value += random(1, 999999) + GameTime::get();
    g_unnamed698758.m_name[1] = characters[value % 36];
    value += random(1, 999999) + GameTime::get();
    g_unnamed698758.m_name[0] = static_cast<char>(value % 26 + 'A');
}

// E:\gamedcs\misc.cpp:170
// BOUNDED 2026-09-06: the instruction counts are EQUAL (169 = 169; the four
// rows that look surplus on our side are the object's trailing NOP pad), the
// memory-reference multiset is equal, and the whole residual is the internal
// SCHEDULE of block 0.  Retail batches four `and reg,ecx` together and then
// four store/load pairs; this compile interleaves them.  Retail's first-load
// order is 0x1c 0x30 0x28 0x24 0x20 0x64 0x68 0x2c 0x40 0x44 0x3c 0x6c 0x18
// 0x38 0x48 0x4c FTT 0x8f 0x54 0x60 - which agrees with the source order
// below as far as combatAutoCreatures and then differs - but load order is
// NOT source order for VC6 here (ours starts 0x28 0x30 0x64 while the source
// starts 0x1c 0x30 0x28), and reordering the run to retail's load sequence
// (combatShadeLevel/autosave/blackoutComputer ahead of the two combat
// toggles) costs 99.1018 -> 92.9256 and is rejected.
// All 24 CFG blocks agree and 23 are instruction-exact. Retail's B0 has
// one redundant second `and eax, 1` on an already-masked value; repeating
// the source assignment is eliminated by VC6, so the canonical validation
// code deliberately carries this one-instruction optimizer residual.
VA(0x0050b260, 0x26C)  // body + sole retail caller, dc 0xfd958
void checkConfigFile()
{
    g_unnamed698758.m_showRoute &= 1;
    g_unnamed698758.m_animateSpellBook &= 1;
    g_unnamed698758.m_videoSubtitles &= 1;
    g_unnamed698758.m_quickCombat &= 1;
    g_unnamed698758.m_moveReminder &= 1;
    g_unnamed698758.m_showCombatGrid &= 1;
    // Retail masks animateSpellBook TWICE: it loads 0x298788 once, ANDs the
    // register at +0x8f and again at +0xa5, and stores once. VC6 folds the
    // repeat only when the two statements are adjacent - written here, seven
    // statements apart, both masks survive (98.5030 -> 99.1000).
    g_unnamed698758.m_animateSpellBook &= 1;
    g_unnamed698758.m_showCombatMouseHex &= 1;
    g_unnamed698758.m_townOutlines &= 1;
    g_unnamed698758.m_combatAutoSpells &= 1;
    g_unnamed698758.m_combatCatapult &= 1;
    g_unnamed698758.m_combatAutoCreatures &= 1;
    g_unnamed698758.m_combatBallista &= 1;
    g_unnamed698758.m_combatFirstAidTent &= 1;
    g_unnamed698758.m_autosave &= 1;
    g_unnamed698758.m_blackoutComputer &= 1;
    g_firstTimeThrough &= 1;
    g_unnamed698758.m_firstInstall &= 1;
    g_unnamed698758.m_mainGameShowMenu &= 1;
    g_unnamed698758.m_mainGameFullScreen &= 1;
    g_unnamed698758.m_combatShadeLevel &= 1;

    if (g_unnamed698758.m_combatArmyInfoLevel < 0 ||
            g_unnamed698758.m_combatArmyInfoLevel > 2)
        g_unnamed698758.m_combatArmyInfoLevel = 0;
    if (g_unnamed698758.m_combatSpeed < 0 ||
            g_unnamed698758.m_combatSpeed > 2)
        g_unnamed698758.m_combatSpeed = 0;
    if (g_unnamed698758.m_windowScrollSpeed < 0 ||
            g_unnamed698758.m_windowScrollSpeed > 2)
        g_unnamed698758.m_windowScrollSpeed = 1;
    if (g_unnamed698758.m_computerWalkSpeed < 2 ||
            g_unnamed698758.m_computerWalkSpeed > 5)
        g_unnamed698758.m_computerWalkSpeed = 3;
    if (g_unnamed698758.m_walkSpeed <= 0 ||
            g_unnamed698758.m_walkSpeed > 4)
        g_unnamed698758.m_walkSpeed = 2;
    if (g_unnamed698758.m_musicVolume < 0 ||
            g_unnamed698758.m_musicVolume > 9)
        g_unnamed698758.m_musicVolume = 5;
    if (g_unnamed698758.m_soundVolume < 0 ||
            g_unnamed698758.m_soundVolume > 9)
        g_unnamed698758.m_soundVolume = 5;

    g_unnamed698758.m_lastMusicVolume = g_unnamed698758.m_musicVolume;
    g_unnamed698758.m_lastSoundVolume = g_unnamed698758.m_soundVolume;
    if (strlen(g_unnamed698758.m_name) < 3)
        generateUniqueSystemID();
}

// E:\gamedcs\misc.cpp:352

// NOT promotable, checked 2026-08-13 against the same evidence that promoted
// its sibling: there is no retail body to promote. Source order would emit it
// between SetGameDefaults (0x50b4d0, ends 0x50b6f2) and
// SetDefaultCombatOptions (0x50b700), and that gap is fourteen bytes of
// padding; no other carve row in misc.obj's band fits eight dword stores
// either (the unclaimed rows are 95/95 B before the band starts and
// 221/32/489 B at its end). An EXTERN function is emitted out of line
// unconditionally under /Ob2, so the absence of a body is itself the
// evidence: retail's SetDefaultSystemOptions has internal linkage and its one
// call site inlined it away. Left non-static and DC_ONLY because the linkage
// change would buy no compared bytes - objdiff never scores this symbol - and
// would touch misc.h for nothing.
DC_ONLY(0xfdb78, 0x20)
void setDefaultSystemOptions()
{
    g_unnamed698758.m_showRoute = 1;
    g_unnamed698758.m_moveReminder = 1;
    g_unnamed698758.m_quickCombat = 0;
    g_unnamed698758.m_videoSubtitles = 1;
    g_unnamed698758.m_townOutlines = 1;
    g_unnamed698758.m_windowScrollSpeed = 1;
    g_unnamed698758.m_computerWalkSpeed = 3;
    g_unnamed698758.m_walkSpeed = 2;
}

// E:\gamedcs\misc.cpp:403
// STATIC-HELPER-AFTER-CALLER again: retail emits ReadPrefs first and
// ReadPrefsFromRegistry after it, the reverse of the DC source order.
// The rel32 edges settle the whole trio without any rank argument:
//     0x0050b750 -> call 0x0050b7b0,  jmp 0x0050be10
//     0x0050b7b0 -> call 0x0050b260,  call 0x0050be10
//     ?WritePrefs@@YIXXZ (0x50c1b0, claimed) -> jmp 0x0050be10
// so 0x0050be10 is WritePrefsToRegistry (the one body WritePrefs tail-
// jumps to), 0x0050b750 is ReadPrefs (zeroes the 0x35-dword prefs block
// at 0x698758, calls the registry reader, sprintf's the three key
// strings, then tail-jumps to the writer) and 0x0050b7b0 is
// ReadPrefsFromRegistry - 1623 B of real registry work against DC's
// 16-byte stub, which is exactly what a Dreamcast port would leave
// hollow.

// The prefs block type and the four sibling dwords are defined in
// include/prefs.h; their DEFINITIONS and DATA claims stay here, in
// the owning TU.
DATA(0x00698758)
SUnnamed698758 g_unnamed698758;

DATA(0x006985c4)
char g_regAppPath[351];
DATA(0x00698838)
char g_regCdRomPath[350];
DATA(0x006993c0)
int g_showIntro;

DATA(0x00699524)
int g_firstTimeThrough;
DATA(0x00699528)
int g_testDecomp;
DATA(0x0069952c)
int g_testRead;
DATA(0x00699530)
int g_testBlit;

// The registry key and value-name table. Retail LOADS each of these
// (`mov ecx, dword ptr [0x63ff80]`) rather than pushing a literal
// address, so they are pointer OBJECTS in .rdata, not folded literals -
// the load is what the `const char* const` spelling has to reproduce.
// They occupy 0x63ff74..0x640010 in the order below; the three reader-only
// slots are retained alongside the writer's table because retail emitted
// one contiguous pointer-object run for this TU.

DATA(0x0063ff74)
static const char* const g_prefRegKey =
    DATA_COMPGEN(0x0067fe28, prefsNameRegKey, "SOFTWARE\\New World Computing\\Heroes of Might and Magic\xae" " III\\1.0");
DATA(0x0063ff78)
static const char* const g_prefWalkSpeed =
    DATA_COMPGEN(0x0067fe1c, prefsNameWalkSpeed, "Walk Speed");
DATA(0x0063ff7c)
static const char* const g_prefComputerWalkSpeed =
    DATA_COMPGEN(0x0067fe08, prefsNameComputerWalkSpeed, "Computer Walk Speed");
DATA(0x0063ff80)
static const char* const g_prefMusicVolume =
    DATA_COMPGEN(0x0067fdf8, prefsNameMusicVolume, "Music Volume");
DATA(0x0063ff84)
static const char* const g_prefSoundVolume =
    DATA_COMPGEN(0x0067fde8, prefsNameSoundVolume, "Sound Volume");
DATA(0x0063ff88)
static const char* const g_prefAutosave =
    DATA_COMPGEN(0x0067fddc, prefsNameAutosave, "Autosave");
DATA(0x0063ff8c)
static const char* const g_prefShowRoute =
    DATA_COMPGEN(0x0067fdd0, prefsNameShowRoute, "Show Route");
DATA(0x0063ff90)
static const char* const g_prefMoveReminder =
    DATA_COMPGEN(0x0067fdc0, prefsNameMoveReminder, "Move Reminder");
DATA(0x0063ff94)
static const char* const g_prefQuickCombat =
    DATA_COMPGEN(0x0067fdb0, prefsNameQuickCombat, "Quick Combat");
DATA(0x0063ff98)
static const char* const g_prefVideoSubtitles =
    DATA_COMPGEN(0x0067fda0, prefsNameVideoSubtitles, "Video Subtitles");
DATA(0x0063ff9c)
static const char* const g_prefTownOutlines =
    DATA_COMPGEN(0x0067fd90, prefsNameTownOutlines, "Town Outlines");
DATA(0x0063ffa0)
static const char* const g_prefAnimateSpellBook =
    DATA_COMPGEN(0x0067fd7c, prefsNameAnimateSpellBook, "Animate SpellBook");
DATA(0x0063ffa4)
static const char* const g_prefWindowScrollSpeed =
    DATA_COMPGEN(0x0067fd68, prefsNameWindowScrollSpeed, "Window Scroll Speed");
DATA(0x0063ffa8)
static const char* const g_prefBlackoutComputer =
    DATA_COMPGEN(0x0067fd54, prefsNameBlackoutComputer, "Blackout Computer");
DATA(0x0063ffac)
static const char* const g_prefLastMusicVolume =
    DATA_COMPGEN(0x0067fd40, prefsNameLastMusicVolume, "Last Music Volume");
DATA(0x0063ffb0)
static const char* const g_prefLastSoundVolume =
    DATA_COMPGEN(0x0067fd2c, prefsNameLastSoundVolume, "Last Sound Volume");
DATA(0x0063ffb4)
static const char* const g_prefBinkVideo =
    DATA_COMPGEN(0x0067fd20, prefsNameBinkVideo, "Bink Video");
DATA(0x0063ffb8)
static const char* const g_prefFirstTime =
    DATA_COMPGEN(0x0067fd14, prefsNameFirstTime, "First Time");
DATA(0x0063ffbc)
static const char* const g_prefTestDecomp =
    DATA_COMPGEN(0x0067fd08, prefsNameTestDecomp, "Test Decomp");
DATA(0x0063ffc0)
static const char* const g_prefTestRead =
    DATA_COMPGEN(0x0067fcfc, prefsNameTestRead, "Test Read");
DATA(0x0063ffc4)
static const char* const g_prefTestBlit =
    DATA_COMPGEN(0x0067fcf0, prefsNameTestBlit, "Test Blit");
DATA(0x0063ffc8)
static const char* const g_prefShowCombatGrid =
    DATA_COMPGEN(0x0067fcdc, prefsNameShowCombatGrid, "Show Combat Grid");
DATA(0x0063ffcc)
static const char* const g_prefShowCombatMouseHex =
    DATA_COMPGEN(0x0067fcc4, prefsNameShowCombatMouseHex, "Show Combat Mouse Hex");
DATA(0x0063ffd0)
static const char* const g_prefCombatShadeLevel =
    DATA_COMPGEN(0x0067fcb0, prefsNameCombatShadeLevel, "Combat Shade Level");
DATA(0x0063ffd4)
static const char* const g_prefCombatArmyInfoLevel =
    DATA_COMPGEN(0x0067fc98, prefsNameCombatArmyInfoLevel, "Combat Army Info Level");
DATA(0x0063ffd8)
static const char* const g_prefCombatSpeed =
    DATA_COMPGEN(0x0067fc88, prefsNameCombatSpeed, "Combat Speed");
DATA(0x0063ffdc)
static const char* const g_prefCombatAutoCreatures =
    DATA_COMPGEN(0x0067fc70, prefsNameCombatAutoCreatures, "Combat Auto Creatures");
DATA(0x0063ffe0)
static const char* const g_prefCombatAutoSpells =
    DATA_COMPGEN(0x0067fc5c, prefsNameCombatAutoSpells, "Combat Auto Spells");
DATA(0x0063ffe4)
static const char* const g_prefCombatCatapult =
    DATA_COMPGEN(0x0067fc4c, prefsNameCombatCatapult, "Combat Catapult");
DATA(0x0063ffe8)
static const char* const g_prefCombatBallista =
    DATA_COMPGEN(0x0067fc3c, prefsNameCombatBallista, "Combat Ballista");
DATA(0x0063ffec)
static const char* const g_prefCombatFirstAidTent =
    DATA_COMPGEN(0x0067fc24, prefsNameCombatFirstAidTent, "Combat First Aid Tent");
DATA(0x0063fff0)
static const char* const g_prefNetworkDefaultName =
    DATA_COMPGEN(0x0067fc0c, prefsNameNetworkDefaultName, "Network Default Name");
DATA(0x0063fff4)
static const char* const g_prefMainGameShowMenu =
    DATA_COMPGEN(0x0067fbf8, prefsNameMainGameShowMenu, "Main Game Show Menu");
DATA(0x0063fff8)
static const char* const g_prefMainGameX =
    DATA_COMPGEN(0x0067fbec, prefsNameMainGameX, "Main Game X");
DATA(0x0063fffc)
static const char* const g_prefMainGameY =
    DATA_COMPGEN(0x0067fbe0, prefsNameMainGameY, "Main Game Y");
DATA(0x00640000)
static const char* const g_prefMainGameFullScreen =
    DATA_COMPGEN(0x0067fbc8, prefsNameMainGameFullScreen, "Main Game Full Screen");
DATA(0x00640004)
static const char* const g_prefAppPath =
    DATA_COMPGEN(0x0067fbc0, prefsNameAppPath, "AppPath");
DATA(0x00640008)
static const char* const g_prefCdDrive =
    DATA_COMPGEN(0x0067fbb8, prefsNameCDDrive, "CDDrive");
DATA(0x0064000c)
static const char* const g_prefUniqueSystemId =
    DATA_COMPGEN(0x0067fba4, prefsNameUniqueSystemID, "Unique System ID");
DATA(0x00640010)
static const char* const g_prefShowIntro =
    DATA_COMPGEN(0x0067fb98, prefsNameShowIntro, "Show Intro");

VA(0x0050b4d0, 0x222)  // dc 0xfda8c
void setGameDefaults()
{
    g_unnamed698758.m_musicVolume = 5;
    g_unnamed698758.m_lastMusicVolume = 5;
    g_unnamed698758.m_soundVolume = 5;
    g_unnamed698758.m_lastSoundVolume = 5;
    g_unnamed698758.m_mainGameX = 10;
    g_unnamed698758.m_mainGameY = 10;
    setDefaultSystemOptions();
    setDefaultCombatOptions();
    g_unnamed698758.m_autosave = 1;
    g_unnamed698758.m_blackoutComputer = 0;
    g_unnamed698758.m_mainGameShowMenu = 1;
    g_unnamed698758.m_mainGameFullScreen = 1;
    g_firstTimeThrough = 1;
    strcpy(g_unnamed698758.m_networkDefaultName, "Player");
    generateUniqueSystemID();
    g_unnamed698758.m_firstInstall = 0;

    _getcwd(g_regAppPath, sizeof(g_regAppPath));
    strcat(g_regAppPath,
        DATA_COMPGEN(0x00677dac, prefsPathSeparator, "\\"));

    HKEY key = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, g_prefRegKey, 0,
            KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) {
        RegSetValueExA(key, g_prefAppPath, 0, REG_SZ,
            static_cast<const BYTE*>(static_cast<const void*>(
                g_regAppPath)), strlen(g_regAppPath));
        RegCloseKey(key);
    }
}

VA(0x0050b700, 0x44)  // dc 0xfdb98
void setDefaultCombatOptions()
{
    g_unnamed698758.m_animateSpellBook = 1;
    g_unnamed698758.m_showCombatGrid = 0;
    g_unnamed698758.m_showCombatMouseHex = 0;
    g_unnamed698758.m_combatShadeLevel = 0;
    g_unnamed698758.m_combatArmyInfoLevel = 0;
    g_unnamed698758.m_combatSpeed = 0;
    g_unnamed698758.m_combatAutoCreatures = 1;
    g_unnamed698758.m_combatAutoSpells = 1;
    g_unnamed698758.m_combatCatapult = 1;
    g_unnamed698758.m_combatBallista = 1;
    g_unnamed698758.m_combatFirstAidTent = 1;
}

VA(0x0050b750, 0x59)  // dc 0xfdbd0
void readPrefs()
{
    memset(&g_unnamed698758, 0, sizeof(g_unnamed698758));
    readPrefsFromRegistry();
    sprintf(g_unnamed698758.m_rcFile,
        DATA_COMPGEN(0x0067fea8, readPrefsRcFileFormat, "RMT%sRC.BIN"),
        g_unnamed698758.m_name);
    sprintf(g_unnamed698758.m_rdFile,
        DATA_COMPGEN(0x0067fe9c, readPrefsRdFileFormat, "RMT%sRD.BIN"),
        g_unnamed698758.m_name);
    sprintf(g_unnamed698758.m_scFile,
        DATA_COMPGEN(0x0067fe90, readPrefsScFileFormat, "RMT%sSC.BIN"),
        g_unnamed698758.m_name);
    writePrefsToRegistry();
}

// E:\gamedcs\misc.cpp:525
// Retail's query order is deliberately kept distinct from the writer's
// order. One shared cbData is reused without being reset after every
// call; only the three differently-sized values reset it. The first music
// query is a probe, so it appears once in the guard and again in the normal
// read run. Canonical source ends with strcpy(CDDrive, AppPath). Retail was
// binary-patched there to a four-byte assignment plus a jump over 17 NOPs,
// leaving the final five instructions of the old inline strcpy unreachable.
// Residual (95.9319%): the canonical strcpy still emits its full inline
// length scan and copy; retail instead executes the four-byte assignment
// and jumps over seventeen NOPs plus the stranded copy tail. This is an
// observed binary-patch boundary, not a source-level missing branch.
// A direct four-byte C assignment control scored 91.136% and did not emit
// the unreachable tail. Preserve the meaningful strcpy and the retail
// evidence; do not manufacture dead code to imitate the patch bytes.
VA(0x0050b7b0, 0x657)  // anchor-callgraph (called by ReadPrefs), dc 0xfdbc0
void readPrefsFromRegistry()
{
    DWORD cbData;
    HKEY key;
    DWORD type;
    DWORD showIntro;
    DWORD showIntroSize;
    char appPath[260];

    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, g_prefRegKey, &key)
            == ERROR_SUCCESS) {
        const char* showIntroName = g_prefShowIntro;
        HKEY showIntroKey = key;
        showIntroSize = 4;
        if (RegQueryValueExA(showIntroKey, showIntroName, 0, 0,
                static_cast<BYTE*>(static_cast<void*>(&showIntro)),
                &showIntroSize) != ERROR_SUCCESS) {
            showIntro = 1;
            RegSetValueExA(showIntroKey, showIntroName, 0, REG_DWORD,
                static_cast<const BYTE*>(static_cast<const void*>(
                    &showIntro)), 4);
        }
        g_showIntro = showIntro;

        cbData = 4;
        if (RegQueryValueExA(key, g_prefMusicVolume, 0, &type,
                static_cast<BYTE*>(static_cast<void*>(
                    &g_unnamed698758.m_musicVolume)), &cbData)
                != ERROR_SUCCESS) {
            memset(&g_unnamed698758, 0, sizeof(g_unnamed698758));
            setGameDefaults();
            RegCloseKey(key);
            writePrefsToRegistry();
            return;
        }

        RegQueryValueExA(key, g_prefMusicVolume, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_musicVolume)), &cbData);
        RegQueryValueExA(key, g_prefSoundVolume, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_soundVolume)), &cbData);
        RegQueryValueExA(key, g_prefLastMusicVolume, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_lastMusicVolume)), &cbData);
        RegQueryValueExA(key, g_prefLastSoundVolume, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_lastSoundVolume)), &cbData);
        RegQueryValueExA(key, g_prefWalkSpeed, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_walkSpeed)), &cbData);
        RegQueryValueExA(key, g_prefComputerWalkSpeed, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_computerWalkSpeed)), &cbData);
        RegQueryValueExA(key, g_prefShowRoute, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_showRoute)), &cbData);
        RegQueryValueExA(key, g_prefMoveReminder, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_moveReminder)), &cbData);
        RegQueryValueExA(key, g_prefQuickCombat, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_quickCombat)), &cbData);
        RegQueryValueExA(key, g_prefVideoSubtitles, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_videoSubtitles)), &cbData);
        RegQueryValueExA(key, g_prefTownOutlines, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_townOutlines)), &cbData);
        RegQueryValueExA(key, g_prefAnimateSpellBook, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_animateSpellBook)), &cbData);
        RegQueryValueExA(key, g_prefWindowScrollSpeed, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_windowScrollSpeed)), &cbData);
        RegQueryValueExA(key, g_prefBlackoutComputer, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_blackoutComputer)), &cbData);
        RegQueryValueExA(key, g_prefFirstTime, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_firstTimeThrough)), &cbData);
        RegQueryValueExA(key, g_prefTestDecomp, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_testDecomp)), &cbData);
        RegQueryValueExA(key, g_prefTestRead, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_testRead)), &cbData);
        RegQueryValueExA(key, g_prefTestBlit, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_testBlit)), &cbData);
        RegQueryValueExA(key, g_prefBinkVideo, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_binkVideo)), &cbData);

        cbData = 4;
        RegQueryValueExA(key, g_prefUniqueSystemId, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                g_unnamed698758.m_name)), &cbData);
        g_unnamed698758.m_name[3] = 0;
        cbData = 31;
        RegQueryValueExA(key, g_prefNetworkDefaultName, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                g_unnamed698758.m_networkDefaultName)), &cbData);
        cbData = 4;
        RegQueryValueExA(key, g_prefAutosave, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_autosave)), &cbData);
        RegQueryValueExA(key, g_prefShowCombatGrid, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_showCombatGrid)), &cbData);
        RegQueryValueExA(key, g_prefShowCombatMouseHex, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_showCombatMouseHex)), &cbData);
        RegQueryValueExA(key, g_prefCombatShadeLevel, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatShadeLevel)), &cbData);
        RegQueryValueExA(key, g_prefCombatArmyInfoLevel, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatArmyInfoLevel)), &cbData);
        RegQueryValueExA(key, g_prefCombatAutoCreatures, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatAutoCreatures)), &cbData);
        RegQueryValueExA(key, g_prefCombatAutoSpells, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatAutoSpells)), &cbData);
        RegQueryValueExA(key, g_prefCombatCatapult, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatCatapult)), &cbData);
        RegQueryValueExA(key, g_prefCombatBallista, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatBallista)), &cbData);
        RegQueryValueExA(key, g_prefCombatFirstAidTent, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatFirstAidTent)), &cbData);
        RegQueryValueExA(key, g_prefCombatSpeed, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_combatSpeed)), &cbData);
        RegQueryValueExA(key, g_prefMainGameShowMenu, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_mainGameShowMenu)), &cbData);
        RegQueryValueExA(key, g_prefMainGameX, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_mainGameX)), &cbData);
        RegQueryValueExA(key, g_prefMainGameY, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_mainGameY)), &cbData);
        RegQueryValueExA(key, g_prefMainGameFullScreen, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(
                &g_unnamed698758.m_mainGameFullScreen)), &cbData);

        cbData = 350;
        _getcwd(appPath, sizeof(appPath));
        strcat(appPath, "\\");
        if (RegQueryValueExA(key, g_prefAppPath, 0, &type,
                static_cast<BYTE*>(static_cast<void*>(g_regAppPath)),
                &cbData)
                != ERROR_SUCCESS ||
                _strcmpi(g_regAppPath, appPath) != 0) {
            strcpy(g_regAppPath, appPath);
            RegSetValueExA(key, g_prefAppPath, 0, REG_SZ,
                static_cast<const BYTE*>(static_cast<const void*>(
                    g_regAppPath)),
                strlen(g_regAppPath));
        }

        cbData = 350;
        RegQueryValueExA(key, g_prefCdDrive, 0, &type,
            static_cast<BYTE*>(static_cast<void*>(g_regCdRomPath)),
            &cbData);
        strcpy(g_regCdRomPath, g_regAppPath);
        RegCloseKey(key);

        if (g_unnamed698758.m_mainGameX > getDesktopWidth() - 800)
            g_unnamed698758.m_mainGameX = getDesktopWidth() - 800;
        if (g_unnamed698758.m_mainGameY > getDesktopHeight() - 600)
            g_unnamed698758.m_mainGameY = getDesktopHeight() - 600;
        if (g_unnamed698758.m_mainGameX < 0)
            g_unnamed698758.m_mainGameX = 0;
        if (g_unnamed698758.m_mainGameY < 0)
            g_unnamed698758.m_mainGameY = 0;

    }
    checkConfigFile();
}

VA(0x0050be10, 0x399)  // dc 0xfdc4c
void writePrefsToRegistry()
{
    HKEY key = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, g_prefRegKey, 0, KEY_ALL_ACCESS,
            &key) == ERROR_SUCCESS) {
        RegSetValueExA(key, g_prefMusicVolume, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_musicVolume)), 4);
        RegSetValueExA(key, g_prefSoundVolume, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_soundVolume)), 4);
        RegSetValueExA(key, g_prefLastMusicVolume, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_lastMusicVolume)), 4);
        RegSetValueExA(key, g_prefLastSoundVolume, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_lastSoundVolume)), 4);
        RegSetValueExA(key, g_prefWalkSpeed, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_walkSpeed)), 4);
        RegSetValueExA(key, g_prefComputerWalkSpeed, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_computerWalkSpeed)), 4);
        RegSetValueExA(key, g_prefShowRoute, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_showRoute)), 4);
        RegSetValueExA(key, g_prefMoveReminder, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_moveReminder)), 4);
        RegSetValueExA(key, g_prefQuickCombat, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_quickCombat)), 4);
        RegSetValueExA(key, g_prefVideoSubtitles, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_videoSubtitles)), 4);
        RegSetValueExA(key, g_prefTownOutlines, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_townOutlines)), 4);
        RegSetValueExA(key, g_prefAnimateSpellBook, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_animateSpellBook)), 4);
        RegSetValueExA(key, g_prefWindowScrollSpeed, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_windowScrollSpeed)), 4);
        RegSetValueExA(key, g_prefBinkVideo, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_binkVideo)), 4);
        RegSetValueExA(key, g_prefBlackoutComputer, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_blackoutComputer)), 4);
        RegSetValueExA(key, g_prefFirstTime, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_firstTimeThrough)), 4);
        RegSetValueExA(key, g_prefTestDecomp, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_testDecomp)), 4);
        RegSetValueExA(key, g_prefTestRead, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_testRead)), 4);
        RegSetValueExA(key, g_prefTestBlit, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_testBlit)), 4);
        RegSetValueExA(key, g_prefUniqueSystemId, 0, REG_SZ,
            static_cast<const BYTE*>(static_cast<const void*>(
                g_unnamed698758.m_name)), 4);
        RegSetValueExA(key, g_prefNetworkDefaultName, 0, REG_SZ,
            static_cast<const BYTE*>(static_cast<const void*>(
                g_unnamed698758.m_networkDefaultName)), 21);
        RegSetValueExA(key, g_prefAutosave, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_autosave)), 4);
        RegSetValueExA(key, g_prefShowCombatGrid, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_showCombatGrid)), 4);
        RegSetValueExA(key, g_prefShowCombatMouseHex, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_showCombatMouseHex)), 4);
        RegSetValueExA(key, g_prefCombatShadeLevel, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatShadeLevel)), 4);
        RegSetValueExA(key, g_prefCombatArmyInfoLevel, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatArmyInfoLevel)), 4);
        RegSetValueExA(key, g_prefCombatAutoCreatures, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatAutoCreatures)), 4);
        RegSetValueExA(key, g_prefCombatAutoSpells, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatAutoSpells)), 4);
        RegSetValueExA(key, g_prefCombatCatapult, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatCatapult)), 4);
        RegSetValueExA(key, g_prefCombatBallista, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatBallista)), 4);
        RegSetValueExA(key, g_prefCombatFirstAidTent, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatFirstAidTent)), 4);
        RegSetValueExA(key, g_prefCombatSpeed, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_combatSpeed)), 4);
        RegSetValueExA(key, g_prefMainGameShowMenu, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_mainGameShowMenu)), 4);
        RegSetValueExA(key, g_prefMainGameX, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_mainGameX)), 4);
        RegSetValueExA(key, g_prefMainGameY, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_mainGameY)), 4);
        RegSetValueExA(key, g_prefMainGameFullScreen, 0, REG_DWORD,
            static_cast<const BYTE*>(static_cast<const void*>(
                &g_unnamed698758.m_mainGameFullScreen)), 4);
        RegCloseKey(key);
    }
}

VA(0x0050c1b0, 0x5)  // dc 0xfe050
void writePrefs()
{
    writePrefsToRegistry();
}

VA(0x0050c1c0, 0x6)  // dc 0xfe064
int setupCDDrive()
{
    return 7;
}

#if 0  // @carcass

// E:\gamedcs\misc.cpp:603
DC_ONLY(0xfe060, 0x4)
int IsCDDrive(int drive)
{
    // @stub
}

#endif  // @carcass

VA(0x0050c5a0, 0x49)  // dc 0xfe068
long fileSize(char* filename)
{
    FILE* stream = fopen(filename, DATA_COMPGEN(0x0067ff20, fileSizeOpenMode, "r+b"));
    if (!stream)
        unnamed4f3a60(filename);
    fseek(stream, 0, SEEK_END);
    long size = ftell(stream);
    fseek(stream, 0, SEEK_SET);
    fclose(stream);
    return size;
}

// Retail's only three references to this scratch are format_string's
// vsprintf destination, strlen source and copy source.  Its 512-byte extent
// is bounded exactly by the next referenced bss cell at 0x6998cc.
DATA(0x006996cc)
static char g_formatStringBuffer[512];

// The seed SRand records before handing it to the CRT. Retail .data
// 0x67fb94, and the store below is its ONLY reference in the whole
// image (one row in config/retail-reloc-evidence.tsv), so nothing
// attests a name or a linkage - house ordinal placeholder, filed
// static in the one TU that touches it.
DATA(0x0067fb94)
static int g_unnamed67fb94;

VA(0x0050c5f0, 0xE)  // dc 0xfe0b8
void sRand(int seed)
{
    g_unnamed67fb94 = seed;
    srand(seed);
}

// CodeView proves both degenerate-range returns, then rand at line 805 and
// the inclusive remainder at 806. Restore this ordinary source body instead
// of three caller-local adapters to Random. Retail callers reach 0x50b230,
// whose guards/rand/remainder also implement Random; the identical-body
// comparison is checked separately from ownership. No second RVA is claimed.
// VC6 verification: `sema compare 0x50b230 --unit misc --symbol
// ?sRandom@@YIHHH@Z --no-build --why-bytes` agrees in every view, including
// the rand relocation. The two source bodies can therefore share retail's
// retained code without replacing SRandom's implementation with an adapter.
// E:\gamedcs\misc.cpp:796, dc 0xfe0d0
int sRandom(int lower, int upper)
{
    if (lower == upper)
        return upper;
    if (upper < lower)
        return lower;
    int value = rand();
    return lower + value % (upper - lower + 1);
}

VA(0x0050c600, 0xDD)  // dc 0xfe10c
std::string formatString(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsprintf(g_formatStringBuffer, format, arguments);
    va_end(arguments);
    return std::string(g_formatStringBuffer);
}

VA(0x0050c6e0, 0x55)  // dc 0xfe150
TPickANumber::TPickANumber(int lowBound, int high)
    : m_low(lowBound),
      m_numbersLeft(high - lowBound + 1),
      m_available(m_numbersLeft, 1)
{
}

// E:\gamedcs\misc.cpp:849.
VA(0x0050c740, 0x52)  // dc 0xfe190
int TPickANumber::pick()
{
    if (m_numbersLeft <= 0)
        return m_low - 1;
    int m = m_numbersLeft - 1;
    int skip;
    if (m == 0)
        skip = 0;
    else if (m < 0)
        skip = 0;
    else
        skip = rand() % (m + 1);
    int idx = 0;
    for (;;) {
        if (m_available[idx]) {
            if (skip == 0)
                break;
            skip--;
        }
        idx++;
    }
    m_numbersLeft--;
    m_available[idx] = 0;
    return m_low + idx;
}

VA(0x0050c7a0, 0x42)  // dc 0xfe248
unsigned long getAvailableDiskSpace()
{
    unsigned long sectorsPerCluster = 0;
    unsigned long bytesPerSector = 0;
    unsigned long numberOfFreeClusters = 0;
    unsigned long totalNumberOfClusters = 0;

    if (!GetDiskFreeSpaceA(0, &sectorsPerCluster, &bytesPerSector,
                           &numberOfFreeClusters, &totalNumberOfClusters))
        return 0;
    return numberOfFreeClusters * bytesPerSector * sectorsPerCluster;
}

#if 0  // @carcass

// E:\gamedcs\misc.cpp:884
DC_ONLY(0xfe208, 0x40)
void TPickANumber::MarkOut(int number)
{
    // @stub
}

#endif  // @carcass
