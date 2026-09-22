#include "va.h"

#include "sacrifice_window.h"

#include "border.h"
#include "button.h"
#include "creaturetype.h"
#include "game.h"
#include "kb.h"
#include "message.h"
#include "misc.h"
#include "mousemgr.h"
#include "resourcemanager.h"
#include "sample.h"
#include "slider.h"
#include "soundmgr.h"
#include "spellbookwindow.h"
#include "textresource.h"
#include "viewarmywindow.h"
#include "winmgr.h"

static const TCreatureType g_deathCreature[145] = {
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON,
    CREATURE_BONE_DRAGON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON
};

// Complete adds the nineteenth SoD equipment slot to DC's 18-pair table.
static const long g_slotDefinitions[19][2] = {
    {143, 18}, {202, 230}, {143, 68}, {17, 57}, {196, 172},
    {143, 119}, {65, 57}, {244, 172}, {149, 283}, {17, 131},
    {33, 181}, {49, 232}, {65, 283}, {198, 18}, {244, 18},
    {244, 64}, {244, 110}, {244, 299}, {15, 283}
};

static const long g_rowStart[5][2] = {
    {314, 50}, {314, 120}, {314, 190}, {314, 260}, {395, 330}
};
static const long g_rowSize[5] = {5, 5, 5, 5, 2};

static const long g_constCreatureSources[2][2] = {
    {45, 109}, {128, 305}
};
static const long g_constSourceSizes[2][2] = {
    {3, 2}, {1, 1}
};
static const long g_constCreatureOfferings[2][2] = {
    {334, 109}, {417, 305}
};

VA(0x0055fc30, 0xab)  // dc 0x123e8c
void type_artifact_offering::set(const type_artifact* artifact, long slot,
                                 const hero* owner)
{
    long artifactClass =
        g_artifactTraits[artifact->m_artifactId].m_artifactClass;
    m_artifactId = artifact->m_artifactId;
    m_extra = artifact->m_extra;
    m_source = slot;
    m_value = 0;

    switch (artifactClass) {
    case SACRIFICE_ARTIFACT_CLASS_TREASURE:
        m_value = 1000;
        break;
    case SACRIFICE_ARTIFACT_CLASS_MINOR:
        m_value = 1500;
        break;
    case SACRIFICE_ARTIFACT_CLASS_MAJOR:
        m_value = 3000;
        break;
    case SACRIFICE_ARTIFACT_CLASS_RELIC:
        m_value = 6000;
        break;
    }
    m_value = static_cast<long>(
        m_value * owner->getExperienceBonusFactor());
}

// E:\gamedcs\sacrifice_window.cpp:170
// All Complete calls are expanded into the artifact widget builder.
inline type_doll_slot_widget::type_doll_slot_widget(
    const type_doll_slot_definition& def, long id)
    : iconWidget(def.m_x, def.m_y, def.m_width, def.m_height, id, def.m_image,
                 0, 0, 0, 0, 16)
{
    m_slot = def.m_slot;
}

// E:\gamedcs\sacrifice_window.cpp:204
// All Complete calls are expanded into the artifact widget builder.
inline type_backpack_slot_widget::type_backpack_slot_widget(
    const type_icon_definition& def, long newSlot, long id)
    : iconWidget(def.m_x, def.m_y, def.m_width, def.m_height, id, def.m_image,
                 0, 0, 0, 0, 16)
{
    m_slot = newSlot;
}

// E:\gamedcs\sacrifice_window.cpp:235
// All Complete calls are expanded into the artifact widget builder.
inline type_artifact_offering_widget::type_artifact_offering_widget(
    long x, long y, long width, long height, long newItemNumber,
    long id, const char* image)
    : iconWidget(x, y, width, height, id, image, 0, 0, 0, 0, 16)
{
    m_itemNumber = newItemNumber;
}

// E:\gamedcs\sacrifice_window.cpp:178
// The doll-slot twin of the two below: one carve row ahead of them, in the
// Dreamcast roster's own order (doll, backpack, offering, army), same 0x26
// body shape, and forwarding to artifact_click - the equipped-slot handler.
// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fce0, 0x26)  // linkorder + iconWidget parent/+0x48 read, dc 0x123f88
bool type_doll_slot_widget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<type_sacrifice_window*>(m_parentWindow)->artifactClick(
            m_slot, rightClick);
        return 1;
    }
    return 0;
}

// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fd10, 0x26)
bool type_backpack_slot_widget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<type_sacrifice_window*>(m_parentWindow)->backpackClick(
            m_slot, rightClick);
        return 1;
    }
    return 0;
}

// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fd40, 0x26)
bool type_artifact_offering_widget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<type_sacrifice_window*>(m_parentWindow)->offeringClick(
            m_itemNumber, rightClick);
        return 1;
    }
    return 0;
}

// Vtable 0x64165c slot 0 is the last of four slot-widget address-takers for
// this ICF representative. Its call target is the 0x5654b0 representative
// immediately before type_transformer_slot::handle_click, fixing the
// canonical source identity despite the three earlier folded aliases.
// E:\gamedcs\sacrifice_window.cpp:1894
VA_COMPGEN(0x0055fd70, 0x21, SCALAR_DELETING_DTOR,
           type_transformer_slot)

// E:\gamedcs\sacrifice_window.cpp:272
// The army-slot member of the same family, 0x2a rather than 0x26 because it
// forwards a THIRD value: the byte at +0x4c alongside the dword at +0x48.
// That is creature_click's (slot, right_click, left_pane) exactly, and the
// pair matches the Dreamcast constructor's (new_slot, _left_pane).
// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fda0, 0x2a)  // linkorder + the +0x48/+0x4c pair, dc 0x12416c
bool type_army_slot_widget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<type_sacrifice_window*>(m_parentWindow)->creatureClick(
            m_slot, rightClick, m_leftPane);
        return 1;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:263
// Complete expands both calls in create_creature_icons and retains no
// separately claimable copy. The base constructor arguments and the three
// trailing stores are byte-proven by those two expansions.
type_army_slot_widget::type_army_slot_widget(
    long newX, long newY, long newW, long newH, long newSlot,
    long newId, const char* image, unsigned char newLeftPane)
    : iconWidget(newX, newY, newW, newH, newId, image,
                 0, 0, 0, 0, 16)
{
    m_slot = newSlot;
    m_leftPane = newLeftPane;
}

VA(0x0055fdd0, 0x574)  // dc 0x12419c
type_sacrifice_window::type_sacrifice_window(hero* newHero, int curPlayer)
    : CAdvPopup(0, 0, 800, 600, 0)
{
    m_currentHero = newHero;
    m_x = 100;
    m_y = 2;
    m_width = 600;
    m_height = 593;
    m_type = 18;

    long widgetId = 100;
    widget* newWidget;
    m_widgets.reserve(150);
    createArtifactWidgets(widgetId, curPlayer);
    createCreatureWidgets(widgetId, curPlayer);

    m_widgets.push_back(new textWidget(
        24, 414, 104, 50,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_NEXT_LEVEL),
        "smalfont.fnt", font::HEADING, widgetId++, 1, 0, 8));

    m_experienceWidget = new textWidget(
        44, 468, 66, 16, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_experienceWidget);

    m_widgets.push_back(new textWidget(
        24, 492, 104, 42,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_TOTAL_EXPERIENCE),
        "smalfont.fnt", font::HEADING, widgetId++, 1, 0, 8));

    m_experienceTotalWidget = new textWidget(
        41, 536, 66, 16, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_experienceTotalWidget);

    m_sacrificeButton = new type_func_button(
        269, 520, 64, 32, widgetId++, "AltSacr.def",
        sacrifice, 0, 1);
    m_widgets.push_back(m_sacrificeButton);

    newWidget = new type_func_button(
        515, 520, 64, 30, widgetId++, "iOkay.def",
        exitClick, 0, 1);
    static_cast<type_func_button*>(newWidget)->setHotkey(28);
    newWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_EXIT_BUTTON].m_text, 0, 1);
    m_widgets.push_back(newWidget);

    m_rolloverText = new textWidget(
        8, 567, 584, 18, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_rolloverText);

    int townType = g_heroClasses[m_currentHero->m_heroClass].m_townType;
    m_canSacrificeArtifacts =
        !(townType > TOWN_TOWER && townType < TOWN_STRONGHOLD);
    m_canSacrificeCreatures = townType > TOWN_TOWER;
    m_totalExperience = 0;

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

// E:\gamedcs\sacrifice_window.cpp:360
VA(0x00560380, 0xD67)  // ctor caller + dc name/order/locals, dc 0x1246b8
void type_sacrifice_window::createArtifactWidgets(
    long& widgetId, int curPlayer)
{
    m_artifactWidgets.reserve(80);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 600, 593, widgetId++,
        g_game->m_gameVersion >= 2 ? "AltrArt2.pcx" : "AltarArt.pcx", 0x800);
    background->setPlayerPaletteColors(curPlayer);
    m_widgets.push_back(background);
    m_artifactWidgets.push_back(background);

    type_doll_slot_definition def;
    iconWidget* currentIconWidget;
    def.m_width = 44;
    def.m_height = 44;
    def.m_image = "artifact.def";
    long count = g_game->m_gameVersion >= 2 ? 19 : 18;
    long i;
    for (i = 0; i < count; ++i) {
        def.m_x = g_slotDefinitions[i][0];
        def.m_y = g_slotDefinitions[i][1];
        def.m_slot = i;

        currentIconWidget = new iconWidget(
            def.m_x, def.m_y, def.m_width, def.m_height, widgetId++, def.m_image,
            0, 0, 0, 0, 16);
        m_widgets.push_back(currentIconWidget);
        m_slotBackWidgets.push_back(currentIconWidget);

        currentIconWidget = new type_doll_slot_widget(def, widgetId++);
        m_widgets.push_back(currentIconWidget);
        m_slotWidgets.push_back(currentIconWidget);
        m_artifactWidgets.push_back(currentIconWidget);
    }

    def.m_x = 43;
    def.m_y = 352;
    for (i = 0; i < 5; ++i) {
        currentIconWidget = new type_backpack_slot_widget(def, i, widgetId++);
        def.m_x += 44;
        m_widgets.push_back(currentIconWidget);
        m_backpackWidgets.push_back(currentIconWidget);
        m_artifactWidgets.push_back(currentIconWidget);
    }

    m_leftBackpackButton = new type_func_button(
        20, 352, 22, 46, widgetId++, "hsbtns3.def",
        scrollBackpackLeft, 0, 1);
    m_widgets.push_back(m_leftBackpackButton);
    m_artifactWidgets.push_back(m_leftBackpackButton);

    m_rightBackpackButton = new type_func_button(
        264, 352, 22, 46, widgetId++, "hsbtns5.def",
        scrollBackpackRight, 0, 1);
    m_widgets.push_back(m_rightBackpackButton);
    m_artifactWidgets.push_back(m_rightBackpackButton);

    long itemCount = 0;
    // Original local: artifact_offering; DC line 446 invokes the generated
    // default constructor, whose base call supplies TArtifact(-1). Line 444
    // initializes item_count before that constructor.
    type_artifact_offering artifactOffering;
    textWidget* currentTextWidget;
    for (long j = 0; j < 5; ++j) {
        long itemX = g_rowStart[j][0];
        int itemY = g_rowStart[j][1];
        long textX = itemX - 2;
        long textY = itemY + 47;
        for (count = g_rowSize[j]; count > 0; --count) {
            currentTextWidget = new textWidget(
                textX, textY, 48, 16, "",
                "smalfont.fnt", font::PRIMARY, widgetId++, 1, 0, 8);
            m_widgets.push_back(currentTextWidget);
            m_artifactWidgets.push_back(currentTextWidget);
            m_artifactValueWidgets.push_back(currentTextWidget);

            currentIconWidget = new type_artifact_offering_widget(
                itemX, itemY, 44, 44, itemCount++,
                widgetId++, "artifact.def");
            m_widgets.push_back(currentIconWidget);
            m_artifactWidgets.push_back(currentIconWidget);
            m_artifactOfferingWidgets.push_back(currentIconWidget);
            m_artifactOfferings.push_back(artifactOffering);

            textX += 54;
            itemX += 54;
        }
    }

    m_currentArtifactValue = new textWidget(
        269, 492, 66, 16,
        DATA_COMPGEN(0x00682a08, artifactZeroText, "0"),
        "smalfont.fnt", font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_currentArtifactValue);
    m_artifactWidgets.push_back(m_currentArtifactValue);

    m_currentArtifactWidget = new iconWidget(
        279, 440, 44, 44, widgetId++, "artifact.def",
        0, 0, 0, 0, 16);
    m_currentArtifactWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_ARTIFACT].m_text, 0, 1);
    m_widgets.push_back(m_currentArtifactWidget);
    m_artifactWidgets.push_back(m_currentArtifactWidget);

    m_emptyBackpackButton = new type_func_button(
        146, 520, 64, 32, widgetId++, "AltEmBk.def",
        emptyBackpack, 0, 1);
    m_emptyBackpackButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_EMPTY_BACKPACK].m_text, 0, 1);
    m_widgets.push_back(m_emptyBackpackButton);
    m_artifactWidgets.push_back(m_emptyBackpackButton);

    m_allArtifactsButton = new type_func_button(
        392, 520, 64, 32, widgetId++, "AltFill.def",
        allArtifacts, 0, 1);
    m_allArtifactsButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_ARTIFACTS].m_text, 0, 1);
    m_widgets.push_back(m_allArtifactsButton);
    m_artifactWidgets.push_back(m_allArtifactsButton);

    m_creaturesButton = new type_func_button(
        515, 421, 64, 32, widgetId++, "AltSacC.def",
        sacrificeCreatures, 0, 1);
    m_creaturesButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_SACRIFICE_CREATURES_BUTTON].m_text,
        0, 1);
    m_widgets.push_back(m_creaturesButton);
    m_artifactWidgets.push_back(m_creaturesButton);

    currentTextWidget = new textWidget(
        317, 23, 256, 18,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_ARTIFACTS_TITLE),
        "smalfont.fnt", font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_artifactWidgets.push_back(currentTextWidget);

    currentTextWidget = new textWidget(
        159, 415, 283, 18,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_CREATURES_TITLE),
        "smalfont.fnt", font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_artifactWidgets.push_back(currentTextWidget);
}

VA(0x005610f0, 0xE73)  // dc 0x124e60
void type_sacrifice_window::createCreatureWidgets(
    long& widgetId, int curPlayer)
{
    std::string buffer;
    long count;
    m_creatureWidgets.reserve(60);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 600, 593, widgetId++, "AltarMon.pcx", 0x800);
    background->setPlayerPaletteColors(curPlayer);
    m_widgets.push_back(background);
    m_creatureWidgets.push_back(background);

    buffer = formatString(
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_HERO_NAME),
        m_currentHero->m_name);

    textWidget* currentTextWidget = new textWidget(
        28, 21, 256, 18, buffer.c_str(), "smalfont.fnt",
        font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_creatureWidgets.push_back(currentTextWidget);

    m_creatureNameWidget = new textWidget(
        29, 56, 256, 42, "", "medfont.fnt",
        font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_creatureNameWidget);
    m_creatureWidgets.push_back(m_creatureNameWidget);

    currentTextWidget = new textWidget(
        317, 21, 256, 18,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_SOURCE_CREATURES),
        "smalfont.fnt", font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_creatureWidgets.push_back(currentTextWidget);

    currentTextWidget = new textWidget(
        318, 56, 256, 42,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_OFFERED_CREATURES),
        "smalfont.fnt", font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_creatureWidgets.push_back(currentTextWidget);

    iconWidget* newIconWidgets[7];
    iconWidget* selectionFrames[7];
    textWidget* newTextWidgets[7];
    long itemNumber = 0;

    for (long defIndex = 0; defIndex < 2; ++defIndex) {
        count = createCreatureIcons(
            g_constCreatureSources[defIndex][0],
            g_constCreatureSources[defIndex][1],
            g_constSourceSizes[defIndex][0],
            g_constSourceSizes[defIndex][1], itemNumber, widgetId,
            newIconWidgets, selectionFrames, newTextWidgets, 1);
        for (long i = 0; i < count; ++i) {
            m_creatureOfferings[itemNumber + i].m_iconWidget =
                newIconWidgets[i];
            m_creatureOfferings[itemNumber + i].m_creatureCountText =
                newTextWidgets[i];
            m_creatureOfferings[itemNumber + i].m_sourceSelectionFrame =
                selectionFrames[i];
        }
        itemNumber += count;
    }

    itemNumber = 0;
    for (defIndex = 0; defIndex < 2; ++defIndex) {
        count = createCreatureIcons(
            g_constCreatureOfferings[defIndex][0],
            g_constCreatureOfferings[defIndex][1],
            g_constSourceSizes[defIndex][0],
            g_constSourceSizes[defIndex][1], itemNumber, widgetId,
            newIconWidgets, selectionFrames, newTextWidgets, 0);
        for (long i = 0; i < count; ++i) {
            type_creature_offering& offering =
                m_creatureOfferings[itemNumber + i];
            offering.m_selectionWidget = newIconWidgets[i];
            offering.m_experienceText = newTextWidgets[i];
            offering.m_offeringSelectionFrame = selectionFrames[i];
        }
        itemNumber += count;
    }

    m_currentCreature.m_creatureCountText = new textWidget(
        145, 493, 66, 16, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_currentCreature.m_creatureCountText->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_CREATURE_AMOUNT].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_creatureCountText);
    m_creatureWidgets.push_back(m_currentCreature.m_creatureCountText);

    m_currentCreature.m_iconWidget = new type_army_slot_widget(
        149, 421, 58, 64, -1, widgetId++, "Twcrport.def", 1);
    m_currentCreature.m_iconWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_SOURCE_CREATURE].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_iconWidget);
    m_creatureWidgets.push_back(m_currentCreature.m_iconWidget);
    m_currentCreature.m_sourceSelectionFrame = 0;

    m_currentCreature.m_experienceText = new textWidget(
        391, 493, 66, 16, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_currentCreature.m_experienceText->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_OFFERING_AMOUNT].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_experienceText);
    m_creatureWidgets.push_back(m_currentCreature.m_experienceText);

    m_currentCreature.m_selectionWidget = new type_army_slot_widget(
        395, 421, 58, 64, -2, widgetId++, "TwCrPort.def", 0);
    m_currentCreature.m_selectionWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_OFFERING_CREATURE].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_selectionWidget);
    m_creatureWidgets.push_back(m_currentCreature.m_selectionWidget);
    m_currentCreature.m_offeringSelectionFrame = 0;

    m_creatureSlider = new slider(
        230, 479, 138, 16, widgetId++, 1, creatureSliderChange,
        slider::BROWN, 0, 0);
    m_creatureSlider->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CREATURE_SLIDER].m_text, 0, 1);
    m_widgets.push_back(m_creatureSlider);
    m_creatureWidgets.push_back(m_creatureSlider);

    m_maxCreaturesButton = new type_func_button(
        146, 520, 64, 32, widgetId++, "IrcBtns.def",
        maxCreatures, 0, 1);
    m_maxCreaturesButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_MAX_CREATURES].m_text, 0, 1);
    m_widgets.push_back(m_maxCreaturesButton);
    m_creatureWidgets.push_back(m_maxCreaturesButton);

    m_allCreaturesButton = new type_func_button(
        392, 520, 64, 32, widgetId++, "AltArmy.def",
        allCreatures, 0, 1);
    m_allCreaturesButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_CREATURES].m_text, 0, 1);
    m_widgets.push_back(m_allCreaturesButton);
    m_creatureWidgets.push_back(m_allCreaturesButton);

    m_artifactsButton = new type_func_button(
        515, 421, 54, 32, widgetId++, "AltArt.def",
        sacrificeArtifacts, 0, 1);
    m_artifactsButton->setHelpText(
        g_sacrificeWindowHelp[
            SACRIFICE_HELP_SACRIFICE_ARTIFACTS_BUTTON].m_text,
        0, 1);
    m_widgets.push_back(m_artifactsButton);
    m_creatureWidgets.push_back(m_artifactsButton);
}

VA(0x00561f70, 0x427)  // dc 0x1255cc
long type_sacrifice_window::createCreatureIcons(
    long iconX, long iconY, long columns, long rows, long itemNumber,
    long& widgetId, iconWidget** iconWidgets,
    iconWidget** selectionWidgets, textWidget** textWidgets,
    unsigned char leftPane)
{
    long count = 0;
    long textX = iconX - 4;
    long textY = iconY + 68;

    for (long row = 0; row < rows; ++row) {
        for (long column = 0; column < columns; ++column) {
            textWidgets[count] = new textWidget(
                textX, textY, 66, 16, "",
                "smalfont.fnt", font::PRIMARY, widgetId++, 1, 0, 8);
            textWidgets[count]->setHelpText(
                g_sacrificeWindowHelp[SACRIFICE_HELP_CREATURE_SLOT].m_text,
                0, 1);
            m_widgets.push_back(textWidgets[count]);
            m_creatureWidgets.push_back(textWidgets[count]);

            iconWidgets[count] = new type_army_slot_widget(
                iconX, iconY, 58, 64, itemNumber + count, widgetId++,
                "twcrport.def", leftPane);
            m_widgets.push_back(iconWidgets[count]);
            m_creatureWidgets.push_back(iconWidgets[count]);

            selectionWidgets[count] = new type_army_slot_widget(
                iconX, iconY, 58, 64, itemNumber + count, widgetId++,
                "TwCrPort.def", leftPane);
            m_widgets.push_back(selectionWidgets[count]);
            m_creatureWidgets.push_back(selectionWidgets[count]);
            selectionWidgets[count]->setIconFrame(1);

            ++count;
            textX += 83;
            iconX += 83;
        }
        textX -= columns * 83;
        iconX -= columns * 83;
        textY += 98;
        iconY += 98;
    }
    return count;
}

// Vtable 0x641620 slot 0 is the canonical VC6 deleting wrapper for the
// destructor below.
VA_COMPGEN(0x00560350, 0x21, SCALAR_DELETING_DTOR,
           type_sacrifice_window)

VA(0x005623a0, 0x15b)  // dc 0x125824
type_sacrifice_window::~type_sacrifice_window()
{
    deleteWidgets();
}

std::string convertWithCommas(long value)
{
    long digits = 0;
    std::string result;
    result = formatString(
        DATA_COMPGEN(0x00660a1c, decimalFormat, "%d"), value);

    long position = result.length();
    while (position--) {
        if (++digits == COMMA_DIGIT_THRESHOLD) {
            result.insert(result.begin() + position + 1, ',');
            digits = 1;
        }
    }
    return result;
}

VA(0x00562500, 0x15a)  // dc 0x125998
void type_sacrifice_window::updateExperience()
{
    std::string text;
    text = convertWithCommas(
        hero::getExperience(m_currentHero->m_level + 1)
        - m_currentHero->m_experience);
    m_experienceWidget->setText(text.c_str());

    text = convertWithCommas(m_totalExperience);
    m_experienceTotalWidget->setText(text.c_str());
    m_sacrificeButton->enable(m_totalExperience > 0);
}

VA(0x00562660, 0x1d5)  // dc 0x1258ac
std::string convertWithCommas(long value);

void updateArtifactWidget(iconWidget* slotWidget, type_artifact artifact)
{
    if (artifact.m_artifactId == ARTIFACT_NONE) {
        slotWidget->setVisible(0);
        slotWidget->setHelpText(0, 0, 1);
    } else {
        slotWidget->setIconFrame(artifact.m_artifactId);
        slotWidget->setVisible(1);
        slotWidget->setHelpText(
            g_artifactTraits[artifact.m_artifactId].m_name, 0, 1);
    }
}

VA(0x00562840, 0x166)  // dc 0x125b3c
void type_sacrifice_window::updateSlot(long slot)
{
    TArtifactSlot artifactSlot;
    memcpy(&artifactSlot, &slot, sizeof artifactSlot);
    type_artifact artifact = m_currentHero->getArtifact(artifactSlot);

    if (m_holdingArtifact.m_artifactId != ARTIFACT_NONE
        && m_currentHero->heroFn004E2840(
               m_holdingArtifact.m_artifactId, slot)) {
        updateArtifactWidget(m_slotBackWidgets[slot], artifact);

        m_slotWidgets[slot]->setVisible(1);
        m_slotWidgets[slot]->setIconFrame(SACRIFICE_ARTIFACT_SLOT_DROP_FRAME);
        m_slotWidgets[slot]->setHelpText(
            m_slotBackWidgets[slot]->getHelpText(), 0, 1);
    } else {
        m_slotBackWidgets[slot]->setVisible(0);
        updateArtifactWidget(m_slotWidgets[slot], artifact);
    }

    if (artifact.m_artifactId == ARTIFACT_NONE) {
        m_slotWidgets[slot]->setHelpText(
            g_artifactSlotTraits[slot].m_name, 0, 1);
    }
}

VA(0x005629e0, 0x33)  // dc 0x125c34
void type_sacrifice_window::updateAllSlots()
{
    long slotCount = g_game->m_gameVersion >= 2 ? 19 : 18;
    for (long slot = 0; slot < slotCount; ++slot)
        updateSlot(slot);
}

VA(0x00562a20, 0x24e)  // dc 0x125c60
void type_sacrifice_window::setArtifactMode()
{
    unsigned long i;
    for (i = 0; i < m_creatureWidgets.size(); ++i)
        m_creatureWidgets[i]->hide();
    for (i = 0; i < m_artifactWidgets.size(); ++i)
        m_artifactWidgets[i]->show();

    m_holdingArtifact.m_artifactId = ARTIFACT_NONE;
    updateAllSlots();

    for (i = 0; i < m_artifactOfferingWidgets.size(); ++i)
        updateArtifactOffering(i);

    for (i = 0; i < m_backpackWidgets.size(); ++i)
        updateArtifactWidget(m_backpackWidgets[i], m_currentHero->getBackpack(i));

    unsigned char scrollBackpack =
        m_currentHero->getLastBackpackIndex() + 1 > m_backpackWidgets.size();
    m_leftBackpackButton->enable(scrollBackpack);
    m_rightBackpackButton->enable(scrollBackpack);

    updateOffering(m_currentArtifactWidget, m_currentArtifactValue,
                    &m_holdingArtifact);
    m_sacrificingArtifacts = 1;
    m_emptyBackpackButton->enable(
        m_currentHero->getNumberInBackpack(1) > 0);
    m_sacrificeButton->enable(0);
    m_sacrificeButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_SACRIFICE_ARTIFACTS].m_text,
        0, 1);
    m_allArtifactsButton->enable(
        m_currentHero->getNumberInBackpack(1) > 0
        || m_currentHero->getEquippedArtifacts(1) > 0);
    m_creaturesButton->enable(m_canSacrificeCreatures);
    updateExperience();
}

VA(0x00562c70, 0x124)  // dc 0x125aac
void updateOffering(iconWidget* artifactWidget, textWidget* valueWidget,
                     const type_artifact_offering* offering)
{
    type_artifact artifact = *offering;
    if (artifact.m_artifactId == -1) {
        artifactWidget->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                      widget::WIDGET_DRAWN);
        artifactWidget->setHelpText(0, 0, 1);
    } else {
        artifactWidget->setIconFrame(artifact.m_artifactId);
        artifactWidget->sendMessage(widget::WIDGET_SET_STATUS,
                                      widget::WIDGET_DRAWN);
        artifactWidget->setHelpText(
            g_artifactTraits[artifact.m_artifactId].m_name, 0, 1);
    }

    if (offering->m_artifactId == -1) {
        artifactWidget->setHelpText(
            g_sacrificeWindowHelp[SACRIFICE_HELP_EMPTY_ARTIFACT_OFFERING].m_text,
            0, 1);
        valueWidget->setVisible(0);
        valueWidget->setHelpText(0, 0, 1);
    } else {
        valueWidget->setText(convertWithCommas(offering->m_value).c_str());
        valueWidget->setVisible(1);
        valueWidget->setHelpText(
            g_sacrificeWindowHelp[SACRIFICE_HELP_ARTIFACT_OFFERING_VALUE].m_text,
            0, 1);
    }
}

// E:\gamedcs\sacrifice_window.cpp:914
// Inlined at both retail callers. The AI-value load at +0x40 and the signed
// divide-by-forty reciprocal fix this integer value exactly.
long sacrificeValue(TCreatureType creature)
{
    return g_creatureTypeTraits[creature].m_aiValue / 40 * 5;
}

// E:\gamedcs\sacrifice_window.cpp:924
// Retail proves the record layout through its six widget loads and terminal
// group/amount pair. The two general-text indices are the folded +0x1ec and
// +0x788 rows of gpGeneralText's pointer table.
// Residual (99.9744%): all 42 blocks, 20 symbolic branch targets, three
// returns and instruction counts agree. VC6
// colors four early string-return temporaries at ebp-0x30 instead of
// retail's ebp-0x40; the later help_text/result slots themselves agree.
// Dreamcast's nested line-955/958 arm scopes are restored and ratcheted even
// though their braces are byte-flat. Hoisting help_text to the procedure scope
// suggested by its raw S_REGREL32 placement is retail-refuted: it constructs
// at entry, adds cleanup paths and falls to 85.65495%. Declaration order, a
// release-VERIFY probe, and splitting total_hits declaration/assignment are
// byte-flat. A fresh why-reg pass measures 18 masked slots, finds no allocator
// model divergence, and rejects all four guided source controls (named group
// is flat; volatile available and both adjacent store swaps are worse), so
// the remaining difference is a measured C1 stack-coloring tie.
// Residual (99.9744%): with memory operands masked the two objects are
// IDENTICAL - every block, branch, call and opcode agrees and the frames are
// equal.  The only divergence is one slot: retail addresses the `result`
// string at [ebp-0x40] where this compile puts it at [ebp-0x30], sixteen
// bytes (one basic_string) apart, with `help_text` taking the other slot.
// Measured and rejected 2026-09-06: declaring `std::string result` ABOVE
// `long total_hits` in the same block, 93.8202 - it costs the `xor ebx,ebx`
// at fn+0x20 and cascades.  The two block-scoped strings do not overlay on
// either side, so the cycle is an allocation order, not a lifetime fact.
VA(0x00562da0, 0x3a2)  // dc order/name/signature + retail field graph, dc 0x125e08
void type_sacrifice_window::updateCreatureOffering(
    type_creature_offering* creature)
{
    TCreatureType creatureType;
    long available;
    if (creature->m_group < 0) {
        creatureType = CREATURE_NONE;
        creature->m_amount = 0;
    } else {
        creatureType = m_currentHero->m_army.m_armyTypes[creature->m_group];
        available = m_currentHero->m_army.m_numTroops[creature->m_group];
    }

    if (creatureType == CREATURE_NONE) {
        creature->m_iconWidget->setVisible(0);
        creature->m_creatureCountText->setVisible(0);
        creature->m_selectionWidget->setVisible(0);
        creature->m_experienceText->setVisible(0);
    } else {
        long totalHits = static_cast<long>(
            (sacrificeValue(creatureType) * creature->m_amount)
            * m_currentHero->getExperienceBonusFactor());
        std::string result;

        creature->m_iconWidget->setIconFrame(creatureType + 2);
        creature->m_iconWidget->setVisible(1);
        if (!creature->m_sourceSelectionFrame) {
            result = convertWithCommas(creature->m_amount);
        } else {
            result = convertWithCommas(available);
        }
        creature->m_creatureCountText->setText(result.c_str());
        creature->m_creatureCountText->setVisible(1);

        creature->m_selectionWidget->setIconFrame(creatureType + 2);
        creature->m_selectionWidget->setVisible(creature->m_amount > 0);
        result = convertWithCommas(totalHits);
        result = formatString(
            g_generalText->getText(SACRIFICE_GENERAL_TEXT_EXPERIENCE),
            result.c_str());
        creature->m_experienceText->setText(result.c_str());
        creature->m_experienceText->setVisible(creature->m_amount > 0);
    }

    if (creature->m_sourceSelectionFrame) {
        if (creatureType == CREATURE_NONE) {
            creature->m_sourceSelectionFrame->setHelpText(0, 0, 1);
            creature->m_offeringSelectionFrame->setHelpText(0, 0, 1);
        } else {
            std::string helpText;
            helpText = formatString(
                g_generalText->getText(SACRIFICE_GENERAL_TEXT_CREATURE),
                getArmyName(creatureType, 0));
            creature->m_sourceSelectionFrame->setHelpText(helpText.c_str(), 0, 1);
            creature->m_offeringSelectionFrame->setHelpText(helpText.c_str(), 0, 1);
        }
    }
}

VA(0x00563150, 0x141)  // dc 0x126064
void type_sacrifice_window::setCreatureMode()
{
    unsigned long i;
    for (i = 0; i < m_artifactWidgets.size(); ++i)
        m_artifactWidgets[i]->hide();
    for (i = 0; i < m_creatureWidgets.size(); ++i)
        m_creatureWidgets[i]->show();

    for (long group = 0; group < 7; ++group) {
        m_creatureOfferings[group].m_amount = 0;
        m_creatureOfferings[group].m_group = group;
        updateCreatureOffering(&m_creatureOfferings[group]);
        m_creatureOfferings[group].m_sourceSelectionFrame->setVisible(0);
        m_creatureOfferings[group].m_offeringSelectionFrame->setVisible(0);
    }

    m_currentCreature.m_group = -1;
    updateCreatureOffering(&m_currentCreature);
    m_sacrificingArtifacts = 0;
    m_maxCreaturesButton->enable(0);
    m_creatureSlider->enable(0);
    m_sacrificeButton->enable(0);
    m_sacrificeButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_SACRIFICE_CREATURES].m_text,
        0, 1);
    m_creatureNameWidget->setVisible(0);
    m_allCreaturesButton->enable(
        m_currentHero->m_army.getCreatureTotal() > 1);
    m_artifactsButton->enable(m_canSacrificeArtifacts);
    updateExperience();
}

// E:\gamedcs\sacrifice_window.cpp:1036
// Retail expands this helper at the click sites. The DC line map fixes the
// source order and argument ABI; the Complete mouse-pointer and refresh call
// graph independently proves the body.
void type_sacrifice_window::pickUpArtifact(
    type_artifact artifact, long slot, unsigned char newArtifact)
{
    m_holdingArtifact.set(&artifact, slot, m_currentHero);
    if (newArtifact) {
        m_totalExperience += m_holdingArtifact.m_value;
        updateExperience();
    }
    updateOffering(m_currentArtifactWidget, m_currentArtifactValue,
                    &m_holdingArtifact);
    g_mouseManager->setPointer(m_holdingArtifact.m_artifactId,
                               mouseManager::ARTIFACT_SET);
    updateAllSlots();
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// E:\gamedcs\sacrifice_window.cpp:1053
// The Dreamcast line table and xref graph prove this helper boundary at each
// artifact-drop site. Complete folds the false change-experience arm into
// sacrifice, but retains the helper's redraw as a distinct inline tail.
void type_sacrifice_window::putDownArtifact(
    unsigned char changeExperience)
{
    if (changeExperience) {
        m_totalExperience -= m_holdingArtifact.m_value;
        updateExperience();
    }
    m_holdingArtifact.m_artifactId = ARTIFACT_NONE;
    updateOffering(m_currentArtifactWidget, m_currentArtifactValue,
                    &m_holdingArtifact);
    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    updateAllSlots();
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x005632a0, 0x417)  // dc 0x1262e4
void type_sacrifice_window::artifactClick(
    long slot, unsigned char rightClick)
{
    type_artifact oldArtifact = m_currentHero->getArtifact(TArtifactSlot(slot));

    if (m_holdingArtifact.m_artifactId == ARTIFACT_NONE) {
        if (oldArtifact.m_artifactId == ARTIFACT_NONE)
            return;

        if (rightClick) {
            m_currentHero->viewArtifact(&oldArtifact, rightClick);
            return;
        }

        if (oldArtifact.m_artifactId == ARTIFACT_SPELLBOOK) {
            TSpellbookWindow spellbook(
                *m_currentHero, 0, TSpellbookWindow::eContextNeither,
                m_currentHero->getSpecialTerrain());
            spellbook.doModal(0);
            return;
        }

        if (oldArtifact.m_artifactId == ARTIFACT_CATAPULT) {
            normalDialog(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_CANNOT_SACRIFICE_ARTIFACT),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }

        m_currentHero->removeArtifact(slot);
        updateSlot(slot);
        pickUpArtifact(oldArtifact, slot, 1);
        return;
    }

    if (rightClick)
        return;
    if (!m_currentHero->heroFn004E2840(
            m_holdingArtifact.m_artifactId, slot))
        return;

    if (oldArtifact.m_artifactId != ARTIFACT_NONE)
        m_currentHero->removeArtifact(slot);
    m_currentHero->equipArtifact(&m_holdingArtifact, slot);
    updateSlot(slot);
    putDownArtifact(1);
    if (oldArtifact.m_artifactId != ARTIFACT_NONE)
        pickUpArtifact(oldArtifact, slot, 1);
}

// E:\gamedcs\sacrifice_window.cpp:1127
// Complete expands this helper into both halves of backpack_click. Its widget
// vector walk and shared scrolling-state byte are directly visible there.
void type_sacrifice_window::updateBackpack()
{
    for (unsigned long i = 0; i < m_backpackWidgets.size(); ++i)
        updateArtifactWidget(m_backpackWidgets[i],
                               m_currentHero->getBackpack(i));

    unsigned char scrollBackpack =
        m_currentHero->getLastBackpackIndex() + 1
        > m_backpackWidgets.size();
    m_leftBackpackButton->enable(scrollBackpack);
    m_rightBackpackButton->enable(scrollBackpack);
}

// Picking up an artifact removes its backpack record. Putting one down inserts
// it or displays the backpack error.
// E:\gamedcs\sacrifice_window.cpp:1150
VA(0x005636c0, 0x31a)  // widget call edge + dc name/order, dc 0x1264dc
void type_sacrifice_window::backpackClick(
    long slot, unsigned char rightClick)
{
    type_artifact oldArtifact = m_currentHero->getBackpack(slot);

    if (m_holdingArtifact.m_artifactId == ARTIFACT_NONE) {
        if (oldArtifact.m_artifactId != ARTIFACT_NONE) {
            if (rightClick) {
                m_currentHero->viewArtifact(
                    &oldArtifact, rightClick);
            } else {
                m_currentHero->removeBackpackArtifact(slot);
                updateBackpack();
                pickUpArtifact(oldArtifact, 19, 1);
            }
        }
    } else if (!rightClick) {
        if (!m_currentHero->addToBackpack(&m_holdingArtifact, slot)) {
            normalDialog(
                m_currentHero
                    ->getBackpackError(
                        m_holdingArtifact.m_artifactId)
                    .c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            updateBackpack();
            putDownArtifact(1);
        }
    }
}

VA(0x005639e0, 0x5b)  // dc 0x125a4c
void updateArtifactWidget(iconWidget* slotWidget, type_artifact artifact);

VA(0x00563a40, 0x31)  // dc 0x1265b8
void type_sacrifice_window::updateArtifactOffering(long slot)
{
    updateOffering(m_artifactOfferingWidgets[slot],
                    m_artifactValueWidgets[slot],
                    &m_artifactOfferings[slot]);
}

VA(0x00563a80, 0x31b)  // dc 0x126640
void type_sacrifice_window::offeringClick(
    long slot, unsigned char rightClick)
{
    type_artifact_offering oldArtifact = m_artifactOfferings[slot];

    if (m_holdingArtifact.m_artifactId == ARTIFACT_NONE) {
        if (oldArtifact.m_artifactId != ARTIFACT_NONE) {
            if (rightClick) {
                m_currentHero->viewArtifact(
                    &oldArtifact, rightClick);
            } else {
                m_artifactOfferings[slot].m_artifactId = ARTIFACT_NONE;
                updateArtifactOffering(slot);
                pickUpArtifact(
                    oldArtifact, oldArtifact.m_source, 0);
            }
        }
    } else if (!rightClick) {
        m_artifactOfferings[slot] = m_holdingArtifact;
        updateArtifactOffering(slot);
        putDownArtifact(0);
        if (oldArtifact.m_artifactId != ARTIFACT_NONE)
            pickUpArtifact(oldArtifact, oldArtifact.m_source, 0);
    }
}

VA(0x00563da0, 0x152)
int type_sacrifice_window::scrollBackpackLeft(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SCROLL_BACKPACK_LEFT].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        window->m_currentHero->rotateBackpackLeft();
        window->updateBackpack();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00563f00, 0x152)
int type_sacrifice_window::scrollBackpackRight(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SCROLL_BACKPACK_RIGHT].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        window->m_currentHero->rotateBackpackRight();
        window->updateBackpack();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:1289
// Complete expands this helper into both artifact-batch callbacks. It fills
// the first empty offering, adds that record's scaled value, and refreshes
// the corresponding pair of offering widgets.
unsigned char type_sacrifice_window::addArtifact(
    type_artifact artifact, long source)
{
    unsigned long i;
    for (i = 0; i < m_artifactOfferings.size(); ++i) {
        if (m_artifactOfferings[i].m_artifactId == ARTIFACT_NONE)
            break;
    }
    if (i == m_artifactOfferings.size())
        return 0;

    m_artifactOfferings[i].set(&artifact, source, m_currentHero);
    m_totalExperience += m_artifactOfferings[i].m_value;
    updateArtifactOffering(i);
    return 1;
}

// E:\gamedcs\sacrifice_window.cpp:1310
// The helper is expanded at each callback. Retail scans the fixed 64-record
// backpack for its next occupied slot and stops if the offering pane fills.
void type_sacrifice_window::emptyBackpack()
{
    type_artifact artifact;
    while (m_currentHero->getNumberInBackpack(1) > 0) {
        long i;
        for (i = 0; i < SACRIFICE_BACKPACK_ARTIFACT_COUNT; ++i) {
            artifact = m_currentHero->getBackpack(i);
            if (artifact.m_artifactId != ARTIFACT_NONE)
                break;
        }
        if (!addArtifact(artifact, SACRIFICE_BACKPACK_SOURCE_SLOT))
            break;
        m_currentHero->removeBackpackArtifact(i);
    }
    updateBackpack();
}

VA(0x00564060, 0x2d3)
int type_sacrifice_window::emptyBackpack(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_EMPTY_BACKPACK].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        window->emptyBackpack();
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:1365
// The all-artifacts callback first offers each of the sixteen admissible doll
// slots, then expands empty_backpack to fill whatever offering slots remain.
// NOT A MEMBER-OFFSET BUG (checked 2026-09-06): the `[eax+0x2e4]` against
// retail's `[eax+0x350]` that a census flagged here is the SWITCH INDEX
// TABLE - `mov dl, byte ptr [eax + <fn>+0x350]` / `jmp [4*edx + <fn>+0x33c]`
// - whose base is the function's own end.  Retail's body is 0x6c bytes
// longer than ours by exactly the update_backpack expansion below, so the
// two tables sit 0x6c apart.  No hero/type_sacrifice_window member is
// involved.
// Residual (86.78%): the first 37 semantic blocks agree. This compile expands
// empty_backpack but keeps its nested update_backpack call, whereas retail
// expands both. An ordinary inline hint is byte-flat; force-inlining either
// helper improves this site to about 91.4% but regresses the exact standalone
// empty-backpack callback (and force-inlining update_backpack also regresses
// backpack_click), so the source-authentic call graph is retained.
VA(0x00564340, 0x35f)  // callback address-take + dc name/signature/order
int type_sacrifice_window::allArtifacts(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_ARTIFACTS].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        type_artifact artifact;
        for (long slot = 0; slot < SACRIFICE_EQUIPPED_SLOT_COUNT; ++slot) {
            artifact = window->m_currentHero->getArtifact(TArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE) {
                if (!window->addArtifact(artifact, slot))
                    break;
                window->m_currentHero->removeArtifact(slot);
                window->updateSlot(slot);
            }
        }
        window->emptyBackpack();
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x005646a0, 0x269)
int type_sacrifice_window::sacrifice(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SACRIFICE_ARTIFACTS].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        if (window->m_sacrificingArtifacts) {
            for (unsigned long i = 0;
                 i < window->m_artifactOfferings.size(); ++i) {
                window->m_artifactOfferings[i].m_artifactId = ARTIFACT_NONE;
                window->updateArtifactOffering(i);
            }
            if (window->m_holdingArtifact.m_artifactId != ARTIFACT_NONE)
                window->putDownArtifact(0);
        } else {
            armyGroup* army = &window->m_currentHero->m_army;
            long group;
            for (group = 0;
                 group < armyGroup::ARMY_GROUP_SLOT_COUNT; ++group) {
                army->m_numTroops[group] -=
                    window->m_creatureOfferings[group].m_amount;
                if (army->m_numTroops[group] <= 0)
                    army->dismiss(group);
                window->m_creatureOfferings[group].m_amount = 0;
                window->updateCreatureOffering(
                    &window->m_creatureOfferings[group]);
                window->m_creatureOfferings[group].m_sourceSelectionFrame->setVisible(0);
                window->m_creatureOfferings[group].m_offeringSelectionFrame->setVisible(0);
            }
            window->m_currentCreature.m_group = -1;
            window->updateCreatureOffering(&window->m_currentCreature);
            window->m_creatureNameWidget->setVisible(0);
            window->m_allCreaturesButton->enable(
                army->getCreatureTotal() > 1);
            window->m_creatureSlider->setResolution(1);
            window->m_creatureSlider->setState(0);
            window->m_creatureSlider->enable(0);
            window->m_maxCreaturesButton->enable(0);
        }

        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        window->m_currentHero->giveExperience(
            window->m_totalExperience, 1, 1);
        window->m_totalExperience = 0;
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564910, 0x164)
int type_sacrifice_window::sacrificeCreatures(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SACRIFICE_CREATURES_BUTTON].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        window->clear();
        window->setCreatureMode();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

// The Dreamcast line map places this one-purpose helper at source line 1499.
// Complete expands it into clear. The same order is independently repeated
// by ExitDialog: original equipped slot, any legal equipped slot, backpack,
// then a final arbitrary equipped-slot fallback.
void type_sacrifice_window::returnArtifact(
    const type_artifact_offering& artifact)
{
    if (artifact.m_source < 19) {
        if (m_currentHero->equipArtifact(&artifact, artifact.m_source))
            return;
        if (m_currentHero->equipArtifact(&artifact, -1))
            return;
    }
    if (m_currentHero->addToBackpack(&artifact, -1))
        return;
    m_currentHero->equipArtifact(&artifact, -1);
}

// The Dreamcast line map places clear at source line 1521. Complete expands
// it into sacrifice_artifacts; retail's vector walk, held-record cleanup and
// terminal total reset expose the entire body.
void type_sacrifice_window::clear()
{
    for (unsigned long i = 0; i < m_artifactOfferings.size(); ++i) {
        if (m_artifactOfferings[i].m_artifactId != ARTIFACT_NONE) {
            returnArtifact(m_artifactOfferings[i]);
            m_artifactOfferings[i].m_artifactId = ARTIFACT_NONE;
        }
    }

    if (m_holdingArtifact.m_artifactId != ARTIFACT_NONE) {
        returnArtifact(m_holdingArtifact);
        m_holdingArtifact.m_artifactId = ARTIFACT_NONE;
    }
    m_totalExperience = 0;
}

VA(0x00564a80, 0x171)
int type_sacrifice_window::exitClick(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_EXIT_BUTTON].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        window->clear();
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = 0;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00564c00, 0xe6)
void type_sacrifice_window::setCreatureSacrifice(long slot, long newAmount)
{
    if (m_creatureOfferings[slot].m_amount == newAmount)
        return;

    TCreatureType creatureType = m_currentHero->m_army.m_armyTypes[slot];
    long value = sacrificeValue(creatureType);
    long oldExperience = static_cast<long>(
        (value * m_creatureOfferings[slot].m_amount)
        * m_currentHero->getExperienceBonusFactor());
    m_totalExperience += static_cast<long>(
        (value * newAmount) * m_currentHero->getExperienceBonusFactor())
        - oldExperience;

    m_creatureOfferings[slot].m_amount = newAmount;
    updateCreatureOffering(&m_creatureOfferings[slot]);
    if (m_currentCreature.m_group == slot) {
        m_currentCreature.m_amount = newAmount;
        updateCreatureOffering(&m_currentCreature);
    }
}

// E:\gamedcs\sacrifice_window.cpp:1599
// Retail expands this helper at both callers. The scan preserves one troop
// only when every other army slot has already been offered to its limit.
long type_sacrifice_window::getMaxAmount(long slot) const
{
    long amount = m_currentHero->m_army.m_numTroops[slot];
    if (amount <= 0)
        return 0;

    long other;
    for (other = 0; other < armyGroup::ARMY_GROUP_SLOT_COUNT; ++other) {
        if (other != slot
            && m_creatureOfferings[other].m_amount
                < m_currentHero->m_army.m_numTroops[other])
            break;
    }
    if (other == armyGroup::ARMY_GROUP_SLOT_COUNT)
        --amount;
    return amount;
}

VA(0x00564cf0, 0xe9)
int type_sacrifice_window::allCreatures(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_CREATURES].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        long slot = armyGroup::ARMY_GROUP_SLOT_COUNT - 1;
        do {
            window->setCreatureSacrifice(
                slot, window->getMaxAmount(slot));
        } while (slot--);
        window->m_creatureSlider->setState(
            window->m_creatureSlider->getMaximum() - 1);
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564de0, 0x88)
int type_sacrifice_window::maxCreatures(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_MAX_CREATURES].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        int maximum = window->m_creatureSlider->getMaximum() - 1;
        window->m_creatureSlider->setState(maximum);
        creatureSliderChange(maximum, window);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564e70, 0x164)
int type_sacrifice_window::sacrificeArtifacts(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SACRIFICE_ARTIFACTS_BUTTON].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_sacrifice_window* window =
            static_cast<type_sacrifice_window*>(msg.m_window);
        window->clear();
        window->setArtifactMode();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564fe0, 0x394)  // dc 0x1270f0
void type_sacrifice_window::creatureClick(
    long slot, unsigned char rightClick, unsigned char leftPane)
{
    if (rightClick || slot == m_currentCreature.m_group || slot < 0) {
        if (slot < 0)
            slot = m_currentCreature.m_group;
        if (slot < 0) {
            if (rightClick) {
                normalDialog(
                    g_generalText->getText(
                        SACRIFICE_GENERAL_TEXT_EMPTY_CREATURE),
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            }
            return;
        }

        TCreatureType creatureType = m_currentHero->m_army.m_armyTypes[slot];
        long amount = m_creatureOfferings[slot].m_amount;
        if (leftPane)
            amount = m_currentHero->m_army.m_numTroops[slot] - amount;
        if (creatureType != CREATURE_NONE && amount > 0) {
            TViewArmyWindow viewArmyWindow(
                creatureType, 0x77, 0x20,
                static_cast<unsigned char>(!rightClick));
            viewArmyWindow.centerWindow(-1, -1);
            if (rightClick)
                viewArmyWindow.quickView();
            else
                viewArmyWindow.doModal();
        }
    } else {
        if (m_currentCreature.m_group >= 0) {
            m_creatureOfferings[m_currentCreature.m_group].m_offeringSelectionFrame->setVisible(0);
            m_creatureOfferings[m_currentCreature.m_group].m_sourceSelectionFrame->setVisible(0);
        }

        m_currentCreature.m_group = slot;
        m_currentCreature.m_amount = m_creatureOfferings[slot].m_amount;
        updateCreatureOffering(&m_currentCreature);
        updateCreatureOffering(&m_creatureOfferings[slot]);

        if (m_currentHero->m_army.m_armyTypes[slot] == CREATURE_NONE) {
            m_creatureNameWidget->setVisible(0);
        } else {
            std::string buffer;
            buffer = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_CREATURE_NAME),
                getArmyName(m_currentHero->m_army.m_armyTypes[slot], 0));
            m_creatureNameWidget->setText(buffer.c_str());
            m_creatureNameWidget->setVisible(1);
            m_creatureOfferings[slot].m_offeringSelectionFrame->setVisible(1);
            m_creatureOfferings[slot].m_sourceSelectionFrame->setVisible(1);
        }

        long maximum = getMaxAmount(slot);
        m_creatureSlider->setResolution(maximum + 1);
        m_creatureSlider->setState(m_currentCreature.m_amount);
        m_creatureSlider->enable(maximum > 0);
        m_maxCreaturesButton->enable(maximum > 0);
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    }
}

VA(0x00565380, 0x2e)
void type_sacrifice_window::creatureSliderChange(
    int state, heroWindow* parentWindow)
{
    type_sacrifice_window* window =
        static_cast<type_sacrifice_window*>(parentWindow);
    window->setCreatureSacrifice(window->m_currentCreature.m_group, state);
    window->updateExperience();
    window->drawWindow(
        1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x005653b0, 0x37)  // dc 0x127404
void type_sacrifice_window::doModal(bool fadeIn)
{
    if (m_canSacrificeArtifacts)
        setArtifactMode();
    else
        setCreatureMode();
    heroWindow::doModal(fadeIn);
}

VA(0x005653f0, 0x3b)  // dc 0x12743c
void type_sacrifice_window::handleWidgetHover(widget* currentWidget)
{
    if (!currentWidget->getHelpText())
        m_rolloverText->setText("");
    else
        m_rolloverText->setText(currentWidget->getHelpText());
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// Original: type_sacrifice_window::WindowHandler; DC source line 1846.
// Retail vtable 0x641620 slot 9 points to 0x5666f0, shared with the skeleton
// window's identical override. Preserve both source methods and one RVA claim.
int type_sacrifice_window::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;
    if (msg.m_id == MESSAGE_MOUSE_MOVE)
        return g_windowManager->convertToHover(msg);
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:1859
// Vtable 0x641620 slot 14 fixes the identity. Complete returns any held
// artifact to its original equipped slot where possible, then tries an
// arbitrary equipped slot, the backpack, and finally the arbitrary equipped
// path once more before closing the modal dialog.
// DC line 1866 calls return_artifact. The existing ordinary returnArtifact
// expands naturally here and preserves 100% while removing three failure joins.
VA(0x00565430, 0x80)  // anchor-vtable slot 14, dc 0x1274bc
int type_sacrifice_window::exitDialog(message& msg)
{
    type_artifact_offering* artifact = &m_holdingArtifact;
    msg.m_id = MESSAGE_WIDGET;
    g_windowManager->m_dialogReturn = 0;
    msg.m_codeY = widget::WIDGET_END_DIALOG;
    msg.m_codeX = widget::WIDGET_END_DIALOG;

    if (artifact->m_artifactId != -1) {
        returnArtifact(*artifact);
        artifact->m_artifactId = ARTIFACT_NONE;
    }
    return MESSAGE_DISPATCH_FORWARD;
}

VA_COMPGEN(0x005654b0, 0x5, IMPLICIT_DTOR, type_transformer_slot)  // dc 0x128764

// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x005654c0, 0x2a)  // linkorder + the +0x48/+0x4c pair, dc 0x127598
bool type_transformer_slot::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<type_skeleton_window*>(m_parentWindow)->creatureClick(
            m_group, m_slot, rightClick);
        return 1;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:1891
// Complete expands both calls in the transformer icon grid and retains no
// separately claimable constructor body.
type_transformer_slot::type_transformer_slot(
    long newX, long newY, long newW, long newH, long newGroup,
    long newSlot, long newId, const char* image)
    : iconWidget(newX, newY, newW, newH, newId, image,
                 0, 0, 0, 0, 16)
{
    m_slot = newSlot;
    m_group = newGroup;
}

// The transformer dialog's own pair, found by the same vtable-uniqueness
// scan that carried the tradpost family: 0x565f30 is a 33-byte scalar
// deleting destructor whose only image-wide reference is slot 0 of vtable
// 0x641694, and its callee 0x565f60 stores that same vtable. The vtable
// itself is referenced exactly twice - here and in the 0xa3c constructor at
// 0x5654f0 - so neither row is an /OPT:ICF fold.
VA_COMPGEN(0x00565f30, 0x21, SCALAR_DELETING_DTOR, type_skeleton_window)

// DC proves push_back; its text subscripts forward to getText. At 98.3508%,
// the final rollover append's growth path retains an extra vector::size.
// Removing the vector alias or binding its pointer argument locally does
// not recover that nested expansion; keep the canonical container call.
VA(0x005654f0, 0xA3C)  // dc 0x1275c0
type_skeleton_window::type_skeleton_window(armyGroup* newArmy)
    : CAdvPopup(100, 67, 600, 485, 18)
{
    long widgetId = 100;
    m_selectedCreatures.initialize();
    m_armies[0] = newArmy;
    m_armies[1] = &m_selectedCreatures;
    m_selectedGroup = -1;
    m_selectedIndex = -1;

    bitmapBorder* background = new bitmapBorder(
        0, 0, 600, 485, widgetId++, "SkTrnBk.pcx", 0x800);
    background->setPlayerPaletteColors(
        g_game->getLocalPlayerGamePos());
    m_widgets.push_back(background);

    m_widgets.push_back(new textWidget(
        25, 21, 257, 18,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_SOURCE_TITLE),
        "smalfont.fnt", font::HEADING, -1, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        320, 21, 257, 18,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_DESTINATION_TITLE),
        "smalfont.fnt", font::HEADING, -1, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        25, 55, 257, 42,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_SOURCE_DESCRIPTION),
        "medfont.fnt", font::HEADING, -1, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        320, 55, 257, 42,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_DESTINATION_DESCRIPTION),
        "medfont.fnt", font::HEADING, -1, 1, 0, 8));

    createCreatureIcons(
        45, 109, 3, 2, 0, 0, widgetId,
        &m_armyWidget[0][0], &m_selectBorder[0][0], &m_armyLabel[0][0]);
    createCreatureIcons(
        128, 305, 1, 1, 0, 6, widgetId,
        &m_armyWidget[0][6], &m_selectBorder[0][6], &m_armyLabel[0][6]);
    createCreatureIcons(
        334, 109, 3, 2, 1, 0, widgetId,
        &m_armyWidget[1][0], &m_selectBorder[1][0], &m_armyLabel[1][0]);
    createCreatureIcons(
        417, 305, 1, 1, 1, 6, widgetId,
        &m_armyWidget[1][6], &m_selectBorder[1][6], &m_armyLabel[1][6]);

    m_allCreaturesButton = new type_func_button(
        146, 416, 64, 32, widgetId++, "AltArmy.def",
        allCreatures, 0, 1);
    m_allCreaturesButton->setHelpText(
        g_transformerWindowHelp[TRANSFORMER_HELP_ALL_CREATURES].m_text,
        0, 1);
    m_widgets.push_back(m_allCreaturesButton);

    m_sacrificeButton = new type_func_button(
        269, 416, 64, 32, widgetId++, "AltSacr.def",
        sacrifice, 0, 1);
    m_sacrificeButton->setHelpText(
        g_transformerWindowHelp[TRANSFORMER_HELP_SACRIFICE].m_text,
        0, 1);
    m_sacrificeButton->enable(0);
    m_widgets.push_back(m_sacrificeButton);

    type_func_button* exitButton = new type_func_button(
        392, 416, 64, 32, widgetId++, "iCancel.def",
        exitClick, 0, 1);
    exitButton->setHelpText(
        g_transformerWindowHelp[TRANSFORMER_HELP_EXIT].m_text, 0, 1);
    m_widgets.push_back(exitButton);

    m_rolloverText = new textWidget(
        8, 459, 585, 19, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_rolloverText);

    for (long i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i)
        update(0, i);
    addWidgetsToMessageStream();
}

VA(0x00565f60, 0xC2)  // dc 0x127a08
type_skeleton_window::~type_skeleton_window()
{
    for (unsigned int i = 0; i < m_deathSamples.size(); i++) {
        g_soundManager->stopSample(m_deathSamples[i]->m_memSample.m_memSampleHandle);
        m_deathSamples[i]->dispose();
    }
    deleteWidgets();
}

// Original: type_skeleton_window::unselect; source line 2144, dc 0x127a8c.
// CreatureClick also hides a border but interleaves update/hover operations;
// neither its DC call sequence nor its retail body establishes a call here.
void type_skeleton_window::unselect()
{
    if (m_selectedGroup < 0)
        return;
    m_selectBorder[m_selectedGroup][m_selectedIndex]->setVisible(0);
    m_selectedGroup = -1;
    m_selectedIndex = -1;
}

// E:\gamedcs\sacrifice_window.cpp:2157
// All Complete callers inline this source helper. The DC call edges and the
// repeated retail expansion prove the transformed-army scan and the two
// terminal button states.
inline void type_skeleton_window::updateButtons()
{
    long i;
    for (i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        long type = m_armies[1]->m_armyTypes[i];
        if (type == CREATURE_NONE)
            continue;
        if (type != g_deathCreature[type])
            break;
    }
    m_sacrificeButton->enable(i < armyGroup::ARMY_GROUP_SLOT_COUNT);
    m_allCreaturesButton->enable(m_armies[0]->hasCreatures());
}

VA(0x00566030, 0x45D)  // dc 0x127b68
void type_skeleton_window::update(long group, long index)
{
    TCreatureType type = m_armies[group]->m_armyTypes[index];
    if (type == CREATURE_NONE) {
        m_armyWidget[group][index]->setVisible(0);
        m_armyLabel[group][index]->setVisible(0);
        m_selectBorder[group][index]->setHelpText(0, 0, 1);
        m_armyWidget[group][index]->setHelpText(0, 0, 1);
        m_armyLabel[group][index]->setHelpText(0, 0, 1);
        return;
    }

    std::string result;
    const char* name = getArmyName(
        type, m_armies[group]->m_numTroops[index]);

    m_armyWidget[group][index]->setIconFrame(type + 2);
    result = formatString(
        DATA_COMPGEN(0x00660a1c, decimalFormat, "%d"),
        m_armies[group]->m_numTroops[index]);
    m_armyLabel[group][index]->setText(result.c_str());
    m_armyWidget[group][index]->setVisible(1);
    m_armyLabel[group][index]->setVisible(1);

    if (group == 0) {
        result = formatString(
            g_generalText->getText(SACRIFICE_GENERAL_TEXT_CREATURE), name);
    } else {
        int transformed = g_deathCreature[type];
        if (transformed != type) {
            result = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_TRANSFORM_CREATURE),
                name, getArmyName(
                    transformed, m_armies[group]->m_numTroops[index]));
        } else if (m_armies[group]->m_numTroops[index] == 1) {
            result = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_ALREADY_TRANSFORMED_ONE),
                name);
        } else {
            result = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_ALREADY_TRANSFORMED_MANY),
                name);
        }
    }

    m_armyWidget[group][index]->setHelpText(result.c_str(), 0, 1);
    m_selectBorder[group][index]->setHelpText(result.c_str(), 0, 1);
    result = formatString(
        DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
        m_armies[group]->m_numTroops[index], name);
    m_armyLabel[group][index]->setHelpText(result.c_str(), 0, 1);
}

VA(0x00566490, 0x258)
void type_skeleton_window::creatureClick(
    long side, long slot, unsigned char rightClick)
{
    TCreatureType creatureType = m_armies[side]->m_armyTypes[slot];

    if (rightClick
        || (slot == m_selectedIndex && side == m_selectedGroup)) {
        if (creatureType != CREATURE_NONE) {
            TViewArmyWindow viewArmyWindow(
                creatureType, 0x77, 0x20,
                static_cast<unsigned char>(!rightClick));
            viewArmyWindow.centerWindow(-1, -1);
            if (rightClick)
                viewArmyWindow.quickView();
            else
                viewArmyWindow.doModal();
        }
    } else if (m_selectedGroup < 0) {
        m_selectedIndex = slot;
        m_selectedGroup = side;
        m_selectBorder[side][slot]->sendMessage(
            widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
        m_selectBorder[side][slot]->draw();
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    } else {
        if (creatureType
            == m_armies[m_selectedGroup]->m_armyTypes[m_selectedIndex]) {
            m_armies[side]->add(
                creatureType,
                m_armies[m_selectedGroup]->m_numTroops[m_selectedIndex], slot);
            m_armies[m_selectedGroup]->dismiss(m_selectedIndex);
        } else {
            long troops = m_armies[side]->m_numTroops[slot];
            m_armies[side]->m_armyTypes[slot] =
                m_armies[m_selectedGroup]->m_armyTypes[m_selectedIndex];
            m_armies[side]->m_numTroops[slot] =
                m_armies[m_selectedGroup]->m_numTroops[m_selectedIndex];
            m_armies[m_selectedGroup]->m_armyTypes[m_selectedIndex] = creatureType;
            m_armies[m_selectedGroup]->m_numTroops[m_selectedIndex] = troops;
        }
        m_selectBorder[m_selectedGroup][m_selectedIndex]->sendMessage(
            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
        update(side, slot);
        update(m_selectedGroup, m_selectedIndex);

        widget::clearHoverWidget();
        m_selectedGroup = -1;
        m_selectedIndex = -1;

        updateButtons();
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    }
}

// E:\gamedcs\sacrifice_window.cpp:2281
// Slot 9, sitting two rows past creature_click 0x566490 - the call target the
// transformer slot above pins - in the Dreamcast roster's order.
VA(0x005666f0, 0x2e)  // anchor-callee (CAdvPopup slot 9) + linkorder, dc 0x128048
int type_skeleton_window::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;
    if (msg.m_id == MESSAGE_MOUSE_MOVE)
        return g_windowManager->convertToHover(msg);
    return 0;
}

// Original: type_skeleton_window::ExitDialog; source line 2293, dc 0x128080.
// Retail vtable 0x641694 slot 14 points to 0x5f1180, the identical body claimed
// by type_university_window::exitDialog. Both classes own this override.
int type_skeleton_window::exitDialog(message& msg)
{
    msg.m_id = MESSAGE_WIDGET;
    g_windowManager->m_dialogReturn = 0;
    msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x00566720, 0x38)  // dc 0x128098
void type_skeleton_window::handleWidgetHover(widget* currentWidget)
{
    if (!currentWidget->getHelpText())
        m_rolloverText->setText("");
    else
        m_rolloverText->setText(currentWidget->getHelpText());
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x00566760, 0x28f)  // dc 0x1280e0
void type_skeleton_window::createCreatureIcons(
    long iconX, long iconY, long columns, long rows,
    long groupNumber, long itemNumber, long& widgetId,
    iconWidget** iconWidgets, iconWidget** selectionWidgets,
    textWidget** textWidgets)
{
    long row;
    long count = 0;
    long column;
    long textX = iconX - 5;
    long textY = iconY + 68;

    for (row = 0; row < rows; ++row) {
        for (column = 0; column < columns; ++column) {
            textWidgets[count] = new textWidget(
                textX, textY, 66, 16, "",
                "smalfont.fnt", font::PRIMARY, widgetId++, 1, 0, 8);
            m_widgets.push_back(textWidgets[count]);

            iconWidgets[count] = new type_transformer_slot(
                iconX, iconY, 58, 64, groupNumber, itemNumber + count,
                widgetId++, "twcrport.def");
            m_widgets.push_back(iconWidgets[count]);

            selectionWidgets[count] = new type_transformer_slot(
                iconX, iconY, 58, 64, groupNumber, itemNumber + count,
                widgetId++, "TwCrPort.def");
            m_widgets.push_back(selectionWidgets[count]);
            selectionWidgets[count]->setIconFrame(1);
            selectionWidgets[count]->setVisible(0);

            ++count;
            textX += 83;
            iconX += 83;
        }
        textX -= columns * 83;
        iconX -= columns * 83;
        textY += 98;
        iconY += 98;
    }
}

// E:\gamedcs\sacrifice_window.cpp:2385
// DC names this helper and records both transformer callbacks as callers.
// Complete expands both calls: occupied source slots move into the same
// destination slot when free, otherwise armyGroup::Add chooses a slot.
inline void moveAllArmies(armyGroup* source, armyGroup* dest)
{
    for (long i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        if (source->m_armyTypes[i] == CREATURE_NONE)
            continue;
        long destIndex = i;
        if (dest->m_armyTypes[i] != CREATURE_NONE)
            destIndex = -1;
        dest->add(source->m_armyTypes[i], source->m_numTroops[i], destIndex);
        source->dismiss(i);
    }
}

VA(0x005669f0, 0x137)  // dc 0x128310
int type_skeleton_window::allCreatures(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT
        && (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        normalDialog(
            g_transformerWindowHelp[TRANSFORMER_HELP_ALL_CREATURES].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_skeleton_window* window =
            static_cast<type_skeleton_window*>(msg.m_window);
        moveAllArmies(window->m_armies[0], window->m_armies[1]);
        for (long i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            window->update(0, i);
            window->update(1, i);
        }
        window->updateButtons();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00566b30, 0xE4)  // dc 0x1283ac
int type_skeleton_window::exitClick(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT
        && (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        normalDialog(
            g_transformerWindowHelp[TRANSFORMER_HELP_EXIT].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_skeleton_window* window =
            static_cast<type_skeleton_window*>(msg.m_window);
        moveAllArmies(window->m_armies[1], window->m_armies[0]);
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = 0;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00566c20, 0x15F)  // dc 0x128468
int type_skeleton_window::sacrifice(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT
        && (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        normalDialog(
            g_transformerWindowHelp[TRANSFORMER_HELP_SACRIFICE].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_skeleton_window* window =
            static_cast<type_skeleton_window*>(msg.m_window);
        for (long i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            TCreatureType type = window->m_armies[1]->m_armyTypes[i];
            if (type == CREATURE_NONE || type == g_deathCreature[type])
                continue;

            sprintf(g_text,
                    DATA_COMPGEN(0x006609e0, transformerKillSampleFormat,
                                 "%skill.82M"),
                    g_creatureTypeTraits[type].m_samplePrefix);
            sample* newSample = ResourceManager::getSample(g_text);
            window->m_deathSamples.push_back(newSample);
            g_soundManager->memorySample(newSample);
            window->m_armies[1]->m_armyTypes[i] = g_deathCreature[type];
            window->update(1, i);
        }
        window->updateButtons();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}
