#ifndef HOMM3_SACRIFICE_WINDOW_H
#define HOMM3_SACRIFICE_WINDOW_H

#include <vector>
#include "advmgr_popup.h"
#include "hero.h"
#include "iconwdgt.h"
#include "textwdgt.h"

class armyGroup;
class sample;
class slider;
class type_func_button;

// DC field list: a 16-byte type_artifact-derived record, with the wearable
// source slot at +8 and the displayed sacrifice value at +12.
struct type_artifact_offering : public type_artifact {
    long m_source;
    long m_value;

    // CodeView marks default construction generated (dc 0x128714).
    // Its base call passes -1 to type_artifact(TArtifact), so the inherited
    // default-argument path supplies construction without written wrappers.

    void set(const type_artifact* artifact, long slot, const hero* owner);
};
SIZE(type_artifact_offering, 16);

enum ECommaFormatting {
    COMMA_DIGIT_THRESHOLD = 4
};

enum ESacrificeWindowHelp {
    SACRIFICE_HELP_EXIT_BUTTON = 0,
    SACRIFICE_HELP_CURRENT_ARTIFACT = 1,
    SACRIFICE_HELP_EMPTY_BACKPACK = 2,
    SACRIFICE_HELP_ALL_ARTIFACTS = 3,
    SACRIFICE_HELP_SACRIFICE_CREATURES_BUTTON = 4,
    SACRIFICE_HELP_CURRENT_CREATURE_AMOUNT = 5,
    SACRIFICE_HELP_CURRENT_SOURCE_CREATURE = 6,
    SACRIFICE_HELP_CURRENT_OFFERING_AMOUNT = 7,
    SACRIFICE_HELP_CURRENT_OFFERING_CREATURE = 8,
    SACRIFICE_HELP_CREATURE_SLIDER = 9,
    SACRIFICE_HELP_MAX_CREATURES = 10,
    SACRIFICE_HELP_ALL_CREATURES = 11,
    SACRIFICE_HELP_SACRIFICE_ARTIFACTS_BUTTON = 12,
    SACRIFICE_HELP_CREATURE_SLOT = 13,
    SACRIFICE_HELP_EMPTY_ARTIFACT_OFFERING = 14,
    SACRIFICE_HELP_ARTIFACT_OFFERING_VALUE = 15,
    SACRIFICE_HELP_SACRIFICE_ARTIFACTS = 16,
    SACRIFICE_HELP_SACRIFICE_CREATURES = 17,
    SACRIFICE_HELP_SCROLL_BACKPACK_LEFT = 18,
    SACRIFICE_HELP_SCROLL_BACKPACK_RIGHT = 19,
    SACRIFICE_HELP_COUNT = 20
};

enum ESacrificeArtifactSlotFrame {
    SACRIFICE_ARTIFACT_SLOT_DROP_FRAME = 0x90,
    SACRIFICE_EQUIPPED_SLOT_COUNT = 16,
    SACRIFICE_BACKPACK_SOURCE_SLOT = 19,
    SACRIFICE_BACKPACK_ARTIFACT_COUNT = 64
};

// ARTRAITS.TXT's parser maps T/N/J/R to these bit values, and the retained
// offering helper maps the same four values to its experience ladder.
enum ESacrificeArtifactClass {
    SACRIFICE_ARTIFACT_CLASS_TREASURE = 2,
    SACRIFICE_ARTIFACT_CLASS_MINOR = 4,
    SACRIFICE_ARTIFACT_CLASS_MAJOR = 8,
    SACRIFICE_ARTIFACT_CLASS_RELIC = 16
};

enum ESacrificeGeneralText {
    SACRIFICE_GENERAL_TEXT_EXPERIENCE = 123,
    SACRIFICE_GENERAL_TEXT_HERO_NAME = 273,
    SACRIFICE_GENERAL_TEXT_NEXT_LEVEL = 476,
    SACRIFICE_GENERAL_TEXT_TOTAL_EXPERIENCE = 477,
    SACRIFICE_GENERAL_TEXT_ARTIFACTS_TITLE = 478,
    SACRIFICE_GENERAL_TEXT_CREATURES_TITLE = 479,
    SACRIFICE_GENERAL_TEXT_SOURCE_CREATURES = 480,
    SACRIFICE_GENERAL_TEXT_OFFERED_CREATURES = 481,
    SACRIFICE_GENERAL_TEXT_CREATURE = 482,
    SACRIFICE_GENERAL_TEXT_CANNOT_SACRIFICE_ARTIFACT = 483,
    SACRIFICE_GENERAL_TEXT_EMPTY_CREATURE = 484,
    SACRIFICE_GENERAL_TEXT_CREATURE_NAME = 485,
    SACRIFICE_GENERAL_TEXT_TRANSFORMER_SOURCE_TITLE = 486,
    SACRIFICE_GENERAL_TEXT_TRANSFORMER_DESTINATION_TITLE = 487,
    SACRIFICE_GENERAL_TEXT_TRANSFORMER_SOURCE_DESCRIPTION = 488,
    SACRIFICE_GENERAL_TEXT_TRANSFORMER_DESTINATION_DESCRIPTION = 489,
    SACRIFICE_GENERAL_TEXT_TRANSFORM_CREATURE = 490,
    SACRIFICE_GENERAL_TEXT_ALREADY_TRANSFORMED_ONE = 491,
    SACRIFICE_GENERAL_TEXT_ALREADY_TRANSFORMED_MANY = 492
};

// HELP.TXT's second pass at 0x5b9b52 fills exactly twenty stride-8
// text/right-click pairs from 0x6a6638 through 0x6a66d7.
DATA(0x006a6638) extern THelpText g_sacrificeWindowHelp[SACRIFICE_HELP_COUNT];

// DC public ?gTransformerWindowHelp@@3PAUTHelpText@@A supplies the name;
// Complete references the three text/right-click pairs at 0x6a77d0..e7.
enum ETransformerWindowHelp {
    TRANSFORMER_HELP_ALL_CREATURES = 0,
    TRANSFORMER_HELP_SACRIFICE = 1,
    TRANSFORMER_HELP_EXIT = 2,
    TRANSFORMER_HELP_COUNT = 3
};
DATA(0x006a77d0) extern THelpText
    g_transformerWindowHelp[TRANSFORMER_HELP_COUNT];

struct type_icon_definition {
    long m_x;
    long m_y;
    long m_width;
    long m_height;
    const char* m_image;
};
SIZE(type_icon_definition, 0x14);

// DC field list 0x2712: a type_icon_definition base and slot at +0x14.
struct type_doll_slot_definition : public type_icon_definition {
    long m_slot;
};
SIZE(type_doll_slot_definition, 0x18);

// DC proves a seven-element 224-byte array, hence 32 bytes per record.
// Retail update_creature_offering independently proves the six widget
// pointers followed by the group/amount pair at +0x18/+0x1c.
struct type_creature_offering {
    iconWidget* m_iconWidget;
    // createCreatureWidgets 0x5610f0 assigns the source grid's selection
    // frames from createCreatureIcons' iconWidget** output. Role-derived name.
    iconWidget* m_sourceSelectionFrame;
    // updateCreatureOffering 0x562da0 displays the available stack count,
    // or the selected amount for the current-creature editor record.
    textWidget* m_creatureCountText;
    iconWidget* m_selectionWidget;
    // The same builder supplies the offered-creature grid's selection
    // frame; its producer also proves iconWidget*, not an untyped widget view.
    iconWidget* m_offeringSelectionFrame;
    // updateCreatureOffering formats sacrificeValue * amount * the hero's
    // experience bonus here, using the experience label text. Role-derived name.
    textWidget* m_experienceText;
    long m_group;
    long m_amount;
};
SIZE(type_creature_offering, 0x20);

// Retail's constructor at 0x55fdd0 proves the CAdvPopup base, current_hero
// at +0x60, and eight VC6 vectors whose last storage triplet ends at +0x23c.
// That is also exactly the Dreamcast 0x214-byte field roster widened by the
// proven 8-byte CAdvPopup delta and eight 12->16-byte vector deltas. Keep the
// still-unadmitted tail opaque, but keep its complete, canonical extent here:
// callback widgets need the real derived type, not a synthetic window view.
class type_sacrifice_window : public CAdvPopup {
public:
    type_sacrifice_window(hero* newHero, int curPlayer);

private:
    hero* m_currentHero;
    type_artifact_offering m_holdingArtifact;  // +0x64
    unsigned char m_sacrificingArtifacts;      // +0x74
    // +0x75: DoModal 0x5653b0 tests this byte to choose set_artifact_mode
    // over set_creature_mode. The Dreamcast field roster puts
    // can_sacrifice_artifacts at DC +0x6d, and the proven 8-byte CAdvPopup
    // delta lands it exactly here - the same widening that already places
    // current_hero and the rollover pointer below.
    unsigned char m_canSacrificeArtifacts;
    unsigned char m_canSacrificeCreatures;   // +0x76

public:
    // The preceding byte field and following four-byte field establish
    // this alignment gap; the reference layout retains the same boundary.
    unsigned char m_paddingBeforeTotalExperience;

private:
    long m_totalExperience;                   // +0x78
    textWidget* m_experienceWidget;           // +0x7c
    textWidget* m_experienceTotalWidget;     // +0x80
    textWidget* m_currentArtifactValue;       // +0x84
    textWidget* m_creatureNameWidget;         // +0x88

public:
    // +0x8c: handle_widget_hover 0x5653f0 reads it and dispatches slot 13
    // (textWidget::SetText) through it - the same rollover pointer
    // type_skeleton_window keeps at +0x60 and type_university_window at
    // +0x70. Offsets either side of it are unchanged.
    textWidget* m_rolloverText;
    // The two mode switches DoModal picks between. Located by an exhaustive
    // order-map of the Dreamcast roster over the segment between the
    // destructor and DoModal: set_artifact_mode 0x562a20, set_creature_mode
    // 0x563150. Bodies still deferred.
    virtual ~type_sacrifice_window();

    void artifactClick(long slot, unsigned char rightClick);
    void backpackClick(long slot, unsigned char rightClick);
    void creatureClick(long slot, unsigned char rightClick,
                        unsigned char leftPane);
    void offeringClick(long slot, unsigned char rightClick);

    virtual void handleWidgetHover(widget* currentWidget);  // slot 4
    virtual int doModal(unsigned char fadeIn);                 // slot 6
    virtual int exitDialog(message& msg);                      // slot 14

private:
    unsigned char addArtifact(type_artifact artifact, long source);
    void clear();
    void createArtifactWidgets(long& widgetId, int curPlayer);
    long createCreatureIcons(
        long iconX, long iconY, long columns, long rows,
        long itemNumber, long& widgetId, iconWidget** iconWidgets,
        iconWidget** selectionWidgets, textWidget** textWidgets,
        unsigned char leftPane);
    void createCreatureWidgets(long& widgetId, int curPlayer);
    void emptyBackpack();
    static int emptyBackpack(message& msg);
    long getMaxAmount(long slot) const;
    void pickUpArtifact(type_artifact artifact, long slot,
                          unsigned char newArtifact);
    void putDownArtifact(unsigned char changeExperience);
    void returnArtifact(const type_artifact_offering& artifact);
    void setArtifactMode();
    void setCreatureMode();
    void setCreatureSacrifice(long slot, long newAmount);
    void updateBackpack();
    void updateExperience();
    void updateAllSlots();
    void updateArtifactOffering(long slot);
    void updateCreatureOffering(type_creature_offering* creature);
    void updateSlot(long slot);
    static int allArtifacts(message& msg);
    static int allCreatures(message& msg);
    static void creatureSliderChange(int state, heroWindow* parentWindow);
    static int exitClick(message& msg);
    static int maxCreatures(message& msg);
    static int sacrifice(message& msg);
    static int sacrificeArtifacts(message& msg);
    static int sacrificeCreatures(message& msg);
    static int scrollBackpackLeft(message& msg);
    static int scrollBackpackRight(message& msg);
    // DC places the current-artifact/slider/button pointers at +0x88..+0xb0.
    // Retail's proven 8-byte base delta moves that run to +0x90..+0xb8;
    // update_experience independently proves sacrifice_button at +0xa4.
    iconWidget* m_currentArtifactWidget;      // +0x90
    slider* m_creatureSlider;                  // +0x94
    type_func_button* m_leftBackpackButton;   // +0x98
    type_func_button* m_rightBackpackButton;  // +0x9c
    type_func_button* m_emptyBackpackButton;  // +0xa0
    type_func_button* m_sacrificeButton;       // +0xa4
    type_func_button* m_allArtifactsButton;   // +0xa8
    type_func_button* m_creaturesButton;       // +0xac
    type_func_button* m_maxCreaturesButton;   // +0xb0
    type_func_button* m_allCreaturesButton;   // +0xb4
    type_func_button* m_artifactsButton;       // +0xb8
    std::vector<type_artifact_offering> m_artifactOfferings; // +0xbc
    std::vector<textWidget*> m_artifactValueWidgets;        // +0xcc
    std::vector<iconWidget*> m_artifactOfferingWidgets;     // +0xdc
    std::vector<iconWidget*> m_slotBackWidgets;              // +0xec
    std::vector<iconWidget*> m_slotWidgets;                   // +0xfc
    std::vector<iconWidget*> m_backpackWidgets;               // +0x10c
    type_creature_offering m_creatureOfferings[7];          // +0x11c
    type_creature_offering m_currentCreature;               // +0x1fc
    std::vector<widget*> m_artifactWidgets;                   // +0x21c
    std::vector<widget*> m_creatureWidgets;                   // +0x22c
};
SIZE(type_sacrifice_window, 0x23c);

// iconWidget ends at +0x48 in retail. Each vtable's added slot 13 reads the
// derived state there before forwarding to its parent sacrifice window.

// The equipped-artifact doll slots. Retail 0x55fce0 reads the single dword at
// +0x48 and forwards it with right_click, so the class adds exactly that.
class type_doll_slot_widget : public iconWidget {
public:
    long m_slot;

    type_doll_slot_widget(const type_doll_slot_definition& def, long id);

    virtual bool handleClick(bool downClick,
                                       bool rightClick);
};
SIZE(type_doll_slot_widget, 0x4c);

class type_backpack_slot_widget : public iconWidget {
public:
    long m_slot;

    type_backpack_slot_widget(const type_icon_definition& def,
                              long newSlot, long id);

    virtual bool handleClick(bool downClick,
                                       bool rightClick);
};
SIZE(type_backpack_slot_widget, 0x4c);

class type_artifact_offering_widget : public iconWidget {
public:
    long m_itemNumber;

    type_artifact_offering_widget(long x, long y, long width, long height,
                                  long newItemNumber, long id,
                                  const char* image);

    virtual bool handleClick(bool downClick,
                                       bool rightClick);
};
SIZE(type_artifact_offering_widget, 0x4c);

// The army slots on both panes. Retail 0x55fda0 reads a dword at +0x48 AND a
// byte at +0x4c and forwards both around right_click, which is exactly
// creature_click's (slot, right_click, left_pane) argument list and exactly
// the pair the Dreamcast constructor takes (new_slot, _left_pane).
class type_army_slot_widget : public iconWidget {
public:
    long m_slot;
    unsigned char m_leftPane;

    type_army_slot_widget(long newX, long newY, long newW, long newH,
                          long newSlot, long newId, const char* image,
                          unsigned char newLeftPane);

    virtual bool handleClick(bool downClick,
                                       bool rightClick);
};
SIZE(type_army_slot_widget, 0x50);

// The skeleton-transformer dialog, over the same 0x60-byte CAdvPopup base
// its Dreamcast virtual roster attests (WindowHandler / ExitDialog /
// handle_widget_hover, exactly type_sacrifice_window's set). Retail accesses
// the derived fields at the offsets recorded below.
class type_skeleton_window : public CAdvPopup {
public:
    textWidget* m_rolloverText;  // +0x60

    type_skeleton_window(armyGroup* newArmy);

    virtual ~type_skeleton_window();
    void creatureClick(long side, long slot, unsigned char rightClick);
    virtual void handleWidgetHover(widget* currentWidget);  // slot 4
    virtual int windowHandler(message& msg);                   // slot 9

private:
    void createCreatureIcons(
        long iconX, long iconY, long columns, long rows,
        long groupNumber, long itemNumber, long& widgetId,
        iconWidget** iconWidgets, iconWidget** selectionWidgets,
        textWidget** textWidgets);
    void update(long group, long index);
    void updateButtons();
    static int allCreatures(message& msg);
    static int exitClick(message& msg);
    static int sacrifice(message& msg);
    type_func_button* m_sacrificeButton;         // +0x64
    type_func_button* m_allCreaturesButton;     // +0x68
    long m_selectedGroup;                        // +0x6c
    long m_selectedIndex;                        // +0x70
    armyGroup m_selectedCreatures;               // +0x74
    armyGroup* m_armies[2];                        // +0xac
    iconWidget* m_armyWidget[2][7];               // +0xb4
    iconWidget* m_selectBorder[2][7];             // +0xec
    textWidget* m_armyLabel[2][7];                // +0x124
    // +0x15c: the destructor 0x565f60 walks this vector by size(), stops and
    // disposes every sample in it, then lets the member's own _Tidy run
    // (operator delete on _First at +0x160, then the 0x160/0x164/0x168
    // triplet zeroed). The Dreamcast roster's last member is
    // death_samples at DC +0x154, and the proven 8-byte CAdvPopup delta -
    // the only delta, since nothing before it is a vector - lands it here.
    std::vector<sample*> m_deathSamples;
};

// The transformer dialog's creature slots. Retail 0x5654c0 forwards TWO
// dwords, +0x48 then +0x4c, ahead of right_click - creature_click's
// (side, slot, right_click) - and the Dreamcast constructor takes exactly
// that pair as (new_group, new_slot).
class type_transformer_slot : public iconWidget {
public:
    long m_group;
    long m_slot;

    type_transformer_slot(long newX, long newY, long newW, long newH,
                          long newGroup, long newSlot, long newId,
                          const char* image);

    virtual bool handleClick(bool downClick,
                                       bool rightClick);
};
SIZE(type_transformer_slot, 0x50);

std::string convertWithCommas(long value);
void updateOffering(iconWidget* artifactWidget, textWidget* valueWidget,
                     const type_artifact_offering* offering);

#endif  /* HOMM3_SACRIFICE_WINDOW_H */
