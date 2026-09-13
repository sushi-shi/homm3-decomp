// hillfortwindow.cpp - E:\gamedcs\hillfortwindow.cpp (compiland hillfortwindow.obj)
#include <stdio.h>
#include <string.h>

#include <va.h>
#include "advmgr.h"
#include "border.h"
#include "button.h"
#include "hillfortwindow.h"
#include "creaturetype.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "kb.h"
#include "message.h"
#include "recruit.h"
#include "mousemgr.h"
#include "soundmgr.h"
#include "textwdgt.h"
#include "textresource.h"
#include "viewarmywindow.h"
#include "widget.h"
#include "winmgr.h"

// Original file-static UpdateHillFort; definition follows the handler.
static void updateHillFort(unsigned char firstUpdate);

// Retail's constructor stores the active dialog here and its destructor
// clears the same slot before the widget-vector teardown.
DATA(0x00699194) static THillFortWindow* g_hillFortWindow;

// The last widget the rollover text was built for. Retail's own static:
// .data 0x67f184 seeded to -1, the dword immediately before this TU's
// aphlf4r/4g/4y string pool.
DATA(0x0067f184) static int g_hillFortHoverId = -1;

// Use game::GetCurrHero's canonical Game.h:991 body (dc 0x2ed4).
// The former TU-local copy only existed while another lane owned game.h.
// Recalculate/UpgradeSlot use the canonical Complete upgrade selector at
// 0x529710 as well, including the base-map elemental exclusion.

// Recalculate's DC lines 228/332 and UpgradeSlot's dc 0xd71d4 call
// IsBaseCreature directly. Complete adds the base-map elemental guards at
// 0x4e7f8d..0x4e7fbb, 0x4e8222..0x4e8240 and 0x4e858c..0x4e85ba.
// Keep those guards at the call sites. The former CanUpgradeCreature wrapper
// was introduced to influence VC6's load scheduling; that scheduling does not
// establish a source helper boundary. Each query snapshots its type before
// reading the map version, and keeps retail's low-byte test of the result.

// The three .rdata objects hillfortwindow.obj contributes, in retail's own
// order: the per-slot upgrade-button icons indexed by TUpgradeSlot::state,
// the same three colours for the upgrade-all button indexed by
// UpgradeAllButtonState, and the level multiplier Recalculate scales the
// gold cost by. The float row is bounded exactly - 0x63eb4c + 7*4 lands on
// the vftable at 0x63eb68.
DATA(0x0063eb34)
static const char* const g_aszUpgradeIcons[3] = {
    "aphlf1y.def", "aphlf1g.def", "aphlf1r.def"
};

DATA(0x0063eb40)
static const char* const g_aszUpgradeAllIcons[3] = {
    "aphlf4y.def", "aphlf4g.def", "aphlf4r.def"
};

DATA(0x0063eb4c)
const float g_afUpgradeCostFactor[7] = {
    0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.0f, 1.0f
};

VA(0x004e75f0, 0x7E9)  // dc 0xd641c
THillFortWindow::THillFortWindow()
    : heroWindow(0x32, 0x32, 0x28c, 0x15c, 2)
{
    g_hillFortWindow = this;
    hero* currentHero = g_game->getCurrHero();

    m_widgets.reserve(NWIDGETS);

    m_widgets.push_back(new bitmapBorder(
        0, 0, 0x28c, 0x15c, BACKGROUND_ID,
        DATA_COMPGEN(0x0067f1e0, hillFortBackground, "APhlftBk.pcx"),
        0x800));
    m_widgets.push_back(new textWidget(
        0, 0x14, 0x28c, 0x15c, g_adventureObjectNames[HILL_FORT],
        DATA_COMPGEN(0x00660b24, hillFortBigFont, "bigfont.fnt"),
        font::HEADING, TITLE_ID, font::CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new bitmapBorder(
        0x1e, 0x3c, 0x3a, 0x40, HERO_PORTRAIT_ID,
        g_heroTraits[currentHero->m_portrait].m_largePortraitName, 0x800));
    m_widgets.push_back(new bitmapBackedTextWidget(
        7, 0x142, 0x27d, 0x13, 0,
        DATA_COMPGEN(0x0065f2f8, hillFortSmallFont, "smalfont.fnt"),
        DATA_COMPGEN(0x0067f1d0, hillFortRolloverBackground,
                     "APhlftrt.pcx"),
        font::PRIMARY, ROLLOVER_ID, font::CENTER_JUSTIFIED, 8));
    m_rolloverWidget = m_widgets.back();

    m_widgets.push_back(new bitmapBorder(
        0x125, 0x112, 0x42, 0x20, BACKGROUND_ID,
        DATA_COMPGEN(0x0067016c, hillFortOkayBorder, "Box64x30.pcx"),
        0x800));

    button* okay = new button(
        0x126, 0x113, 0x40, 0x1e, DIALOG_RETURN_OK,
        DATA_COMPGEN(0x00670160, hillFortOkayButton, "iOkay.def"),
        0, 1, 0, 0, 2);
    okay->setHotkey(1);
    okay->setHotkey(0x1c);
    m_widgets.push_back(okay);

    int i;
    int x = 0x68;
    for (i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; x += 0x4c, ++i) {
        int id = CREATURE_PORTRAIT_1_ID + i;
        int numId = CREATURE_NUM_1_ID + i;
        int goldIconId = GOLD_ICON_1_ID + i;
        int goldCostId = GOLD_COST_1_ID + i;
        int resIconId = RES_ICON_1_ID + i;
        int resCostId = RES_COST_1_ID + i;
        int numY = 0x7c - g_calligraphicFont->m_fs.m_height;

        m_widgets.push_back(new iconWidget(
            x + 3, 0x3c, 0x3a, 0x40, id,
            DATA_COMPGEN(0x006601e0, hillFortCreatureSprite,
                         "twcrport.def"),
            0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
        m_widgets.push_back(new textWidget(
            x + 3, numY, 0x3a, 0x40, "",
            DATA_COMPGEN(0x006700b4, hillFortCountFont, "Verd10B.fnt"),
            font::PRIMARY, numId, font::RIGHT_JUSTIFIED, 0, 8));
        m_widgets.push_back(new iconWidget(
            x, 0x80, 0x3e, 0x12, goldIconId,
            DATA_COMPGEN(0x00660cd4, hillFortResourceSprite,
                         "smalres.def"),
            6, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
        m_widgets.push_back(new textWidget(
            x, 0x80, 0x3e, 0x12, "",
            DATA_COMPGEN(0x0065f2f8, hillFortSmallFont, "smalfont.fnt"),
            font::PRIMARY, goldCostId, font::RIGHT_JUSTIFIED, 0, 8));
        m_widgets.push_back(new iconWidget(
            x, 0x94, 0x3e, 0x12, resIconId,
            DATA_COMPGEN(0x00660cd4, hillFortResourceSprite,
                         "smalres.def"),
            6, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
        m_widgets.push_back(new textWidget(
            x, 0x94, 0x3e, 0x12, "",
            DATA_COMPGEN(0x0065f2f8, hillFortSmallFont, "smalfont.fnt"),
            font::PRIMARY, resCostId, font::RIGHT_JUSTIFIED, 0, 8));
    }

    x = 0x68;
    for (i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; x += 0x4c, ++i) {
        m_widgets.push_back(new iconWidget(
            x, 0xed, 0x3e, 0x14, TOTAL_RES_ICON_1_ID + i,
            DATA_COMPGEN(0x00660cd4, hillFortResourceSprite,
                         "smalres.def"),
            i, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
        m_widgets.push_back(new textWidget(
            x, 0xed, 0x3e, 0x14, "",
            DATA_COMPGEN(0x0065f2f8, hillFortSmallFont, "smalfont.fnt"),
            font::PRIMARY, TOTAL_RES_COST_1_ID + i,
            font::RIGHT_JUSTIFIED, 0, 8));
    }

    x = 0x6b;
    for (i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; x += 0x4c, ++i) {
        button* upgrade = new button(
            x, 0xab, 0x3a, 0x1d, UPGRADE_BUTTON_1_ID + i,
            g_aszUpgradeIcons[UPGRADE_STATE_TOO_EXPENSIVE], 0, 1, 0, 0, 2);
        upgrade->setHotkey(i + 2);
        m_widgets.push_back(upgrade);
    }

    button* upgradeAll = new button(
        0x1e, 0xe8, 0x3a, 0x1d, UPGRADE_ALL_BUTTON_ID,
        DATA_COMPGEN(0x0067f1a0, hillFortUpgradeAllButton, "aphlf4y.def"),
        0, 1, 0, 0, 2);
    upgradeAll->setHotkey(0x1e);
    m_widgets.push_back(upgradeAll);

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    updateHillFort(1);
}

VA_COMPGEN(0x004e7de0, 0x21, SCALAR_DELETING_DTOR, THillFortWindow)

VA(0x004e7e10, 0x75)  // dc 0xd6b2c
THillFortWindow::~THillFortWindow()
{
    g_hillFortWindow = 0;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\hillfortwindow.cpp:178
#if 0  // @carcass: retail inlines this switch into the dialog handler
DC_ONLY(0xd6b94, 0x1A)
int THillFortWindow::convertID2HelpID(int id)
{
    // @stub
}
#endif

VA(0x004e7e90, 0x1F)  // dc 0xd6bb0
void THillFortWindow::doModal()
{
    recalculate(0);
    g_windowManager->doDialog(this, hillFortWindowHandler, 0);
}

// E:\gamedcs\hillfortwindow.cpp:192
DC_ONLY(0xd6bd4, 0x22)
inline bool canAfford(const long* cost, const long* playerRes)
{
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        if (cost[i] > playerRes[i])
            return 0;
    }
    return 1;
}

// E:\gamedcs\hillfortwindow.cpp:203
// Residual (82.2872%, 2026-08-21): the CFG is closed at 34 branches / one
// return on both sides, with every mnemonic and symbolic target agreeing.
// Restoring the CodeView-named CanAfford helper below removed our redundant
// post-loop `a == 7` test (79.6984 -> 80.0868); putting the resourceIndex ==
// -1 arm first, as retail's sole polarity difference required, then reached
// 82.2872. Both sides now reserve the same 0x40-byte frame.

// The dominant remainder is one cascading strength-reduction choice. Retail
// homes the 0xd4-based widget ID in [ebp-8] and bases the slot induction on
// slot[i].count (this+0x90, `add ebx,0x50`). This compile homes the slot index
// and bases the induction on slot[i].cost[6] (this+0x84), adding ID LEAs and
// shifting most member displacements by 0xc. The out-of-line ledger is 23 vs
// retail's 24, solely BroadcastMessage x14 vs x15. Raw disassembly proves
// this is not an expanded callee: our C2 tail-merges the identical occupied-
// and empty-arm upgrade-button update, while retail keeps two call sites.

// Dreamcast's loop block names six ID locals, in emission order: res_cost_id,
// res_icon_id, button_id, gold_icon_id, num_id, gold_cost_id; it names no i.
// Reintroducing all six beside the current i/reference scores 79.60, and a
// num_id-driven no-i form scores 79.08. Adding only button_id scores 77.7066
// and still leaves the merged call. Earlier bounded loop forms: direct
// slot[i] (78.83), id <= CREATURE_NUM_7_ID (78.66), separate i/id induction
// (77.12), and id-driven without the reference (68.18). The retained
// i/id/reference form is therefore the measured x86 winner. The clear order
// at entry is byte-proven: szGoldCost, szResourceCost, resourceIndex, szCount.
// 2026-09-06, polish lane 22 (82.2872 -> 83.9500): the affordability verdict
// is NOT a ternary. Retail stores UPGRADE_STATE_TOO_EXPENSIVE and
// UPGRADE_STATE_AFFORDABLE as two separate constant stores ON THE LOOP'S
// TWO EXITS (`jg -> mov [edi+0x4c],2` and loop-completion ->
// `mov [edi+0x4c],1`), with no post-loop compare at all; the ternary makes
// VC6 fold the verdict into arithmetic on the counter (`inc edx / mov
// [edi+0x4c],edx`). Assigning inside the loop and `goto`-ing past the
// fall-through assignment reproduces retail's threading and takes the branch
// census clean at 34/34. Measured: plain `if/else` after the loop 83.24 (one
// branch too many), `continue` instead of the trailing `else` byte-flat at
// 83.95, `int` flags 83.23, memset before the flags 83.42, mixed int/uchar
// flags 83.30 and 83.79, swapping the two flag declarations 83.96 (noise).
// Residual (83.95%): the merged BroadcastMessage the note above describes is
// still merged - retail's 25th call at fn+0x405 is the occupied arm's
// SET_ICON_NAME, ours cross-jumps into the empty arm's SET_STATUS at fn+0x4e4
// (`push eax / jmp`) - and retail additionally SHARES one inline strcpy tail
// between two emptyRolloverText copies where we duplicate it, so the two
// cross-jump decisions run in OPPOSITE directions in one body. Also open:
// retail's slot induction pointer sits at &slot[i]+0x44, ours at +0x40, which
// is what shifts every `[ebx +/- N]` displacement by four.
// Residual (84.0329%), 2026-09-06.  Structure is now exact: 62/62 blocks
// with 47 of them byte-identical (it was 3), 34/34 branches and no missing
// block; the one remaining one-sided call is the if-arm's own
// SET_ICON_NAME BroadcastMessage, whose two-instruction tail
// (`mov ecx,esi / call`) our cross-jumper merges into the else arm's last
// call site where retail keeps both.  What is left is TWO INDUCTION-ANCHOR BIASES that shift
// a displacement on every touched instruction and nothing else:
//   * the widget id lives at [ebp-8] as 0xcd (CREATURE_PORTRAIT_1_ID) where
//     retail parks 0xd4 (CREATURE_NUM_1_ID), so every codeY is +7 off;
//   * the slot cursor is `esi+0x8c` (anchored on `s.type`, record +0x40)
//     where retail uses `esi+0x90` (anchored on `s.count`, record +0x44),
//     so every `[ebx+N]` is 4 off.
// Both are C2 anchor choices over the SAME effective addresses.  Measured
// and rejected against 84.0329: spelling every codeY with its absolute
// enum name (CREATURE_NUM_1_ID + i, GOLD_COST_1_ID + i, ...) and dropping
// `id` - 83.9081; keeping `msg.codeY = id - 7;` for the portrait rather than
// `CREATURE_PORTRAIT_1_ID + i` - 83.9081; moving `msg.codeX` after
// `msg.extraText` in the SET_ICON_NAME statement - byte-flat; and carrying
// `totalID` the same way in the totals loop below - 83.4988, so the lever is
// specific to the loop whose derived id feeds six different widget bands.

// 2026-09-06, polish lane 36 (84.0331 -> 95.2727), the DC LOCAL-SCOPE SWEEP.
// The Dreamcast procedure block declares SEVEN per-iteration id variables at
// hillfortwindow.cpp:271..277, one statement each, computed unconditionally
// BEFORE the `s.type != CREATURE_NONE && s.count > 0` gate at :279 - six of
// them named in the CodeView local list (num_id sp+0x18, res_icon_id sp+0x14,
// button_id sp+0x1c, res_cost_id sp+0x20, gold_icon_id sp+0x24, gold_cost_id
// sp+0x28) and the seventh, the portrait id, register-allocated into r4 and
// consumed at :284.  The SH4 constants close the identification exactly:
// :271 K1, :272 K1+7, :273 K1+14 with K1 = CREATURE_PORTRAIT_1_ID = 205, then
// :274 K2 = GOLD_COST_1_ID, :275 K2+7 = RES_ICON_1_ID, :276 K3 =
// RES_COST_1_ID, :277 K3+21 = UPGRADE_BUTTON_1_ID.  Writing all seven and
// DROPPING the carried `id` took the row 84.0331 -> 94.0227 in one edit (54
// exact blocks, then 59 after the delink refresh); VC6 still CSEs them back
// onto retail's single [ebp-8] home and re-derives `+0xe` from it, which is
// why the earlier note's "reintroducing all six BESIDE the current i/reference
// scores 79.60" measured something else - that probe kept `id`.
// The state verdict is the DC's `CanAfford` call, not the hand-written loop:
// :332 `if (!IsBaseCreature(...)) state = 0;` / :337 `else if (CanAfford(
// s.cost, gpCurPlayer->resources)) state = 1;` / :345 `else state = 2;`,
// worth a further 94.0227 -> 95.2727 and it retires the `goto have_state`.
// That call needs `TUpgradeSlot::cost` to be `long[7]` - the DC CanAfford
// takes `const long*` - so the whole `get_upgrade_cost` out-parameter family
// (recruit.h/.cpp plus the four caller-local `cost` arrays in viewarmywindow,
// game and philai) widened with it; that retype is byte-flat tree-wide, only
// the mangled name moves (PAH -> PAJ).
// The DC line table orders the entry clears szCount, szGoldCost,
// szResourceCost, resourceIndex; that order is byte-flat against the previous
// one, so the older note's "byte-proven" claim for the other order is only a
// statement that both compile the same.
// Residual (95.2727%): three size-only blocks and the SAME single anchor bias
// the note above names - retail's slot cursor is `esi+0x90` (&slot[0].count),
// ours `esi+0x8c` (&slot[0].type), shifting every `[ebx-N]` by four.  The
// widget-id half of that bias is now GONE (both sides home 0xd4 at [ebp-8]).
VA(0x004e7eb0, 0x64D)  // source/call order + DoModal/handler call sites, dc 0xd6bf8
void THillFortWindow::recalculate(unsigned char drawDimmedButtons)
{
    message msg;
    msg.m_id = 0;
    msg.m_codeX = 0;
    msg.m_codeY = 0;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_extra = 0;
    msg.m_window = 0;

    hero* currHero = g_game->getCurrHero();

    unsigned char upgradeAllValid = 0;
    unsigned char allUpgraded = 1;
    memset(m_totalCost, 0, sizeof m_totalCost);

    // Retail carries `id` itself as the loop's induction variable and
    // recovers `i` for the test (`inc edi / mov [ebp-8],edi /
    // add edi,-0xd4 / cmp edi,7 / jl`); computing `id` from `i` inside the
    // body instead leaves `i` memory-homed and every widget id rebuilt with
    // its own `lea`, which cost 44 of the 62 blocks.
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        TUpgradeSlot& s = m_slot[i];
        s.m_countText[0] = 0;
        s.m_goldCost[0] = 0;
        s.m_resourceCost[0] = 0;
        s.m_resourceIndex = -1;
        s.m_type = currHero->m_army.m_armyTypes[i];
        s.m_count = currHero->m_army.m_numTroops[i];
        s.m_level = g_creatureTypeTraits[s.m_type].m_level;
        sprintf(s.m_countText, "%d", s.m_count);
        memset(s.m_cost, 0, sizeof s.m_cost);

        TCreatureType type = s.m_type;
        if ((g_game->m_f1f698 != 0
             || (type != CREATURE_AIR_ELEMENTAL && type != CREATURE_EARTH_ELEMENTAL
                 && type != CREATURE_FIRE_ELEMENTAL
                 && type != CREATURE_WATER_ELEMENTAL))
            && static_cast<unsigned char>(isBaseCreature(type))) {
            upgradeAllValid = 1;
            allUpgraded = 0;
            if (s.m_level == 0) {
                strcpy(s.m_goldCost, (*g_generalText)[345]);
                strcpy(s.m_resourceCost, g_emptyRolloverText);
            } else {
                TCreatureType upgraded;
                {
                    TCreatureType baseType = s.m_type;
                    upgraded = g_game->upgradedCreatureType(baseType);
                }
                {
                    TCreatureType baseType = s.m_type;
                    getUpgradeCost(baseType, upgraded, s.m_count, s.m_cost);
                }
                s.m_cost[6] = static_cast<int>(
                    s.m_cost[6] * g_afUpgradeCostFactor[s.m_level]);
                m_totalCost[6] += s.m_cost[6];
                for (int r = 0; r < 6; r++) {
                    if (s.m_cost[r]) {
                        m_totalCost[r] += s.m_cost[r];
                        s.m_resourceIndex = r;
                    }
                }
                sprintf(s.m_goldCost, "%d", s.m_cost[6]);
                if (s.m_resourceIndex == -1)
                    sprintf(s.m_resourceCost, g_emptyRolloverText);
                else
                    sprintf(s.m_resourceCost, "%d",
                            s.m_cost[s.m_resourceIndex]);
            }
        } else {
            strcpy(s.m_goldCost, (*g_generalText)[346]);
        }

        int portraitId = CREATURE_PORTRAIT_1_ID + i;
        int numId = CREATURE_NUM_1_ID + i;
        int goldIconId = GOLD_ICON_1_ID + i;
        int goldCostId = GOLD_COST_1_ID + i;
        int resIconId = RES_ICON_1_ID + i;
        int resCostId = RES_COST_1_ID + i;
        int buttonId = UPGRADE_BUTTON_1_ID + i;

        if (s.m_type != CREATURE_NONE && s.m_count > 0) {
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
            msg.m_codeY = portraitId;
            msg.m_extra = s.m_type + 2;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_TEXT;
            msg.m_codeY = numId;
            msg.m_extraText = s.m_countText;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_TEXT;
            msg.m_codeY = goldCostId;
            msg.m_extraText = s.m_goldCost;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            if (s.m_resourceIndex == -1) {
                msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
                msg.m_codeY = resIconId;
                msg.m_extra = widget::WIDGET_CLEAR_STATUS;
                broadcastMessage(msg);
            } else {
                msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
                msg.m_codeY = resIconId;
                msg.m_extra = s.m_resourceIndex;
                broadcastMessage(msg);

                msg.m_id = MESSAGE_WIDGET;
                msg.m_codeX = widget::WIDGET_SET_TEXT;
                msg.m_codeY = resCostId;
                msg.m_extraText = s.m_resourceCost;
                broadcastMessage(msg);
            }

            TCreatureType stateType = s.m_type;
            if ((g_game->m_f1f698 == 0
                 && (stateType == CREATURE_AIR_ELEMENTAL
                     || stateType == CREATURE_EARTH_ELEMENTAL
                     || stateType == CREATURE_FIRE_ELEMENTAL
                     || stateType == CREATURE_WATER_ELEMENTAL))
                || !static_cast<unsigned char>(isBaseCreature(stateType))) {
                s.m_state = UPGRADE_STATE_NONE;
            } else if (canAfford(s.m_cost, g_currentPlayer->m_resources)) {
                s.m_state = UPGRADE_STATE_AFFORDABLE;
            } else {
                s.m_state = UPGRADE_STATE_TOO_EXPENSIVE;
            }

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_ICON_NAME;
            msg.m_codeY = buttonId;
            msg.m_extraText = g_aszUpgradeIcons[s.m_state];
            broadcastMessage(msg);
        } else {
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_codeY = portraitId;
            msg.m_extra = widget::WIDGET_CLEAR_STATUS;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_codeY = numId;
            msg.m_extra = widget::WIDGET_CLEAR_STATUS;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_codeY = goldIconId;
            msg.m_extra = widget::WIDGET_CLEAR_STATUS;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_codeY = resIconId;
            msg.m_extra = widget::WIDGET_CLEAR_STATUS;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_STATUS;
            msg.m_codeY = buttonId;
            msg.m_extra = drawDimmedButtons ? widget::WIDGET_DIMMED
                                          : widget::WIDGET_DIMMED_NODRAW;
            broadcastMessage(msg);
        }
    }

    char totalCostText[10];
    for (int t = 0; t < armyGroup::ARMY_GROUP_SLOT_COUNT; t++) {
        int totalID = TOTAL_RES_COST_1_ID + t;
        if (m_totalCost[t] > 0) {
            sprintf(totalCostText, "%d", m_totalCost[t]);
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_TEXT;
            msg.m_codeY = totalID;
            msg.m_extraText = totalCostText;
            broadcastMessage(msg);
        } else {
            strcpy(totalCostText, g_emptyRolloverText);
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_TEXT;
            msg.m_codeY = totalID;
            msg.m_extraText = totalCostText;
            broadcastMessage(msg);

            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_codeY = totalID - 7;
            msg.m_extra = widget::WIDGET_CLEAR_STATUS;
            broadcastMessage(msg);
        }
    }

    if (canAfford(m_totalCost, g_currentPlayer->m_resources) && upgradeAllValid)
        m_upgradeAllButtonState = UPGRADE_STATE_AFFORDABLE;
    else if (allUpgraded)
        m_upgradeAllButtonState = UPGRADE_STATE_NONE;
    else
        m_upgradeAllButtonState = UPGRADE_STATE_TOO_EXPENSIVE;

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_ICON_NAME;
    msg.m_codeY = UPGRADE_ALL_BUTTON_ID;
    msg.m_extraText = g_aszUpgradeAllIcons[m_upgradeAllButtonState];
    broadcastMessage(msg);
}

VA(0x004e8500, 0x18F)  // dc 0xd7124
void THillFortWindow::upgradeSlot(int which, unsigned char showMessage)
{
    switch (m_slot[which].m_state) {
    case UPGRADE_STATE_NONE:
        if (showMessage)
            normalDialog((*g_generalText)[314], 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        break;

    case UPGRADE_STATE_AFFORDABLE:
        if (m_slot[which].m_type != CREATURE_NONE && m_slot[which].m_count > 0) {
            TCreatureType type = m_slot[which].m_type;
            if ((g_game->m_f1f698 != 0
                 || (type != CREATURE_AIR_ELEMENTAL
                     && type != CREATURE_EARTH_ELEMENTAL
                     && type != CREATURE_FIRE_ELEMENTAL
                     && type != CREATURE_WATER_ELEMENTAL))
                && static_cast<unsigned char>(isBaseCreature(type))) {
                TCreatureType upgraded;
                {
                    TCreatureType baseType = m_slot[which].m_type;
                    upgraded = g_game->upgradedCreatureType(baseType);
                }

                g_game->getCurrHero()->m_army.m_armies[which] = upgraded;
                for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++)
                    g_currentPlayer->m_resources[i] -= m_slot[which].m_cost[i];
            }
        }
        break;

    case UPGRADE_STATE_TOO_EXPENSIVE:
        if (showMessage)
            normalDialog((*g_generalText)[315], 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    }
}

// E:\gamedcs\hillfortwindow.cpp:500..519. Original: UpgradeAll.
// DC504/508 owns the two dialogs; 514..515 loops over seven UpgradeSlot
// calls. Retail expands this ordinary helper in the handler at 0x4e8944.
// No retained retail row is required for the source boundary to exist.
void THillFortWindow::upgradeAll()
{
    switch (m_upgradeAllButtonState) {
    case UPGRADE_STATE_NONE:
        normalDialog((*g_generalText)[316], 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case UPGRADE_STATE_TOO_EXPENSIVE:
        normalDialog((*g_generalText)[317], 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case UPGRADE_STATE_AFFORDABLE:
        for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++)
            upgradeSlot(i, 0);
        break;
    }
}

VA(0x004e8690, 0x1B8)  // dc 0xd72f4
void THillFortWindow::handleClick(message& msg)
{
    unsigned char rightClick =
        (static_cast<unsigned int>(msg.m_qualifier) >> 9) & 1;

    switch (msg.m_codeY) {
    case HERO_PORTRAIT_ID:
        sprintf(g_text,
                (*g_generalText)[GENERAL_TEXT_HERO_ROLLOVER_FORMAT],
                g_game->getCurrHero()->m_name, g_game->getCurrHero()->heroFn004D8F70());
        if (rightClick)
            normalDialog(g_text, 4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        else
            normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;

    case CREATURE_NUM_1_ID:
    case CREATURE_NUM_2_ID:
    case CREATURE_NUM_3_ID:
    case CREATURE_NUM_4_ID:
    case CREATURE_NUM_5_ID:
    case CREATURE_NUM_6_ID:
    case CREATURE_NUM_7_ID:
        msg.m_codeY -= CREATURE_NUM_1_ID - CREATURE_PORTRAIT_1_ID;
        // fall through - the count row shares the portrait row's popup
    case CREATURE_PORTRAIT_1_ID:
    case CREATURE_PORTRAIT_2_ID:
    case CREATURE_PORTRAIT_3_ID:
    case CREATURE_PORTRAIT_4_ID:
    case CREATURE_PORTRAIT_5_ID:
    case CREATURE_PORTRAIT_6_ID:
    case CREATURE_PORTRAIT_7_ID:
        {
            int creature =
                g_hillFortWindow->getCreatureType(msg.m_codeY - CREATURE_PORTRAIT_1_ID);
            if (creature == CREATURE_NONE)
                break;
            TViewArmyWindow viewArmy(creature, 0x77, 0x20, !rightClick);
            viewArmy.centerWindow(-1, -1);
            if (rightClick)
                viewArmy.quickView();
            else
                viewArmy.doModal();
        }
        break;
    }
}

// E:\gamedcs\hillfortwindow.cpp:612
// EXACT 2026-09-12 with the ordinary UpgradeAll/UpdateHillFort helpers,
// typed GetCreatureType accessor, and the two original dispatch flags.
// DC616 initializes the close flag; 645 sets it; 749 tests it before the
// final stores at 751..754. DC649 initializes a scoped refresh flag, both
// upgrade arms set it at 662/666, and 670 guards UpdateHillFort at 671.
// The seven-state family emits three objects: bool/uchar/int if-chains all
// reach 100%; outer switches score 90.4966%. All seven siblings stay exact.
//
// Historical 91.9430% early-return forms lost the deferred close scope.
// Tail labels, moving the accept block and separate DIALOG_RETURN_OK cases
// stayed at 91.56..91.94. A fake second predecessor reached 95.96 but changed
// behavior and was rejected. Restoring the actual flags naturally puts the
// closing block last and recovers both dialog-call register schedules.
VA(0x004e8850, 0x369)  // DoModal address-take, dc 0xd7458
int hillFortWindowHandler(message& msg)
{
    pollSound();
    bool closeWindow = 0;

    if (msg.m_id == MESSAGE_KEY_DOWN) {
    } else if (msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
        case widget::WIDGET_RIGHT_SELECT:
            g_hillFortWindow->handleClick(msg);
            break;
        case widget::WIDGET_DESELECT:
            if (msg.m_codeY == DIALOG_RETURN_OK) {
                closeWindow = 1;
            } else {
                bool refreshWindow = 0;
                switch (msg.m_codeY) {
                case THillFortWindow::UPGRADE_BUTTON_1_ID:
                case THillFortWindow::UPGRADE_BUTTON_2_ID:
                case THillFortWindow::UPGRADE_BUTTON_3_ID:
                case THillFortWindow::UPGRADE_BUTTON_4_ID:
                case THillFortWindow::UPGRADE_BUTTON_5_ID:
                case THillFortWindow::UPGRADE_BUTTON_6_ID:
                case THillFortWindow::UPGRADE_BUTTON_7_ID:
                    g_hillFortWindow->upgradeSlot(
                        msg.m_codeY - THillFortWindow::UPGRADE_BUTTON_1_ID, 1);
                    refreshWindow = 1;
                    break;

                case THillFortWindow::UPGRADE_ALL_BUTTON_ID:
                    g_hillFortWindow->upgradeAll();
                    refreshWindow = 1;
                    break;

                default:
                    break;
                }
                if (refreshWindow)
                    updateHillFort(0);
            }
            break;
        }
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = g_hillFortWindow->findWidget(msg.m_mouseX, msg.m_mouseY);
        if (hoverID == g_hillFortHoverId)
            return MESSAGE_DISPATCH_CONSUME;

        g_hillFortHoverId = hoverID;
        g_mouseManager->setPointer(1, mouseManager::DEFAULT_SET);

        switch (hoverID) {
        case THillFortWindow::HERO_PORTRAIT_ID:
            sprintf(g_text,
                    (*g_generalText)[GENERAL_TEXT_HERO_ROLLOVER_FORMAT],
                    g_game->getCurrHero()->m_name, g_game->getCurrHero()->heroFn004D8F70());
            msg.m_extraText = g_text;
            break;

        case THillFortWindow::UPGRADE_ALL_BUTTON_ID:
            msg.m_extraText = (*g_generalText)[433];
            break;

        case THillFortWindow::CREATURE_PORTRAIT_1_ID:
        case THillFortWindow::CREATURE_PORTRAIT_2_ID:
        case THillFortWindow::CREATURE_PORTRAIT_3_ID:
        case THillFortWindow::CREATURE_PORTRAIT_4_ID:
        case THillFortWindow::CREATURE_PORTRAIT_5_ID:
        case THillFortWindow::CREATURE_PORTRAIT_6_ID:
        case THillFortWindow::CREATURE_PORTRAIT_7_ID:
            msg.m_extraText = g_creatureTypeTraits[
                g_hillFortWindow->getCreatureType(
                    hoverID - THillFortWindow::CREATURE_PORTRAIT_1_ID)].m_pluralName;
            break;

        case THillFortWindow::UPGRADE_BUTTON_1_ID:
        case THillFortWindow::UPGRADE_BUTTON_2_ID:
        case THillFortWindow::UPGRADE_BUTTON_3_ID:
        case THillFortWindow::UPGRADE_BUTTON_4_ID:
        case THillFortWindow::UPGRADE_BUTTON_5_ID:
        case THillFortWindow::UPGRADE_BUTTON_6_ID:
        case THillFortWindow::UPGRADE_BUTTON_7_ID:
            sprintf(g_text, (*g_generalText)[319],
                    g_creatureTypeTraits[
                        g_hillFortWindow->getCreatureType(
                            hoverID - THillFortWindow::UPGRADE_BUTTON_1_ID)].m_pluralName);
            msg.m_extraText = g_text;
            break;

        default:
            g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
            msg.m_extraText = g_emptyRolloverText;
            break;
        }

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_SET_TEXT;
        msg.m_codeY = THillFortWindow::ROLLOVER_ID;
        g_hillFortWindow->broadcastMessage(msg);
        g_hillFortWindow->drawWindow(1, THillFortWindow::ROLLOVER_ID,
                                     THillFortWindow::ROLLOVER_ID);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (closeWindow) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\hillfortwindow.cpp:763..780. Original: UpdateHillFort.
// DC764 branches on firstUpdate: 766..773 builds the initial palette message;
// 777..778 recalculates and redraws. These are the constructor's final block
// and the handler's post-upgrade block in retail. The older note incorrectly
// described this helper as constructing the window. It has no retail row:
// 0x4e8fb0 belongs to hiscore, beyond this TU's end at 0x4e8bb9.
static void updateHillFort(unsigned char firstUpdate)
{
    if (firstUpdate) {
        message msg;
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_SET_PLAYER_PALETTE_COLORS;
        msg.m_codeY = THillFortWindow::BACKGROUND_ID;
        msg.m_extra = g_game->getLocalPlayerGamePos();
        g_hillFortWindow->broadcastMessage(msg);
    } else {
        g_hillFortWindow->recalculate(1);
        g_hillFortWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                     WINDOW_ALL_WIDGETS_HIGH);
    }
}
