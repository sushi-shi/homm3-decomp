#ifndef HOMM3_UNIVERSITY_WINDOW_H
#define HOMM3_UNIVERSITY_WINDOW_H

#include "advmgr_popup.h"
#include "herospec.h"
#include "iconwdgt.h"
#include "textwdgt.h"

#include <vector>
class hero;
struct type_university;
class type_university_window;

// Shared three-entry Basic/Advanced/Expert display-name row. Its retail
// storage is claimed by levelupwindow.cpp; the university purchase callback
// reads the Basic entry when composing its skill dialog.
extern const char* g_skillMasteryNames[3];

// Retail's two inlined constructor sites prove the iconWidget base followed
// by the byte click latch and dword skill at +0x48/+0x4c. Dreamcast preserves
// both the class identity and the source-level constructor/handle_click
// boundaries even though Complete inlines the constructor into this TU.
class type_university_skill_button : public iconWidget {
public:
    unsigned char m_click;       // +0x48
    TSecondarySkill m_skill;     // +0x4c

    type_university_skill_button(long x, long y, long width, long height,
                                 long newId, const char* image,
                                 TSecondarySkill newSkill);
    virtual bool handleClick(bool downClick,
                                       bool rightClick);
    void setSkill(TSecondarySkill newSkill, unsigned char newClick);
};
SIZE(type_university_skill_button, 0x50);

// The retail constructor and update path prove this complete 0x14-byte
// record: the button, two coloured bars and label occupy the first four
// dwords, followed by the TSecondarySkill. Dreamcast proves the record's
// identity and the containing window's four-plus-one arrangement.
struct type_university_skill {
    type_university_skill_button* m_button;
    iconWidget* m_topBar;
    iconWidget* m_bottomBar;
    textWidget* m_textWidget;
    TSecondarySkill m_skill;
};
SIZE(type_university_skill, 0x14);

// Retail's +0x70 rollover pointer and +0x74 skill-array accesses translate
// DC's +0x68/+0x6c fields by exactly the eight-byte CAdvPopup widening already
// proven in advmgr_popup.h. The constructor and callbacks below complete the
// translated tail through the two VC6 vectors at +0xd8 and +0xe8.
class type_university_window : public CAdvPopup {
public:
    // The hero's secondary-skill slot cap, as BOTH university bodies test
    // it (`cmp dword [hero+0x101], 8`). Scoped to this window because it is
    // the only consumer that compares for EQUALITY; events.obj's own
    // readers spell the same cap as `< 8` / `>= 8`.
    enum { TUITION = 2000, MAX_SECONDARY_SKILLS = 8 };

    // Dreamcast names this member sequence at +0x58..+0xdc. Retail shifts it
    // by the byte-proven eight-byte CAdvPopup widening; the constructor and
    // modal/callback bodies independently corroborate every used offset.

protected:
    hero* m_currentHero;                       // +0x60
    class type_func_button* m_purchaseButton; // +0x64
    textWidget* m_purchaseTitleWidget;        // +0x68 (unused by Complete)
    textWidget* m_purchaseTextWidget;         // +0x6c
    textWidget* m_rolloverWidget;              // +0x70
    type_university_skill m_skills[4];           // +0x74 .. +0xc3
    type_university_skill m_selectedSkill;      // +0xc4 .. +0xd7
    std::vector<widget*> m_selectionWidgets;    // +0xd8
    std::vector<widget*> m_purchaseWidgets;     // +0xe8

public:
    // Retail 0x5ef500. `bTownUniversity` is the retail-added third
    // parameter the Dreamcast pair does not have, and both image-wide
    // call sites name it: the map object's visit (0x4aa526) passes 0,
    // the town building's page (0x5d2f26) passes 1, and the constructor
    // gates one extra 0x48-byte widget on it.
    type_university_window(hero* newHero, const type_university* university,
                           unsigned char townUniversity);
    virtual void doModal(bool fade);  // slot 6

    // DC message-reference override; retail slot 9 folds at 0x5666f0.
    virtual int windowHandler(message& msg);
    void skillClick(TSecondarySkill skill);

protected:
    virtual void handleWidgetHover(widget* currentWidget);  // slot 4

public:
    virtual int exitDialog(message& msg);  // slot 14

protected:
    void setSelectionMode();
    void updateSkillButton(type_university_skill& skill);
    static int cancelClick(message& msg);
    static int exitClick(message& msg);
    static int purchaseClick(message& msg);
};
SIZE(type_university_window, 0xf8);

#endif  /* HOMM3_UNIVERSITY_WINDOW_H */
