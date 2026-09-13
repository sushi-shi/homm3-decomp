// viewarmywindow.cpp - E:\gamedcs\viewarmywindow.cpp (compiland viewarmywindow.obj)
#include <va.h>
#include <stdio.h>
#include "viewarmywindow.h"
#include "border.h"
#include "button.h"
#include "exec.h"
#include "widget.h"
#include "game.h"
#include "kb.h"
#include "kbwin.h"
#include "textwdgt.h"
#include "army.h"
#include "armygrp.h"
#include "cmbtmgr.h"
#include "hero.h"
#include "iconwdgt.h"
#include "recruit.h"
#include "message.h"
#include "mousemgr.h"
#include "winmgr.h"
#include "creaturetype.h"
#include "soundmgr.h"
#include "misc.h"

// Dinkumware-era max source shape used by retail: arguments are copied into
// homes and the selected home is returned by reference.
template <class T>
inline const T& cppMax(T left, T right)
{
    return left < right ? right : left;
}

// Integer-input adapter, not an original union claim. DC proves the bare
// constructor's army_type parameter and Upgrade are int, whereas ArmyType
// and army::creatureType are TCreatureType. Keep the actual input boundaries;
// displayed member values no longer need local representation bridges.
inline TCreatureType creatureTypeFromInt(int value)
{
    union {
        int m_value;
        TCreatureType m_creature;
    } storage;
    storage.m_value = value;
    return storage.m_creature;
}

// The Faerie Dragon's in-combat cast button handler, retail 0x5f5030:
// 48 bytes sitting in this TU's own band that
// config/retail-functions.tsv has NO row for (the 0x5f4850 row's 2007
// bytes end at 0x5f5027 and the next row is 0x5f5060 - a carve defect,
// reported not fixed). The body is
// `if (msg->codeX == 13 && !(msg->qualifier & 0x200)) { msg->id =
// MESSAGE_WIDGET; msg->codeX = 10; msg->codeY = OK_ID; return
// MESSAGE_DISPATCH_FORWARD; } return 0;`. DECLARED ONLY - defining it
// would put an unpaired function in this object.
int viewArmyCastSpellHandler(message& msg);

// The shooter bit, byte-proven by initialize_creatures and consumed here
// by create_shots_widget's single `test byte [traits+0x10], 4` guard.
// TU-local for the reason ai_combat.cpp keeps its own copy: the shared
// header stays as small as its own consumers need.
const unsigned int g_ctaShooter = 0x4;

// The Dendroid's hold used to be a TU-local `const int SPELL_BIND` here,
// byte-proven by the folded `akSpellTraits + 0x2650` row address. It is
// armygrp.h's own ungated ESpellId enumerator as of 2026-08-20 - the
// local copy became a hard C2373 - and the two spell it in the same
// place with the same value, so the substitution is byte-inert.

// The four BASE elementals, tested behind the version gate everywhere the
// game asks whether a creature has an alignment at all - eight times in
// armygrp.cpp, four in game.cpp, once each in cmbtmgr.cpp and
// quickherowindow.cpp, and TWICE in this file. TU-local for the reason
// CTA_SHOOTER and SPELL_BIND above are: the shared header stays as small as
// its own consumers need.

// This is a CODEGEN construct as much as a spelling, and the constructor at
// 0x5f4210 is where that was proven: it expands to exactly the four
// `cmp eax,0x7N / je` compares the longhand chain gives (both call sites in
// this file are byte-flat under the substitution), while costing the /Ob2
// allowance one more candidate site in each caller - which is what stops
// the SECOND std::string member's `_Tidy` from being expanded there. See
// that constructor's note.
inline bool isBaseElemental(int type)
{
    return type == CREATURE_AIR_ELEMENTAL || type == CREATURE_EARTH_ELEMENTAL
        || type == CREATURE_FIRE_ELEMENTAL || type == CREATURE_WATER_ELEMENTAL;
}

// The two rows of convertID2HelpID's compact 0..15 domain that
// WindowHandler builds text for instead of reading HELP.TXT.
const int g_moraleHelpIndex = 9;
const int g_luckHelpIndex = 10;

// The popup's own help roster, filled from HELP.TXT by text.obj's
// spreadsheet loader (0x5b98b0), which walks 0x6a7458 in stride-8 pairs
// up to 0x6a74d4 - i.e. fifteen THelpText rows. convertID2HelpID's
// 0..15 range is what indexes it: [].text is the bottom strip's line
// and [].rclick the right-click dialog body. The extent is spelled 16
// because the id map can produce 15; the loader stops one row short.
DATA(0x006a7458) extern THelpText g_viewArmyHelp[16];

// Two ARRAYTXT.TXT runs text.obj's second loader (0x5b9cc0) fills with
// stride-4 char pointers: 42 rows from 0x6a57bc and 25 from 0x6a532c.
// Rows 0/1/2 are the positive/neutral/negative descriptor and row 3 is
// the carrier format both describers hand to format_string; the high
// row is the "nothing is modifying this" fallback used when the popup's
// own describer string came back empty. kb.cpp's standalone morale and
// luck describers (0x4f32a0 / 0x4f3540) read the SAME rows at the same
// offsets with the same format_string/append shape, which is what fixes
// both bases and both roles.
DATA(0x006a57bc) extern const char* g_moraleTexts[42];
DATA(0x006a532c) extern const char* g_luckTexts[25];

// The five inlined-away rows, spelled as the Dreamcast members they
// are. `inline` (not a plain out-of-line definition) is what models
// retail's link: each of the three constructors expands them, nothing
// references the COMDAT, and /OPT:REF drops it - which is why the carve
// has no row for any of the five. Spending the /Ob2 budget on them here
// is also what keeps basic_string::_Tidy and the [-3, 3] selector OUT
// of line in the constructor, exactly as retail has them.

inline void TViewArmyWindow::createBackgroundWidget(const hero* thisHero)
{
    bitmapBorder* plate = new bitmapBorder(
        0, 0, 298, 311, BACKGROUND_ID,
        DATA_COMPGEN(0x0068c698, viewArmyPlate, "CrStkPU.pcx"),
        0x800);
    plate->setPlayerPaletteColors(
        thisHero != 0 && thisHero->m_owner >= 0
            ? thisHero->m_owner
            : g_game->getLocalPlayerGamePos());
    m_widgets.push_back(plate);
}

inline void TViewArmyWindow::createNameWidget(const char* name)
{
    m_widgets.push_back(new textWidget(
        20, 21, 258, 19, name, "smalfont.fnt",
        font::HEADING, NAME_ID, 5, 0, 8));
}

inline void TViewArmyWindow::createMoraleWidget(int newMorale)
{
    m_widgets.push_back(new iconWidget(
        23, 189, 42, 38, MORALE_ID,
        DATA_COMPGEN(0x0068c68c, viewArmyMoraleIcons, "imrl42.def"),
        limit(-3, newMorale, 3) + 3, 0, 0, 0, 0x10));
}

inline void TViewArmyWindow::createLuckWidget(int newLuck)
{
    m_widgets.push_back(new iconWidget(
        77, 189, 42, 38, LUCK_ID,
        DATA_COMPGEN(0x0068c680, viewArmyLuckIcons, "ilck42.def"),
        limit(-3, newLuck, 3) + 3, 0, 0, 0, 0x10));
}

inline void TViewArmyWindow::createRolloverWidget()
{
    m_rolloverWidget = new bitmapBackedTextWidget(
        7, 285, 284, 19, 0, "smalfont.fnt",
        DATA_COMPGEN(0x0068c674, viewArmyRolloverBack, "VARBack.pcx"),
        font::PRIMARY, ROLLOVER_ID, 1, 8);
    m_widgets.push_back(m_rolloverWidget);
}

// The single-stack popup: one army's whole record laid out over the
// 298x311 CrStkPU.pcx plate. EH-bearing (`push -1 / push __ehhandler$ /
// fs:[0]`) with thirteen unwind states, one per live widget allocation
// plus the two std::string members and the two describer temporaries.

// The mem-init list is what puts retail's vptr store at 0x5f33f0, AFTER
// both string constructors and the three flag bytes: VC6 lays the list
// out in DECLARATION order and only then stores the vptr, so
// ArmyType/ArmySize land ahead of morale_help/luck_help and the three
// Showing* bytes behind them, exactly as retail has it. morale, luck
// and Upgrade are NOT in the list - the first two are written from the
// body just before their icon rows, and Upgrade is never written here
// at all.

// Source-boundary recovery: DC ArmyType is TCreatureType, line 70 calls
// army::GetName, and lines 98/107 append the help strings. Restoring these
// together allows both description depth fences and the local enum union
// to go: 91.2989 -> 92.6780. All other tracked rows in six header consumers
// stay unchanged except the group constructor's improvement below. A
// 32-state family independently reproduced the all-corrections/unpinned
// corner. Retaining only the luck fence gives 93.5871, but keeps an override;
// deleting both without GetName gives 87.4391 with the typed member.
// The append correction is score-flat because the report masks relocation
// names; its evidence is the actual retail calls, not that scalar score.
// The current call sequence now recovers both mem-init _Tidy calls, reserve's
// size/_Destroy calls, and the morale temporary's _Tidy. The luck temporary
// still expands its cleanup (34 conditional branches versus retail's 31).

// Remaining nested-inline/lifetime differences are not permission to
// manufacture compiler work. Earlier bounded controls: mem-init depth 1/2
// and reserve depth 1/2/3 do not reach the missing nested boundaries;
// reserve depth 0 instead loses its expansion. Pasting the five widget
// helpers scores 61.42, an alternate clamp 87.11, and a forwarding vector
// reserve 88.3837 while also harming the other constructors. Plain vector
// derivation is byte-flat. Dummy size/capacity expressions and dead calls
// were rejected, as was extracting an unproven action-slot helper (89.30).
// Keep the canonical helpers and actual string lifetime boundaries.

// Retail's named call sequence independently fixes controller for the
// background and damage, owner for morale. The old assignment/pinned
// operator= discussion was wrong for this battle constructor; the group
// constructor really does use assignment. No union is needed to work
// around a de-inlined enum adapter once the owning member is typed.
// E:\gamedcs\viewarmywindow.cpp:55
VA(0x005f3360, 0x7B5)  // direct caller + CrStkPU.pcx, dc 0x190abc
TViewArmyWindow::TViewArmyWindow(const army* thisArmy, int x0, int y0,
                                 unsigned char showOk)
    : CAdvPopup(x0, y0, 298, 311, 0x12),
      m_armyType(thisArmy->m_creatureType),
      m_armySize(thisArmy->m_numTroops),
      m_showingUpgradeButton(0),
      m_showingDismissButton(0),
      m_showingOkButton(showOk)
{
    // The stack's OWN traits row is the copy embedded in army at +0x74
    // (army.h's sMonInfo slice); the table row is the unmodified one.
    const TCreatureTypeTraits* stackTraits = &thisArmy->m_monInfo;
    const TCreatureTypeTraits* typeTraits =
        &g_creatureTypeTraits[thisArmy->m_creatureType];

    unsigned char shooting = thisArmy->canShoot(0);
    int attack = thisArmy->getAdjustedAttack(0, shooting);
    int defense = thisArmy->getAdjustedDefense(0, 1);
    if (shooting) {
        int melee = thisArmy->getAdjustedAttack(0, 0);
        attack = cppMax(attack, melee);
    }

    m_widgets.reserve(NWIDGETS);

    // The plate is recoloured for the stack's CONTROLLER, falling back to
    // the local player when it has no hero - retail's relocation order is
    // `call get_controller` immediately ahead of the bitmapBorder ctor.
    // (The note that stood here said OWNER; see the correction above.)
    createBackgroundWidget(thisArmy->getController());

    // DC line 70 calls army::GetName; its canonical inline definition owns
    // the singular/plural lookup and invalid-type fallback.
    createNameWidget(thisArmy->getName());

    createPortraitWidget(stackTraits->m_spriteName,
                           stackTraits->m_townType, thisArmy->m_numTroops);
    createAttackWidget(typeTraits->m_attackSkill, attack);
    createDefenseWidget(typeTraits->m_defenseSkill, defense);
    createShotsWidget(stackTraits, typeTraits->m_numShots,
                        stackTraits->m_numShots);
    createDamageWidget(stackTraits, thisArmy->getController());
    createHitpointsWidget(typeTraits->m_hitPoints, stackTraits->m_hitPoints);
    createHitpointsLeftWidget(stackTraits->m_hitPoints
                                 - thisArmy->m_topCreatureDamage);
    createSpeedWidget(typeTraits->m_speed, thisArmy->getSpeed());

    m_morale = thisArmy->getMorale(0);
    createMoraleWidget(m_morale);

    int side = thisArmy->m_combatSide;
    const hero* ourHero = thisArmy->getOwner();
    const town* ourTown = 0;
    armyGroup* ourGroup = g_combatManager->m_armyGroups[side];
    const hero* enemyHero = g_combatManager->m_heroes[1 - side];
    const armyGroup* enemyGroup = g_combatManager->m_armyGroups[1 - side];
    unsigned char groupAlignments = g_combatManager->m_hasAngelicAlliance[side];
    if (side == 1)
        ourTown = g_combatManager->m_defendingTown;
    // DC lines 98/107 append the returned strings, and retail retains the
    // append calls. Complete reloads ArmyType for each added creature arg.
    m_moraleHelp += ourGroup->getMoraleDescription(
        m_armyType, m_morale, ourHero, ourTown,
        enemyHero, enemyGroup, g_combatManager->m_magicTerrain,
        groupAlignments);

    m_luck = thisArmy->getLuck(0);
    createLuckWidget(m_luck);
    m_luckHelp += ourGroup->getLuckDescription(
        m_armyType, m_luck, ourHero, ourTown,
        enemyHero, enemyGroup, g_combatManager->m_magicTerrain);

    createSpellInfluenceWidgets(thisArmy);
    if (showOk)
        createOkWidget();

    // The message strip along the bottom, kept in RolloverWidget so
    // WindowHandler can retarget its text.
    createRolloverWidget();

    // The bottom-left action slot: the Faerie Dragon's own cast button
    // when it is this side's turn and the stack still has its spell,
    // otherwise the creature's special-ability blurb.
    if (showOk
            && (thisArmy->m_spellInfluence[60] ? 1 - thisArmy->m_combatSide
                                         : thisArmy->m_combatSide)
                   == g_combatManager->m_currentSide
            && thisArmy->m_creatureType == CREATURE_FAERIE_DRAGON
            && stackTraits->m_hasSpell > 0
            && !g_combatManager->m_creaturePlacement) {
        m_widgets.push_back(new bitmapBorder(
            74, 236, 48, 34, -1,
            "Box46x32.pcx",  // pooled with create_upgrade_widget's 0x68c664
            0x800));
        m_widgets.push_back(new type_func_button(
            75, 237, 46, 32, OK_ID,
            DATA_COMPGEN(0x0066ffd4, viewArmyCastButton, "icm005.def"),
            viewArmyCastSpellHandler, 0, 1));
    } else if (stackTraits->m_specialAbility) {
        m_widgets.push_back(new textWidget(
            20, 232, 192, 41, stackTraits->m_specialAbility, "smalfont.fnt",
            font::WHITE, -1, 0, 0, 8));
    }

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

VA_COMPGEN(0x005f3b20, 0x21, SCALAR_DELETING_DTOR, TViewArmyWindow)

// The garrison/hero-screen popup: one slot of an armyGroup, shown with
// the owning hero's bonuses folded in and the upgrade/dismiss actions
// live. Three things separate it from the one-army constructor:

//  * the traits row is COPIED BY VALUE onto the frame (`mov ecx,0x1d /
//    rep movsd`) so hero::HeroFn_004E6120 can fold the hero's own
//    attack/defense and artifacts into the copy without touching the
//    table. Every "modified" column then reads the copy while its
//    "base" column reads the table row through the cached pointer.
//  * ShowingUpgradeButton and ShowingDismissButton are NOT in the
//    mem-init list - retail writes each 1 only inside the branch that
//    creates its button, and leaves them alone otherwise. Only
//    ShowingOkButton is initialised, which is why the vptr store
//    follows +0x96 directly.
//  * `Upgrade` is written from the BODY, after the vptr, because the
//    !show_ok early clean-up rewrites its incoming value first.

// Retail takes TEN arguments (`ret 0x28`), one more than the Dreamcast
// nine: a trailing alignment-grouping byte forwarded to GetArmyMorale's
// arg5 and get_morale_description's arg8.
// The shared includes.h limit/t_limit chain replaces the former
// declared-only cppClamp. DC create_morale_widget/create_luck_widget
// (0x1927d4/0x19288c) call limit; its by-value parameters provide the
// three homes before the reference selector. This raises the one-army
// constructor 90.1633 -> 91.2989 and this constructor 90.9521 -> 92.2569.
// Retail retains tLimit at the morale site and expands it at the luck
// site. Restoring ArmyType's enum ownership and GetArmyName below recovers
// that decision too: 93.7569 -> 97.4452, with all CFG edges agreeing. The
// remaining differences are local scheduling/homing and folded STL labels,
// not another source helper to paste into this constructor.
// Bypassing limit and calling tLimit directly at the two widget sites
// lowers the constructors to 88.5008 and 90.1918, respectively.
// E:\gamedcs\viewarmywindow.cpp:140
VA(0x005f3b50, 0x6B2)  // vtable-store + builder call set + describer pair, dc 0x190e78
TViewArmyWindow::TViewArmyWindow(armyGroup* group, int iarmy,
                                 const hero* thisHero, const town* thisTown,
                                 int x0, int y0, int upgrade,
                                 unsigned char showDismiss,
                                 unsigned char showOk,
                                 unsigned char groupAlignments)
    : CAdvPopup(x0, y0, 298, 311, 0x12),
      m_armyType(group->m_armyTypes[iarmy]),
      m_armySize(group->m_numTroops[iarmy]),
      m_showingOkButton(showOk)
{
    if (!showOk) {
        upgrade = -1;
        showDismiss = 0;
    }
    m_upgrade = upgrade;

    const TCreatureTypeTraits* typeTraits = &g_creatureTypeTraits[m_armyType];
    TCreatureTypeTraits traits = *typeTraits;

    // The widget vector NAMED AS A REFERENCE (three uses): 90.4657 ->
    // 90.9521.  The sibling army-only constructor below LOSES 0.03 on the
    // same change, so it is per-body.
    std::vector<widget*>& widgets = m_widgets;
    widgets.reserve(NWIDGETS);

    createBackgroundWidget(thisHero);

    // DC line 159 passes the literal 2 to GetArmyName for a plural name.
    createNameWidget(getArmyName(m_armyType, 2));

    int townType;
    if (!g_game->m_f1f698 && isBaseElemental(m_armyType))
        townType = -1;
    else
        townType = g_creatureTypeTraits[m_armyType].m_townType;
    createPortraitWidget(traits.m_spriteName, townType,
                           group->m_numTroops[iarmy]);

    if (thisHero)
        thisHero->heroFn004E6120(m_armyType, &traits);

    createAttackWidget(typeTraits->m_attackSkill, traits.m_attackSkill);
    createDefenseWidget(typeTraits->m_defenseSkill, traits.m_defenseSkill);
    createShotsWidget(&traits, traits.m_numShots, traits.m_numShots);
    createDamageWidget(&traits, thisHero);
    createHitpointsWidget(typeTraits->m_hitPoints, traits.m_hitPoints);
    createSpeedWidget(typeTraits->m_speed, traits.m_speed);

    m_morale = group->getArmyMorale(iarmy, thisHero, thisTown, -1,
                                  groupAlignments, 0);
    createMoraleWidget(m_morale);
    m_moraleHelp = group->getMoraleDescription(
        m_armyType, m_morale, thisHero, thisTown,
        0, 0, -1, groupAlignments);

    m_luck = group->getArmyLuck(iarmy, thisHero, thisTown, -1, 1);
    createLuckWidget(m_luck);
    m_luckHelp = group->getLuckDescription(
        m_armyType, m_luck, thisHero, thisTown,
        0, 0, -1);

    if (showOk)
        createOkWidget();

    if (upgrade != -1) {
        createUpgradeWidget();
        m_showingUpgradeButton = 1;
    }
    if (showDismiss) {
        createDismissWidget();
        m_showingDismissButton = 1;
    } else if (upgrade == -1 && traits.m_specialAbility) {
        widgets.push_back(new textWidget(
            20, 232, 192, 41, traits.m_specialAbility, "smalfont.fnt",
            font::WHITE, -1, 0, 0, 8));
    }

    createRolloverWidget();

    m_influence[0] = -1;
    m_influence[1] = -1;
    m_influence[2] = -1;

    for (widget** it = widgets.begin(); it != widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    // The upgrade button greys itself out when the player cannot pay.
    if (upgrade != -1) {
        long cost[7];
        getUpgradeCost(m_armyType,
                         creatureTypeFromInt(upgrade), m_armySize, cost);
        for (int i = 0; i < 7; i++) {
            if (g_currentPlayer->m_resources[i] < cost[i]) {
                widgetSetStatus(UPGRADE_ID, 8);
                break;
            }
        }
    }
}

VA(0x005f4210, 0x3C1)  // dc 0x19148c
TViewArmyWindow::TViewArmyWindow(int armyType, int x0, int y0,
                                 unsigned char showOk)
    : CAdvPopup(x0, y0, 298, 311, 0x12),
      m_armyType(creatureTypeFromInt(armyType)),
      m_showingUpgradeButton(0),
      m_showingDismissButton(0),
      m_showingOkButton(showOk)
{
    const TCreatureTypeTraits* traits = &g_creatureTypeTraits[armyType];

    m_widgets.reserve(NWIDGETS);

    // No hero owns a bare creature type, so the plate takes the local
    // player's colours unconditionally - the null argument is what folds
    // create_background_widget's ternary down to that one call.
    createBackgroundWidget(0);
    createNameWidget(traits->m_pluralName);

    int townType;
    if (!g_game->m_f1f698 && isBaseElemental(armyType))
        townType = -1;
    else
        townType = g_creatureTypeTraits[armyType].m_townType;
    createPortraitWidget(traits->m_spriteName, townType, 0);

    createAttackWidget(traits->m_attackSkill, traits->m_attackSkill);
    createDefenseWidget(traits->m_defenseSkill, traits->m_defenseSkill);
    createShotsWidget(traits, traits->m_numShots, traits->m_numShots);
    createDamageWidget(traits, 0);
    createHitpointsWidget(traits->m_hitPoints, traits->m_hitPoints);
    createSpeedWidget(traits->m_speed, traits->m_speed);

    if (showOk)
        createOkWidget();

    if (traits->m_specialAbility) {
        m_widgets.push_back(new textWidget(
            20, 232, 192, 41, traits->m_specialAbility, "smalfont.fnt",
            font::WHITE, -1, 0, 0, 8));
    }

    createRolloverWidget();

    for (int i = 0; i < 3; i++)
        m_influence[i] = -1;

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

VA(0x005f45e0, 0xD5)  // dc 0x191660
TViewArmyWindow::~TViewArmyWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x005f46c0, 0x12C)  // dc 0x1916d4
int TViewArmyWindow::convertID2HelpID(int id) const
{
    if (id < 0)
        return -1;

    switch (id) {
    case NAME_ID: return 0;
    case NUMBER_ID: return 1;
    case ATTACK_LABEL_ID: return 2;
    case ATTACK_ID: return 2;
    case DEFENSE_LABEL_ID: return 3;
    case DEFENSE_ID: return 3;
    case SHOTS_LABEL_ID: return 4;
    case SHOTS_ID: return 4;
    case DAMAGE_LABEL_ID: return 5;
    case DAMAGE_ID: return 5;
    case HEALTH_LABEL_ID: return 6;
    case HEALTH_ID: return 6;
    case HEALTH_REMAINING_LABEL_ID: return 7;
    case HEALTH_REMAINING_ID: return 7;
    case SPEED_LABEL_ID: return 7;
    case SPEED_ID: return 8;
    case MORALE_ID: return 9;
    case LUCK_ID: return 10;
    case AFFECTING_SPELLS_0_ID: return 11;
    case AFFECTING_SPELLS_1_ID: return 11;
    case AFFECTING_SPELLS_2_ID: return 11;
    case DISMISS_ID: return 12;
    case UPGRADE_ID: return 13;
    case ACCEPT_ID: return 14;
    case OK_ID: return 15;
    default: return -1;
    }
}

#if 0  // @carcass

// E:\gamedcs\viewarmywindow.cpp:366 - no retail row: the carve has nothing
// between DoModal (0x5f47f0 + 92 = 0x5f484c) and WindowHandler (0x5f4850),
// so retail either inlined this at its one call site or /OPT:REF dropped it.
DC_ONLY(0x191764, 0x40)
void TViewArmyWindow::quickView()
{
    // @stub
}

#endif  // @carcass

VA(0x005f47f0, 0x5C)  // dc 0x1917a4
void TViewArmyWindow::doModal()
{
    g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] =
        GameTime::get() + VIEW_ARMY_DELAY;

    if (!g_currentPlayer->isLocalHuman()) {
        widget* action = getWidget(UPGRADE_ID);
        if (action)
            action->enable(0);
        action = getWidget(DISMISS_ID);
        if (action)
            action->enable(0);
    }
    heroWindow::doModal(0);
}

// The widget id the rollover strip was last built for, so a mouse move
// that stays inside one widget costs nothing. This TU's own .data cell,
// sitting immediately in front of its string literal pool (0x68c664 is
// "Box46x32.pcx") and initialised to -1 in the image.
DATA(0x0068c660) static int g_lastViewArmyHoverId = -1;

// The popup's whole input surface, in three arms over one message:
// right-click help, the three action buttons, and the rollover strip -
// with the creature sprite's animation step as the unconditional tail.

// The rollover arm's one special case is a spell icon: for the three
// spells whose effect has no turn count (Bind, Berserk, Disrupting Ray)
// the strip gets a fixed descriptor instead of Duration.

VA(0x005f4850, 0x7D7)  // dc 0x191804
int TViewArmyWindow::windowHandler(message* msg)
{
    unsigned char exitFlag;
    pollSound();

    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    exitFlag = 0;
    if (msg->m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg->m_codeX == widget::WIDGET_SELECT
            || msg->m_codeX == widget::WIDGET_RIGHT_SELECT) {
            // `text.assign(...)` at all seven help-text stores, not
            // `text = ...`: basic_string::operator= is a forwarder to
            // assign that CARRIES the assign call, so spelling the deeper
            // level directly is worth 92.5744 -> 99.1520 here. Measured and
            // rejected on top of it: `.append` for the four `text +=`
            // stores (byte-flat) and a named `const std::string& rclick`
            // for the default arm's subscript (72.70).
            int helpID = convertID2HelpID(msg->m_codeY);
            int resType = -1;
            std::string text;
            switch (helpID) {
            case g_moraleHelpIndex:
                if (m_morale > 0) {
                    text.assign(formatString(g_moraleTexts[3], g_moraleTexts[0]));
                    resType = 14;
                } else if (m_morale == 0) {
                    text.assign(formatString(g_moraleTexts[3], g_moraleTexts[1]));
                    resType = 15;
                } else {
                    text.assign(formatString(g_moraleTexts[3], g_moraleTexts[2]));
                    resType = 16;
                }
                if (m_moraleHelp.length() == 0)
                    text += g_moraleTexts[23];
                else
                    text += m_moraleHelp;
                break;
            case g_luckHelpIndex:
                if (m_luck > 0) {
                    text.assign(formatString(g_luckTexts[3], g_luckTexts[0]));
                    resType = 11;
                } else if (m_luck == 0) {
                    text.assign(formatString(g_luckTexts[3], g_luckTexts[1]));
                    resType = 12;
                } else {
                    text.assign(formatString(g_luckTexts[3], g_luckTexts[2]));
                    resType = 13;
                }
                if (m_luckHelp.length() == 0)
                    text += g_luckTexts[18];
                else
                    text += m_luckHelp;
                break;
            default:
                if (helpID >= 0)
                    text.assign(g_viewArmyHelp[helpID].m_rclick);
                break;
            }
            if (text.length() > 0)
                normalDialog(text.c_str(), 4, -1, -1, resType, 0, -1, 0,
                             -1, 0, -1, 0);
        }
    } else if (msg->m_id == MESSAGE_WIDGET) {
        if (msg->m_codeX == widget::WIDGET_DESELECT) {
            switch (msg->m_codeY) {
            case UPGRADE_ID: {
                long cost[7];
                int amount;
                amount = 0;
                union {
                    int m_value;
                    TCreatureType m_creature;
                } upgradeType;
                upgradeType.m_value = m_upgrade;
                getUpgradeCost(m_armyType, upgradeType.m_creature,
                                 m_armySize, cost);
                int resource;
                for (resource = 5; resource >= 0; resource--) {
                    if (cost[resource] > 0)
                        break;
                }
                if (resource >= 0)
                    amount = cost[resource];
                normalDialog(g_generalText->getText(
                                 GENERAL_TEXT_UPGRADE_ARMY_PROMPT),
                             2, -1, -1, 6, cost[6], resource, amount,
                             -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT)
                    exitFlag = 1;
                break;
            }
            case DISMISS_ID:
                normalDialog(g_generalText->getText(
                                 GENERAL_TEXT_DISMISS_ARMY_PROMPT),
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT)
                    exitFlag = 1;
                break;
            case ACCEPT_ID:
                exitFlag = 1;
                break;
            }
        }
    } else if (msg->m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = findWidget(msg->m_mouseX, msg->m_mouseY);
        if (hoverID != g_lastViewArmyHoverId) {
            const char* rollover = g_emptyRolloverText;
            g_lastViewArmyHoverId = hoverID;
            if (hoverID != -1) {
                g_mouseManager->setPointer(1, mouseManager::DEFAULT_SET);
                int helpID = convertID2HelpID(hoverID);
                if (helpID >= 0) {
                    if (hoverID >= AFFECTING_SPELLS_0_ID
                        && hoverID <= AFFECTING_SPELLS_2_ID
                        && m_influence[hoverID - AFFECTING_SPELLS_0_ID] != -1) {
                        const int& spell =
                            m_influence[hoverID - AFFECTING_SPELLS_0_ID];
                        if (spell == SPELL_BIND)
                            sprintf(g_text,
                                    g_generalText->getText(
                                        GENERAL_TEXT_ARMY_SPELL_FOREVER_FORMAT),
                                    g_spellTraits[SPELL_BIND].m_name,
                                    g_generalText->getText(
                                        GENERAL_TEXT_ARMY_SPELL_BIND));
                        else if (spell == SPELL_BERSERK)
                            sprintf(g_text,
                                    g_generalText->getText(
                                        GENERAL_TEXT_ARMY_SPELL_FOREVER_FORMAT),
                                    g_spellTraits[SPELL_BERSERK].m_name,
                                    g_generalText->getText(
                                        GENERAL_TEXT_ARMY_SPELL_BERSERK));
                        else if (spell == SPELL_DISRUPTING_RAY)
                            sprintf(g_text,
                                    g_generalText->getText(
                                        GENERAL_TEXT_ARMY_SPELL_FOREVER_FORMAT),
                                    g_spellTraits[SPELL_DISRUPTING_RAY].m_name,
                                    g_generalText->getText(
                                        GENERAL_TEXT_ARMY_SPELL_DISRUPTING_RAY));
                        else
                            sprintf(g_text,
                                    g_generalText->getText(
                                        GENERAL_TEXT_ARMY_SPELL_ROUNDS_FORMAT),
                                    g_spellTraits[spell].m_name,
                                    m_duration[hoverID - AFFECTING_SPELLS_0_ID]);
                        rollover = g_text;
                    } else {
                        rollover = g_viewArmyHelp[helpID].m_text;
                    }
                }
            } else {
                g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
            }
            m_rolloverWidget->setText(rollover);
            drawWindow(0, ROLLOVER_ID, ROLLOVER_ID);
            g_windowManager->updateScreen(m_x + m_rolloverWidget->m_x,
                                          m_y + m_rolloverWidget->m_y,
                                          m_rolloverWidget->m_width,
                                          m_rolloverWidget->m_height);
        }
    }

    if (exitFlag) {
        msg->m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg->m_codeY;
        msg->m_codeY = widget::WIDGET_END_DIALOG;
        msg->m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }

    if (GameTime::isPast(g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])) {
        if (isSiegeWeapon(m_armyType))
            m_spriteWidget->nextRandomSiegeEngineFrame();
        else
            m_spriteWidget->nextRandomFrame();
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] =
            GameTime::nextFrameTime(
                g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT],
                VIEW_ARMY_DELAY);
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x005f5060, 0x2D6)  // dc 0x191f2c
void TViewArmyWindow::createPortraitWidget(const char* spriteName,
                                             int townType, int count)
{
    m_widgets.push_back(new bitmapBorder(
        21, 48, 100, 130, SPRITE_BACKGROUND_ID,
        g_creatureBackgrounds[townType],
        0x800));

    m_spriteWidget = new iconWidget(
        21, 48, 100, 130, SPRITE_ID, spriteName, 0, 2, 0, 0, 0x12);
    m_widgets.push_back(m_spriteWidget);

    if (count > 0) {
        sprintf(g_text, "%d", count);
        m_widgets.push_back(new textWidget(
            21, 160, 100, 20, g_text,
            DATA_COMPGEN(0x006700b4, viewArmyCountFont, "Verd10B.fnt"),
            font::WHITE, NUMBER_ID, 10, 0, 8));
    }
}

VA(0x005f5340, 0x28E)  // widget IDs + primary-skill table + format literals
void TViewArmyWindow::createAttackWidget(int normalAttackSkill,
                                           int currentAttackSkill)
{
    m_widgets.push_back(new textWidget(
        154, 48, 122, 17, g_primarySkillNames[0], "smalfont.fnt",
        font::PRIMARY, ATTACK_LABEL_ID, 4, 0, 8));

    if (normalAttackSkill == currentAttackSkill)
        sprintf(g_text, "%d", normalAttackSkill);
    else
        sprintf(g_text, "%d(%d)", normalAttackSkill, currentAttackSkill);

    m_widgets.push_back(new textWidget(
        154, 48, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, ATTACK_ID, 6, 0, 8));
}

VA(0x005f55d0, 0x28E)  // widget IDs + primary-skill table + format literals
void TViewArmyWindow::createDefenseWidget(int normalDefenseSkill,
                                            int currentDefenseSkill)
{
    m_widgets.push_back(new textWidget(
        154, 66, 122, 17, g_primarySkillNames[1], "smalfont.fnt",
        font::PRIMARY, DEFENSE_LABEL_ID, 4, 0, 8));

    if (normalDefenseSkill == currentDefenseSkill)
        sprintf(g_text, "%d", normalDefenseSkill);
    else
        sprintf(g_text, "%d(%d)", normalDefenseSkill,
                currentDefenseSkill);

    m_widgets.push_back(new textWidget(
        154, 66, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, DEFENSE_ID, 6, 0, 8));
}

// createDamageWidget retains the release invariant !m_widgets.empty() after
// its first insertion (DC viewarmywindow.cpp:707). Pricing vector::empty()
// preserves the final push_back expansion and its retained vector::size() call.
// Removing that evaluation expands size() too and adds a branch (21 vs 20).
VA(0x005f5860, 0x2C2)  // dc 0x19226c
void TViewArmyWindow::createDamageWidget(const TCreatureTypeTraits* traits,
                                           const hero* ourHero)
{
    m_widgets.push_back(new textWidget(
        154, 104, 122, 17,
        g_generalText->getText(GENERAL_TEXT_VIEW_ARMY_DAMAGE),
        "smalfont.fnt", font::PRIMARY, DAMAGE_LABEL_ID, 4, 0, 8));

    int low = traits->m_damageLowBound;
    int high = traits->m_damageHighBound;
    if (ourHero != 0 && m_armyType == CREATURE_BALLISTA) {
        low *= ourHero->getPrimarySkill(0) + 1;
        high *= ourHero->getPrimarySkill(0) + 1;
    }

    if (low == high)
        sprintf(g_text, "%d", low);
    else
        sprintf(g_text, DATA_COMPGEN(0x0068c6a4, viewArmyDamageRangeFormat,
                                    "%d - %d"),
                low, high);

    // Conventional release expansion of VERIFY(!Widgets.empty()).
    static_cast<void>(!m_widgets.empty());
    m_widgets.push_back(new textWidget(
        154, 103, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, DAMAGE_ID, 6, 0, 8));
}

VA(0x005f5b30, 0x29D)  // dc 0x1923c0
void TViewArmyWindow::createShotsWidget(const TCreatureTypeTraits* traits,
                                          int normalShots, int currentShots)
{
    if (traits->m_attributes & g_ctaShooter) {
        m_widgets.push_back(new textWidget(
            154, 85, 122, 17,
            g_generalText->getText(GENERAL_TEXT_VIEW_ARMY_SHOTS),
            "smalfont.fnt", font::PRIMARY, SHOTS_LABEL_ID, 4, 0, 8));

        if (normalShots == currentShots)
            sprintf(g_text, "%d", normalShots);
        else
            sprintf(g_text, "%d(%d)", normalShots, currentShots);

        m_widgets.push_back(new textWidget(
            154, 85, 122, 17, g_text, "smalfont.fnt",
            font::PRIMARY, SHOTS_ID, 6, 0, 8));
    }
}

VA(0x005f5dd0, 0x297)  // widget IDs + text-record field + format literals
void TViewArmyWindow::createHitpointsWidget(int normalHitpoints,
                                              int currentHitpoints)
{
    m_widgets.push_back(new textWidget(
        154, 123, 122, 17,
        g_generalText->getText(GENERAL_TEXT_VIEW_ARMY_HEALTH),
        "smalfont.fnt", font::PRIMARY, HEALTH_LABEL_ID, 4, 0, 8));

    if (normalHitpoints == currentHitpoints)
        sprintf(g_text, "%d", normalHitpoints);
    else
        sprintf(g_text, "%d(%d)", normalHitpoints, currentHitpoints);

    m_widgets.push_back(new textWidget(
        154, 123, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, HEALTH_ID, 6, 0, 8));
}

VA(0x005f6070, 0x27D)  // widget IDs + text-record field + integer format
void TViewArmyWindow::createHitpointsLeftWidget(int hitpointsLeft)
{
    m_widgets.push_back(new textWidget(
        154, 142, 122, 17,
        g_generalText->getText(GENERAL_TEXT_VIEW_ARMY_HEALTH_REMAINING),
        "smalfont.fnt", font::PRIMARY, HEALTH_REMAINING_LABEL_ID, 4, 0, 8));

    sprintf(g_text, "%d", hitpointsLeft);

    m_widgets.push_back(new textWidget(
        154, 142, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, HEALTH_REMAINING_ID, 6, 0, 8));
}

VA(0x005f62f0, 0x2BE)  // widget IDs + text-record field + clamped formats
void TViewArmyWindow::createSpeedWidget(int normalSpeed,
                                          int currentSpeed)
{
    m_widgets.push_back(new textWidget(
        154, 161, 122, 17,
        g_generalText->getText(GENERAL_TEXT_VIEW_ARMY_SPEED),
        "smalfont.fnt", font::PRIMARY, SPEED_LABEL_ID, 4, 0, 8));

    normalSpeed = cppMax<int>(0, normalSpeed);
    currentSpeed = cppMax<int>(0, currentSpeed);
    if (normalSpeed == currentSpeed)
        sprintf(g_text, "%d", normalSpeed);
    else
        sprintf(g_text, "%d(%d)", normalSpeed, currentSpeed);

    m_widgets.push_back(new textWidget(
        154, 161, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, SPEED_ID, 6, 0, 8));
}

// The fixed three-slot row shows the newest standing spell influences. The
// queue's VC6 deque layout, its 16-byte iterator and the two 81-dword spell
// rows are independently byte-proven in army.h; this body is their first UI
// consumer. Retail always visits all three display slots, writing -1 into an
// unused Influence entry so WindowHandler can suppress its help text.

// Residual (86.65703%): this is a cyclic VC6 inliner wall. The named iterator
// plus the statement-scoped pin below reproduces retail's 16-byte begin()
// temporary and its one out-of-line operator+= call, but our remaining budget
// expands the first vector::_Ucopy loop (18 branches against retail's 16).
// The natural deque subscript, including bounded inline-depth 1 and 2 probes,
// makes that vector insertion take retail's call-form but expands the deque's
// map/block arithmetic here instead (79.8430%). An explicit begin()+i is
// 79.3512%, direct vector::insert 84.6860%, and the non-const iterator path
// 79.5413%. The outer loop, member offsets, widget arguments and return agree;
// the remaining choice is one front-end inline budget spent at either of two
// nested STL sites.
// E:\gamedcs\viewarmywindow.cpp:837
VA(0x005f65b0, 0x2B5)  // queue iterator arithmetic + SpellInt.def + widget ids
void TViewArmyWindow::createSpellInfluenceWidgets(const army* thisArmy)
{
    int x = 127;
    unsigned int first = cppMax<int>(
        0, static_cast<int>(thisArmy->m_spellInfluenceQueue.size()) - NSPELLS);
    unsigned int i = first;
    int* influence = m_influence;
    int widgetId = AFFECTING_SPELLS_0_ID - first;
    int count = NSPELLS;

    do {
        if (i < thisArmy->m_spellInfluenceQueue.size()) {
            army::TSpellQueue::const_iterator position =
                thisArmy->m_spellInfluenceQueue.begin();
#pragma inline_depth(0)
            position += i;
#pragma inline_depth()
            *influence = *position;
            influence[NSPELLS] = thisArmy->m_spellInfluence[*influence];
            m_widgets.push_back(new iconWidget(
                x, 186, 48, 36, widgetId + i,
                DATA_COMPGEN(0x006700a4, viewArmySpellIcons, "spellint.def"),
                *influence + 1, 0, 0, 0, 0x10));
        } else {
            *influence = -1;
        }
        ++influence;
        x += 52;
        ++i;
    } while (--count);
}

VA(0x005f6870, 0x264)  // dc 0x192a28
void TViewArmyWindow::createOkWidget()
{
    m_widgets.push_back(new bitmapBorder(
        215, 237, 66, 32, OK_BORDER_ID,
        DATA_COMPGEN(0x0067016c, viewArmyOkBorder, "Box64x30.pcx"),
        0x800));

    button* accept = new button(
        216, 238, 64, 30, ACCEPT_ID,
        DATA_COMPGEN(0x0068c6ac, viewArmyOkButton, "iOKAY.def"),
        0, 1, 0, 0, 2);
    accept->setHotkey(1);
    accept->setHotkey(28);
    m_widgets.push_back(accept);
}

VA(0x005f6ae0, 0x265)  // dc 0x192ad8
void TViewArmyWindow::createUpgradeWidget()
{
    m_widgets.push_back(new bitmapBorder(
        74, 236, 48, 34, -1,
        DATA_COMPGEN(0x0068c664, viewArmySmallActionBorder, "Box46x32.pcx"),
        0x800));

    button* upgrade = new button(
        75, 237, 46, 32, UPGRADE_ID,
        DATA_COMPGEN(0x00660134, viewArmyUpgradeButton, "iViewCr.def"),
        0, 1, 0, 0, 2);
    upgrade->setHotkey(22);
    m_widgets.push_back(upgrade);
}

VA(0x005f6d50, 0x265)  // dc 0x192b40
void TViewArmyWindow::createDismissWidget()
{
    m_widgets.push_back(new bitmapBorder(
        19, 236, 48, 34, -1,
        "Box46x32.pcx",
        0x800));

    button* dismiss = new button(
        20, 237, 46, 32, DISMISS_ID,
        DATA_COMPGEN(0x00660124, viewArmyDismissButton, "iViewCr2.def"),
        0, 1, 0, 0, 2);
    dismiss->setHotkey(32);
    m_widgets.push_back(dismiss);
}

VA_COMPGEN(0x00404860, 0x210, BASIC_STRING_ASSIGN_STR, char)

VA_COMPGEN(0x00404bc0, 0x06, BASIC_STRING_MAX_SIZE, char)

// COMDAT pairing: basic_string<char>::_Split - the reference-count split that
// assign/_Freeze reach; a nullary private thiscall in the same Dinkumware
// block as _Tidy (0x40f0), _Grow (0x4a90) and the assign above.
VA_COMPGEN(0x00404ce0, 0xD8, BASIC_STRING_SPLIT, char)
