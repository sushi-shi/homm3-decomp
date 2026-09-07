// prefs.h - the preferences block misc.obj reads from and writes to the
// registry (retail .bss 0x698758).
// HAND-OWNED after admission.
//
// This is a SEPARATE header rather than a section of misc.h on purpose.
// misc.h is included by soundmgr.cpp, game.cpp, kbwin.cpp and
// ai_combat.cpp as well as misc.cpp; a struct DEFINITION added there
// would enter four more TUs' include closures, and the include-set
// sensitivity class (initialize_game_data precedent, re-measured
// 2026-08-08) makes that a real codegen risk for bodies that are
// already exact. Keeping the definition here, included only by the one
// TU that owns the block, gives the type its home in include/ with the
// same closure misc.obj had when its bodies were matched.
#ifndef HOMM3_PREFS_H
#define HOMM3_PREFS_H

#include <va.h>

// Extent is byte-proven by ReadPrefs' own `mov ecx,0x35 / xor eax,eax /
// rep stosd` at 0x698758: 53 dwords = 212 bytes, so
// 0x698758..0x69882b inclusive is ONE object, and every interior
// address misc.obj touches lands inside it - which is what makes them
// MEMBERS and not neighbours.
//
// The FIELD NAMES below are not guesses: WritePrefsToRegistry hands
// each one to RegSetValueExA next to the registry value name it is
// stored under, so the name/offset pairing is read straight off the
// image. The slicing closes EXACTLY - the last field ends at +0xd4 =
// 212 - which is the arithmetic check that no field was invented:
//   ...+0x70 the dword run, +0x74..+0x8f untouched by this TU,
//   +0x90 name[4] ("Unique System ID", written REG_SZ with cbData 4),
//   +0x94 combatSpeed, +0x98/+0xa5/+0xb2 the three 13-byte RMT keys,
//   +0xbf networkDefaultName[21] ("Network Default Name", cbData 0x15).
// The earlier reading of name as char[8] was wrong: 'Combat Speed'
// sits at +0x94, inside that span, and pins name at four bytes - which
// is also what makes the 13-byte RMT buffers exactly big enough for
// "RMT" + a 3-char name + "RC.BIN" + NUL.
// Dreamcast configStruct supplies the legacy driver-name/Redbook fields
// at +0x74..+0x8e. Retail fixes firstInstall at +0x8f and the same total
// extent, corroborating this retained layout. NH3API instead widens the
// preceding scalar into an array and puts firstInstall two bytes early.
struct SUnnamed698758 {
    // Before normalization: computerWalkSpeed.
    int m_computerWalkSpeed;        // +0x00  "Computer Walk Speed"
    // Before normalization: walkSpeed.
    int m_walkSpeed;                // +0x04  "Walk Speed"
    // Before normalization: musicVolume.
    int m_musicVolume;              // +0x08  "Music Volume"
    // Before normalization: soundVolume.
    int m_soundVolume;              // +0x0c  "Sound Volume"
    // Before normalization: lastMusicVolume.
    int m_lastMusicVolume;          // +0x10  "Last Music Volume"
    // Before normalization: lastSoundVolume.
    int m_lastSoundVolume;          // +0x14  "Last Sound Volume"
    // Before normalization: autosave.
    int m_autosave;                 // +0x18  "Autosave"
    // Before normalization: showRoute.
    int m_showRoute;                // +0x1c  "Show Route"
    // Before normalization: moveReminder.
    int m_moveReminder;             // +0x20  "Move Reminder"
    // Before normalization: quickCombat.
    int m_quickCombat;              // +0x24  "Quick Combat"
    // Before normalization: videoSubtitles.
    int m_videoSubtitles;           // +0x28  "Video Subtitles"
    // Before normalization: townOutlines.
    int m_townOutlines;             // +0x2c  "Town Outlines"
    // Before normalization: animateSpellBook.
    int m_animateSpellBook;         // +0x30  "Animate SpellBook"
    // Before normalization: windowScrollSpeed.
    int m_windowScrollSpeed;        // +0x34  "Window Scroll Speed"
    // Before normalization: blackoutComputer.
    int m_blackoutComputer;         // +0x38  "Blackout Computer"
    // Before normalization: combatAutoCreatures.
    int m_combatAutoCreatures;      // +0x3c  "Combat Auto Creatures"
    // Before normalization: combatAutoSpells.
    int m_combatAutoSpells;         // +0x40  "Combat Auto Spells"
    // Before normalization: combatCatapult.
    int m_combatCatapult;           // +0x44  "Combat Catapult"
    // Before normalization: combatBallista.
    int m_combatBallista;           // +0x48  "Combat Ballista"
    // Before normalization: combatFirstAidTent.
    int m_combatFirstAidTent;       // +0x4c  "Combat First Aid Tent"
    // Before normalization: binkVideo.
    int m_binkVideo;                // +0x50  "Bink Video"
    // Before normalization: mainGameShowMenu.
    int m_mainGameShowMenu;         // +0x54  "Main Game Show Menu"
    // Before normalization: mainGameX.
    int m_mainGameX;                // +0x58  "Main Game X"
    // Before normalization: mainGameY.
    int m_mainGameY;                // +0x5c  "Main Game Y"
    // Before normalization: mainGameFullScreen.
    int m_mainGameFullScreen;       // +0x60  "Main Game Full Screen"
    // Before normalization: showCombatGrid.
    int m_showCombatGrid;           // +0x64  "Show Combat Grid"
    // Before normalization: showCombatMouseHex.
    int m_showCombatMouseHex;       // +0x68  "Show Combat Mouse Hex"
    // Before normalization: combatShadeLevel.
    int m_combatShadeLevel;         // +0x6c  "Combat Shade Level"
    // Before normalization: combatArmyInfoLevel.
    int m_combatArmyInfoLevel;      // +0x70  "Combat Army Info Level"
    // Former pad74: Dreamcast configStruct::cDOSDigitalDriver and
    // cDOSMIDIDriver are consecutive 13-byte names; bDontTryRedbook is
    // the byte at +0x8e. These legacy slots are not used by retail misc.
    char m_dosDigitalDriver[13];    // +0x74
    char m_dosMidiDriver[13];       // +0x81
    char m_dontTryRedbook;          // +0x8e
    // Before normalization: unnamed8f; reference member configStruct::bFirstInstall.
    unsigned char m_firstInstall;      // +0x8f  boolean checked/defaulted here
    // Before normalization: name.
    char m_name[4];                 // +0x90  "Unique System ID"
    // Before normalization: combatSpeed.
    int m_combatSpeed;              // +0x94  "Combat Speed"
    // Before normalization: rcFile.
    char m_rcFile[13];              // +0x98  "RMT%sRC.BIN" destination
    // Before normalization: rdFile.
    char m_rdFile[13];              // +0xa5  "RMT%sRD.BIN" destination
    // Before normalization: scFile.
    char m_scFile[13];              // +0xb2  "RMT%sSC.BIN" destination
    // Before normalization: networkDefaultName.
    char m_networkDefaultName[21];  // +0xbf  "Network Default Name"
};
SIZE(SUnnamed698758, 212);

// Definition + DATA claim in src/misc.cpp.
// Before normalization: gUnnamed698758.
extern SUnnamed698758 g_unnamed698758;

// Four dwords at 0x699524..0x699530, OUTSIDE the prefs block (it ends
// at 0x69882b), so they are separate globals and not members. Their names are
// published by Dreamcast CodeView and their roles/addresses are independently
// confirmed by the retail preference I/O and oldmain benchmark block.
// Definitions + DATA claims in src/misc.cpp.
// Retail .bss 0x698780, the subtitle toggle. It sits in the same prefs
// personality as the four dwords below - the registry reader/writer at
// 0x50b27d/0x50b294/0x50b536 and the two address-taken checkbox bindings at
// 0x50b983/0x50bf27 are five of its ten sites - and the campaign prologue
// player reads it to decide whether the subtitle strip is built and
// scrolled. Name is a ROLE invention; no DEFINITION or DATA claim yet,
// because misc.obj's own span has not been carved that far.
// Before normalization: gbShowSubtitles.
extern int g_showSubtitles;     // 0x698780

// Before normalization: gbFirstTimeThrough.
extern int g_firstTimeThrough;  // 0x699524, "First Time"
// Before normalization: giTestDecomp.
extern int g_testDecomp;        // 0x699528, "Test Decomp"
// Before normalization: giTestRead.
extern int g_testRead;          // 0x69952c, "Test Read"
// Before normalization: giTestBlit.
extern int g_testBlit;          // 0x699530, "Test Blit"

#endif  /* HOMM3_PREFS_H */
