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
#include "includes.h"

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
    const TCreatureTypeTraits& typeTraits =
        g_creatureTypeTraits[thisArmy->m_creatureType];

    // 97.20%: 67/67 blocks exact, every reloc and call agrees, and the
    // sole residual is one stack slot - retail spills the shooting-attack
    // max to [ebp-0x1c] where we use [ebp-0x18]. Everything stored before
    // it matches, so retail owns one extra 4-byte slot whose lifetime
    // starts here. Falsified: hoisting `side`'s declaration (no change -
    // VC6 slots by first use, not declaration), `int shooting` (96.73,
    // and the divergence moves earlier), and an added early int (folded
    // away). DC types the traits local as a reference, which is adopted
    // above and is byte-neutral.
    unsigned char shooting = thisArmy->canShoot(0);
    int attack = thisArmy->getAdjustedAttack(0, shooting);
    int defense = thisArmy->getAdjustedDefense(0, 1);
    if (shooting) {
        attack = max(attack, thisArmy->getAdjustedAttack(0, 0));
    }

    m_widgets.reserve(NWIDGETS);

    createBackgroundWidget(thisArmy->getController());

    // DC line 70 calls army::GetName; its canonical inline definition owns
    // the singular/plural lookup and invalid-type fallback.
    createNameWidget(thisArmy->getName());

    createPortraitWidget(stackTraits->m_spriteName,
                           stackTraits->m_townType, thisArmy->m_numTroops);
    createAttackWidget(typeTraits.m_attackSkill, attack);
    createDefenseWidget(typeTraits.m_defenseSkill, defense);
    createShotsWidget(*stackTraits, typeTraits.m_numShots,
                        stackTraits->m_numShots);
    createDamageWidget(*stackTraits, thisArmy->getController());
    createHitpointsWidget(typeTraits.m_hitPoints, stackTraits->m_hitPoints);
    createHitpointsLeftWidget(stackTraits->m_hitPoints
                                 - thisArmy->m_topCreatureDamage);
    createSpeedWidget(typeTraits.m_speed, thisArmy->getSpeed());

    createMoraleWidget(thisArmy->getMorale(0));

    int side = thisArmy->getOwningSide();
    hero* thisHero = thisArmy->getOwner();
    hero* enemyHero = g_combatManager->m_heroes[1 - side];
    armyGroup* group = g_combatManager->m_armyGroups[side];
    armyGroup* enemies = g_combatManager->m_armyGroups[1 - side];
    unsigned char groupAlignments = g_combatManager->m_hasAngelicAlliance[side];
    const town* ourTown = 0;
    if (side == 1)
        ourTown = g_combatManager->m_defendingTown;
    // DC lines 98/107 append the returned strings, and retail retains the
    // append calls. Complete reloads ArmyType for each added creature arg.
    m_moraleHelp += group->getMoraleDescription(
        m_armyType, m_morale, thisHero, ourTown,
        enemyHero, enemies, g_combatManager->m_magicTerrain,
        groupAlignments);

    createLuckWidget(thisArmy->getLuck(0));
    m_luckHelp += group->getLuckDescription(
        m_armyType, m_luck, thisHero, ourTown,
        enemyHero, enemies, g_combatManager->m_magicTerrain);

    createSpellInfluenceWidgets(thisArmy);
    if (showOk)
        createOkWidget();

    // Complete places the rollover before the Faerie Dragon action slot;
    // retail calls this builder before the button/blurb branch. The older
    // DC rows 113/114 instead put its blurb before create_rollover_widget.
    // WindowHandler retains this widget to retarget its text.
    createRolloverWidget();

    // The bottom-left action slot: the Faerie Dragon's own cast button
    // when it is this side's turn and the stack still has its spell,
    // otherwise the creature's special-ability blurb.
    if (showOk
            && thisArmy->getControllingSide()
                   == g_combatManager->m_currentSide
            && thisArmy->m_creatureType == CREATURE_FAERIE_DRAGON
            && stackTraits->m_hasSpell > 0
            && !g_combatManager->m_creaturePlacement) {
        m_widgets.push_back(new bitmapBorder(
            74, 236, 48, 34, -1,
            "Box46x32.pcx",  // pooled with create_upgrade_widget's 0x68c664
            0x800));
        m_widgets.push_back(new type_func_button(
            75, 237, 48, 36, OK_ID,
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

// Current group-constructor score: 88.5394 versus the preceding 91.1781;
// HIST retains 97.4452. The background's recorded palette-call arms and the
// morale/luck helpers' owning member stores are positive source facts. Keep
// those boundaries while recovering this caller's remaining nested inlining.
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
    createShotsWidget(traits, traits.m_numShots, traits.m_numShots);
    createDamageWidget(traits, thisHero);
    createHitpointsWidget(typeTraits->m_hitPoints, traits.m_hitPoints);
    createSpeedWidget(typeTraits->m_speed, traits.m_speed);

    createMoraleWidget(group->getArmyMorale(
        iarmy, thisHero, thisTown, -1, groupAlignments, 0));
    m_moraleHelp = group->getMoraleDescription(
        m_armyType, m_morale, thisHero, thisTown,
        0, 0, -1, groupAlignments);

    createLuckWidget(group->getArmyLuck(iarmy, thisHero, thisTown, -1, 1));
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
                         TCreatureType(upgrade), m_armySize, cost);
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
      // Retail initialises the creature type in the member list, before the
      // widget run; assigning it in the body costs 94.4475 against 97.1745.
      m_armyType(TCreatureType(armyType)),
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
    createShotsWidget(*traits, traits->m_numShots, traits->m_numShots);
    createDamageWidget(*traits, 0);
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

// Original: TViewArmyWindow::QuickView; viewarmywindow.cpp:366, dc 0x191764.
// HillFortWindow's right-click path invokes the common quick-view wrapper.
void TViewArmyWindow::quickView()
{
    g_windowManager->doQuickView(this);
}

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
//
// EXACT (2026-09-06), and the road there is worth keeping because three
// separate levers each closed a different third of it.
// (1) The exits are the Dreamcast's `bExitFlag` device, not gotos. The
// dossier names the byte local (sp+0x33), zeroes it right before the
// qualifier test (line 412), sets it in the UPGRADE and DISMISS arms'
// accept tests and in the ACCEPT arm (511-520), and tests it ONCE after
// the three arms (583): `if (bExitFlag) { msg->id = WIDGET; dialogReturn =
// codeY; codeY = codeX = END_DIALOG; return 2; }` followed by the one
// animation step behind `GameTime::IsPast(glTimers[..])` (588) and
// `return 1`. That is the whole of the placement the earlier notes fought
// with `check_accept:`/`accepted:`/`animate_tail:` labels and a duplicated
// right-click animation copy: VC6 threads the constant flag into a shared
// return-2 block, cross-jumps the dismiss arm into the upgrade arm's
// NormalDialog+test tail by itself, and the IsPast form is what puts the
// glTimers load ahead of the GameTime::Get call. 77.26 -> 92.57 on that one
// accessor. Same device as townManager::Main and advManager::ProcessKeyPress.
// (2) THE DEPTH LADDER on the seven help-text stores (polish 29):
// `text.assign(X)` rather than `text = X`, 92.5744 -> 99.1520. The note at
// the help arm records what else was measured there.
// (3) The rollover arm's `spell` BOUND BY `const int&`, 99.1520 -> 100.0000
// - see the width sweep recorded at that declaration.
// E:\gamedcs\viewarmywindow.cpp:404
VA(0x005f4850, 0x7D7)  // direct caller + convertID2HelpID + help table, dc 0x191804
int TViewArmyWindow::windowHandler(message& msg)
{
    unsigned char exitFlag;
    pollSound();

    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    exitFlag = 0;
    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg.m_codeX == widget::WIDGET_SELECT
            || msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
            int helpID = convertID2HelpID(msg.m_codeY);
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
    } else if (msg.m_id == MESSAGE_WIDGET) {
        if (msg.m_codeX == widget::WIDGET_DESELECT) {
            switch (msg.m_codeY) {
            case UPGRADE_ID: {
                long cost[7];
                int amount;
                amount = 0;
                int upgradeType;
                upgradeType = m_upgrade;
                getUpgradeCost(m_armyType, TCreatureType(upgradeType),
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
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = findWidget(msg.m_mouseX, msg.m_mouseY);
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
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
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

// Complete-only Faerie Dragon cast-button callback. The battle constructor
// takes 0x5f5030 as its handler address. Its two returns end at 0x5f505a;
// the five NOPs before the next function at 0x5f5060 are padding, so the
// missing retail inventory row owns 43 bytes. The authored fastcall
// message-reference body reproduces all 43 bytes without relocations.
VA(0x005f5030, 0x2B)  // Complete-only callback: address taken by battle constructor
int viewArmyCastSpellHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_DESELECT
            && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        msg.m_codeY = TViewArmyWindow::OK_ID;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

// The five inline helpers keep one definition and their source calls. Their
// relative positions follow DC rows 617/635, 809/823 and 917. Retail expands
// them in the constructors and retains no standalone bodies. Their real
// branches and member accesses affect the callers' nested string cleanup and
// selector inlining. A 32-state explicit/ordinary inline diagnostic leaves
// the battle constructor byte-neutral under /Ob2; ordinary forms additionally
// emit unpaired helper bodies. That does not establish new retail claims.

inline void TViewArmyWindow::createBackgroundWidget(const hero* thisHero)
{
    bitmapBorder* plate = new bitmapBorder(
        0, 0, 298, 311, BACKGROUND_ID,
        DATA_COMPGEN(0x0068c698, viewArmyPlate, "CrStkPU.pcx"),
        0x800);
    // DC rows 620/622/623 give the two palette-call arms; SH4 merges the
    // final call. In the battle constructor, the shared ternary-call probe
    // changes the following widget insert/GetName schedules. These original
    // arms recover their retail instructions under VC6.
    if (thisHero != 0 && thisHero->m_owner >= 0) {
        plate->setPlayerPaletteColors(thisHero->m_owner);
    } else {
        plate->setPlayerPaletteColors(g_game->getLocalPlayerGamePos());
    }
    m_widgets.push_back(plate);
}

inline void TViewArmyWindow::createNameWidget(const char* name)
{
    m_widgets.push_back(new textWidget(
        20, 21, 258, 19, name, "smalfont.fnt",
        font::HEADING, NAME_ID, 5, 0, 8));
}

// The creature portrait: the town-alignment backdrop, the creature's own
// animated sprite kept in SpriteWidget, and the stack size printed under
// them in the popup's one Verd10B row. The count line is suppressed for an
// empty slot, which is what the three-widget/two-widget split of the retail
// tail encodes.
// E:\gamedcs\viewarmywindow.cpp:646
VA(0x005f5060, 0x2D6)  // ctor call set + CrBkg table + Verd10B.fnt, dc 0x191f2c
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

// The damage row prints a single number when the creature's low and high
// bounds agree and a "low - high" range otherwise. The one creature whose
// damage is not the table's is the BALLISTA: its bounds are scaled by the
// wielding hero's Attack skill, which is why the builder takes a hero at
// all. Both `+ 1` factors expand hero::GetPrimarySkill(0) - the 0..99
// clamp is byte-visible twice over a single `mov cl,[hero+0x476]`, and
// its `skill >= 2 ? 1 : 0` floor folds to `xor edi,edi` for skill 0,
// which is what fixes the index as Attack.
//
// DC 707 types traits as const TCreatureTypeTraits&, and row 708 calls
// TTextResource::operator[]. Restoring both interfaces and all three callers
// preserves the 706 retail instruction bytes after the legitimate function
// name/exception-handler owner relabeling. The shots helper has the same
// reference/accessor evidence at rows 735/738 and preserves its 669 bytes.
//
// E:\gamedcs\viewarmywindow.cpp:707
VA(0x005f5860, 0x2C2)  // widget IDs + text-record field + "%d - %d", dc 0x19226c
void TViewArmyWindow::createDamageWidget(const TCreatureTypeTraits& traits,
                                           const hero* ourHero)
{
    m_widgets.push_back(new textWidget(
        154, 104, 122, 17,
        (*g_generalText)[GENERAL_TEXT_VIEW_ARMY_DAMAGE],
        "smalfont.fnt", font::PRIMARY, DAMAGE_LABEL_ID, 4, 0, 8));

    int low = traits.m_damageLowBound;
    int high = traits.m_damageHighBound;
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

    m_widgets.push_back(new textWidget(
        154, 103, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, DAMAGE_ID, 6, 0, 8));
}

// The ammunition row exists only for a shooter, which is what the whole
// body sitting under one `test byte [traits+0x10], 4` says: a melee stack
// gets neither the label nor the count. Inside, the base/modified pair is
// the same presentation the primary skills use, on one shared y.
// E:\gamedcs\viewarmywindow.cpp:735
VA(0x005f5b30, 0x29D)  // widget IDs + text-record field + format literals, dc 0x1923c0
void TViewArmyWindow::createShotsWidget(const TCreatureTypeTraits& traits,
                                          int normalShots, int currentShots)
{
    if (traits.m_attributes & g_ctaShooter) {
        m_widgets.push_back(new textWidget(
            154, 85, 122, 17,
            (*g_generalText)[GENERAL_TEXT_VIEW_ARMY_SHOTS],
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
        (*g_generalText)[GENERAL_TEXT_VIEW_ARMY_SPEED],
        "smalfont.fnt", font::PRIMARY, SPEED_LABEL_ID, 4, 0, 8));

    normalSpeed = max(0, normalSpeed);
    currentSpeed = max(0, currentSpeed);
    if (normalSpeed == currentSpeed)
        sprintf(g_text, "%d", normalSpeed);
    else
        sprintf(g_text, "%d(%d)", normalSpeed, currentSpeed);

    m_widgets.push_back(new textWidget(
        154, 161, 122, 17, g_text, "smalfont.fnt",
        font::PRIMARY, SPEED_ID, 6, 0, 8));
}

// DC 0x1927ea stores the member before allocation; 0x1927f6 reloads it for
// limit. Retail's battle constructor likewise reloads +0x68 after allocation.
// The helper owns this store; caching the getter result in each caller loses
// that reload. The group constructor uses the same canonical helper.
inline void TViewArmyWindow::createMoraleWidget(int newMorale)
{
    m_morale = newMorale;
    m_widgets.push_back(new iconWidget(
        23, 189, 42, 38, MORALE_ID,
        DATA_COMPGEN(0x0068c68c, viewArmyMoraleIcons, "imrl42.def"),
        limit(-3, m_morale, 3) + 3, 0, 0, 0, 0x10));
}

// DC 0x1928a2/0x1928ae and retail's battle +0x7c access prove the same
// store-then-reload ownership as createMoraleWidget.
inline void TViewArmyWindow::createLuckWidget(int newLuck)
{
    m_luck = newLuck;
    m_widgets.push_back(new iconWidget(
        77, 189, 42, 38, LUCK_ID,
        DATA_COMPGEN(0x0068c680, viewArmyLuckIcons, "ilck42.def"),
        limit(-3, m_luck, 3) + 3, 0, 0, 0, 0x10));
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

inline void TViewArmyWindow::createRolloverWidget()
{
    m_rolloverWidget = new bitmapBackedTextWidget(
        7, 285, 284, 19, 0, "smalfont.fnt",
        DATA_COMPGEN(0x0068c674, viewArmyRolloverBack, "VARBack.pcx"),
        font::PRIMARY, ROLLOVER_ID, 1, 8);
    m_widgets.push_back(m_rolloverWidget);
}

// COMDAT pairing: basic_string<char>::assign(const basic_string&, size_t,
// size_t). Mnemonic agreement is 1.000, and the identification was already
// proven from the other side: our own bitset<N>::_Xran emits a call to this
// exact decorated name at the position where retail calls 0x4860 (visible as
// a reloc-name-only row in that function's asm diff).
VA_COMPGEN(0x00404860, 0x210, BASIC_STRING_ASSIGN_STR, char)

VA_COMPGEN(0x00404bc0, 0x06, BASIC_STRING_MAX_SIZE, char)

// COMDAT pairing: basic_string<char>::_Split - the reference-count split that
// assign/_Freeze reach; a nullary private thiscall in the same Dinkumware
// block as _Tidy (0x40f0), _Grow (0x4a90) and the assign above.
VA_COMPGEN(0x00404ce0, 0xD8, BASIC_STRING_SPLIT, char)
