// university_window.h - prototypes of university_window.cpp (compiland university_window.obj)
#ifndef HOMM3_UNIVERSITY_WINDOW_H
#define HOMM3_UNIVERSITY_WINDOW_H

#include "advmgr_popup.h"
#include "herospec.h"
#include "iconwdgt.h"
#include "textwdgt.h"

#include <vector>
class Hero;
struct University;
class UniversityWindow;

// Shared three-entry Basic/Advanced/Expert display-name row. Its retail
// storage is claimed by levelupwindow.cpp; the university purchase callback
// reads the Basic entry when composing its skill dialog.
extern const char* g_skillMasteryNames[3];

// Retail's two inlined constructor sites prove the iconWidget base followed
// by the byte click latch and dword skill at +0x48/+0x4c. Dreamcast preserves
// both the class identity and the source-level constructor/handle_click
// boundaries even though Complete inlines the constructor into this TU.
// Before normalization (type): type_university_skill_button.
class UniversitySkillButton : public IconWidget {
public:
    unsigned char m_click;       // +0x48
    SecondarySkill m_skill;     // +0x4c

    UniversitySkillButton(long x, long y, long width, long height,
                                 long newId, const char* image,
                                 SecondarySkill newSkill);
    virtual bool handleClick(bool downClick,
                                       bool rightClick);
    void setSkill(SecondarySkill newSkill, unsigned char newClick);
};
SIZE(UniversitySkillButton, 0x50);

// The retail constructor and update path prove this complete 0x14-byte
// record: the button, two coloured bars and label occupy the first four
// dwords, followed by the TSecondarySkill. Dreamcast proves the record's
// identity and the containing window's four-plus-one arrangement.
// Before normalization (type): type_university_skill.
struct UniversitySkill {
    UniversitySkillButton* m_button;
    IconWidget* m_topBar;
    IconWidget* m_bottomBar;
    TextWidget* m_textWidget;
    SecondarySkill m_skill;
};
SIZE(UniversitySkill, 0x14);

// Retail's +0x70 rollover pointer and +0x74 skill-array accesses translate
// DC's +0x68/+0x6c fields by exactly the eight-byte CAdvPopup widening already
// proven in advmgr_popup.h. The constructor and callbacks below complete the
// translated tail through the two VC6 vectors at +0xd8 and +0xe8.
// Before normalization (type): type_university_window.
class UniversityWindow : public CAdvPopup {
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
    Hero* m_currentHero;                       // +0x60
    class FuncButton* m_purchaseButton; // +0x64
    TextWidget* m_purchaseTitleWidget;        // +0x68 (unused by Complete)
    TextWidget* m_purchaseTextWidget;         // +0x6c
    TextWidget* m_rolloverWidget;              // +0x70
    UniversitySkill m_skills[4];           // +0x74 .. +0xc3
    UniversitySkill m_selectedSkill;      // +0xc4 .. +0xd7
    std::vector<Widget*> m_selectionWidgets;    // +0xd8
    std::vector<Widget*> m_purchaseWidgets;     // +0xe8

public:
    // Retail 0x5ef500. `bTownUniversity` is the retail-added third
    // parameter the Dreamcast pair does not have, and both image-wide
    // call sites name it: the map object's visit (0x4aa526) passes 0,
    // the town building's page (0x5d2f26) passes 1, and the constructor
    // gates one extra 0x48-byte widget on it.
    UniversityWindow(Hero* newHero, const University* university,
                           unsigned char townUniversity);
    virtual int doModal(unsigned char fade);  // slot 6

    // DC message-reference override; retail slot 9 folds at 0x5666f0.
    virtual int windowHandler(Message& msg);
    void skillClick(SecondarySkill skill);

protected:
    virtual void handleWidgetHover(Widget* currentWidget);  // slot 4

public:
    virtual int exitDialog(Message& msg);  // slot 14

protected:
    void setSelectionMode();
    void updateSkillButton(UniversitySkill& skill);
    static int cancelClick(Message& msg);
    static int exitClick(Message& msg);
    static int purchaseClick(Message& msg);
};
SIZE(UniversityWindow, 0xf8);

// --- type_university_skill_button ---
// CODEVIEW(E:\gamedcs\university_window.cpp:65, dc 0x18e6ac) void type_university_skill_button::type_university_skill_button(long _x, long _y, long _width, long _height, long new_id, const char* _image, TSecondarySkill new_skill);
// CODEVIEW(E:\gamedcs\university_window.cpp:50, dc 0x18fad8) void type_university_skill_button::set_skill(TSecondarySkill new_skill, unsigned char new_click);
// CODEVIEW(E:\gamedcs\university_window.cpp:68, dc 0x18fae4) void* type_university_skill_button::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\university_window.cpp:68, dc 0x18fb18) void type_university_skill_button::~type_university_skill_button();

// --- type_university_window ---
// CODEVIEW(E:\gamedcs\university_window.cpp:102, dc 0x18e790) void type_university_window::type_university_window(hero* new_hero, const type_university* university);
// CODEVIEW(E:\gamedcs\university_window.cpp:327, dc 0x18f428) void type_university_window::set_selection_mode();
// CODEVIEW(E:\gamedcs\university_window.cpp:404, dc 0x18f7e4) int type_university_window::WindowHandler(message* msg);
// CODEVIEW(E:\gamedcs\university_window.cpp:479, dc 0x18f97c) int type_university_window::purchase_click(message* msg);
// CODEVIEW(E:\gamedcs\university_window.cpp:277, dc 0x18fb30) void* type_university_window::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_UNIVERSITY_WINDOW_H */
