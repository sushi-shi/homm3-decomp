// combatcontrolsubwindow.h - prototypes of combatcontrolsubwindow.cpp (compiland combatcontrolsubwindow.obj)
#ifndef HOMM3_COMBATCONTROLSUBWINDOW_H
#define HOMM3_COMBATCONTROLSUBWINDOW_H

#include "subwindow.h"

class bitmapBackedTextWidget;
class message;
class bitmapBorder;
class hero;
class type_func_button;
class iconWidget;
class textWidget;
class army;

// The family base. Retail's 0x46b610 constructor takes (parent, sprite
// name), chains ??0TSubWindow, stores the 0x63d410 vptr and one dword at
// +0x34. The two derived tables 0x63d420 and 0x63d430 follow 0x63d410 at a
// 16-byte stride, so every table in the family holds FOUR slots - exactly
// the Dreamcast record's virtual set: the destructor, set_rollover,
// set_rollover_buttons and DisableAllButtons (see the CODEVIEW block).

// The base's extent is NOT settled and no derived field is named here.
// 0x4721d0 allocates TCombatPlacementSubWindow at 0x3c and
// TCombatControlSubWindow at 0x40; the base constructor writes +0x34, the
// control constructor writes +0x38 and +0x3c, and the placement
// constructor writes neither - which leaves the +0x38 dword attributable
// either to the base or to each derived class. The three destructors
// below need none of it: every one is the base body, and the base body
// touches only the inherited TSubWindow subobject.
class type_combat_sub_window : public TSubWindow {
public:
    // +0x34, and the base owns it: the base constructor NULLS it and the
    // family's set_rollover slot is what reads it. Only
    // TCombatControlSubWindow fills it in - with a bitmapBackedTextWidget
    // over 'cRollovr.pcx' - and the derived TYPE is what the constructor
    // proves, because push_back has to build a widget* temporary for it.
    bitmapBackedTextWidget* m_rolloverWidget;

    type_combat_sub_window(heroWindow* parent,
                           const char* backgroundSpriteName);
    virtual ~type_combat_sub_window();
    // Slots 1 and 2 inherit the image-wide empty-body folds at 0x485d80
    // (`ret 4`) and 0x5bc7e0 (`ret 8`). The Dreamcast decorated publics
    // independently preserve the same PBD/JJ arguments; only the generated
    // prototype comments below lost them.
    virtual void setRollover(const char* newText);
    // Before normalization (function): type_combat_sub_window::set_rollover_buttons.
    virtual void setRolloverButtons(long first, long second);
    // Before normalization (function): type_combat_sub_window::DisableAllButtons.
    virtual void disableAllButtons();
};
SIZE(type_combat_sub_window, 0x38);

// Its constructor stores 0x63d420 and passes "cbar.pcx" (0x670030) to the
// base; the sole construction site is 0x4721d0, reached from
// combatManager::Open. The destructor body is empty in retail - all 120
// bytes are the base destructor inlined whole.
class TCombatControlSubWindow : public type_combat_sub_window {
public:
    // The two 'ComSlide.def' arrows over the combat message log, +0x38 and
    // +0x3c; the constructor is the only body that writes either, and it
    // dims both at the end.
    type_func_button* m_logScrollUpButton;
    type_func_button* m_logScrollDownButton;

    TCombatControlSubWindow(heroWindow* parent);
    virtual ~TCombatControlSubWindow();
    virtual void setRollover(const char* newText);
    // Before normalization: set_rollover_buttons; retained DC public: JJ.
    virtual void setRolloverButtons(long first, long second);
    // Before normalization: DisableAllButtons; DC271 delegates to the base.
    virtual void disableAllButtons();
};
SIZE(TCombatControlSubWindow, 0x40);

// The same shape one class over: 0x63d430, "coplacbr.pcx" (0x670054), the
// same 0x4721d0 construction site, the same empty destructor.
class TCombatPlacementSubWindow : public type_combat_sub_window {
private:
    // 0x4721d0 allocates this class at 0x3c against the base's 0x38, and
    // no body in the image writes the difference.
    char m_pad038[4];

public:
    TCombatPlacementSubWindow(heroWindow* parent);
    virtual ~TCombatPlacementSubWindow();
    virtual void disableAllButtons();
};
SIZE(TCombatPlacementSubWindow, 0x3c);

// Retail's two construction sites allocate 0x5c bytes. The constructor and
// Show/UnShow pair independently fix the TSubWindow base, the nine pointer
// fields at +0x34..+0x54, and the shown byte at +0x58.
class TCombatHeroSubWindow : public TSubWindow {
public:
    bitmapBorder* m_backgroundWidget;
    bitmapBorder* m_portrait;
    textWidget* m_attackText;
    textWidget* m_defenseText;
    textWidget* m_powerText;
    textWidget* m_knowledgeText;
    iconWidget* m_moraleIcon;
    iconWidget* m_luckIcon;
    textWidget* m_manaText;
    bool m_shown;

    TCombatHeroSubWindow(int x, int y, int w, int h, heroWindow* parent);
    virtual ~TCombatHeroSubWindow();
    void update(const hero& info, const hero* otherHero,
                bool onCursedGround);
    void show();
    void unShow();
    bool isShown() const { return m_shown; }
};
SIZE(TCombatHeroSubWindow, 0x5c);

// Retail's two constructor arms close the entire 0x34..0x6f tail. The
// full-stat arm creates the compact creature portrait and six statistic
// rows; both arms create the three standing-spell icons and their text
// overlay. DrawCreatureAndHeroSubwindows independently proves the shown
// byte at +0x68 for each of TCombatWindow's four panels.
class TCombatCreatureSubWindow : public TSubWindow {
public:
    bitmapBorder* m_backgroundWidget;  // +0x34
    iconWidget* m_creatureIcon;         // +0x38, full-stat arm only
    textWidget* m_attackText;           // +0x3c, full-stat arm only
    textWidget* m_defenseText;          // +0x40, full-stat arm only
    textWidget* m_damageText;           // +0x44, full-stat arm only
    textWidget* m_speedText;            // +0x48, full-stat arm only
    iconWidget* m_moraleIcon;            // +0x4c, full-stat arm only
    iconWidget* m_luckIcon;              // +0x50, full-stat arm only
    textWidget* m_countText;             // +0x54, full-stat arm only
    iconWidget* m_spellIcons[3];         // +0x58
    textWidget* m_spellText;             // +0x64
    bool m_shown;
    // Retail shown is a byte at +0x68; viewLevel is an int at +0x6c.
    // These three bytes align the integer.
    char m_paddingBeforeViewLevel[3];
    int m_viewLevel;                     // +0x6c

    TCombatCreatureSubWindow(int x, int y, int w, int h,
                             heroWindow* parent, int viewLevel);

    // Its 0x63d444 table holds exactly one slot: 0x63d440 (the sibling
    // TCombatHeroSubWindow's table) sits four bytes earlier, so neither
    // class adds a virtual beyond the destructor.
    virtual ~TCombatCreatureSubWindow();

    // Dreamcast's public symbols preserve these two source boundaries and
    // ProcessCombatMsg's retail call sites independently prove their
    // pointer ABI.  They remain out of line just like the sibling hero
    // panel methods.
    // Before normalization (function): TCombatCreatureSubWindow::Update.
    void update(const army& info, const hero* owner);
    // Before normalization (function): TCombatCreatureSubWindow::Show.
    void show();

    void unShow();
};
SIZE(TCombatCreatureSubWindow, 0x70);

// --- TCombatControlSubWindow ---
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:227, dc 0x65270) void TCombatControlSubWindow::set_rollover_buttons();
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:261, dc 0x65298) void TCombatControlSubWindow::DisableAllButtons();
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:214, dc 0x66a6c) void* TCombatControlSubWindow::`scalar deleting destructor'(unsigned __flags);

// --- TCombatCreatureSubWindow ---
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:562, dc 0x65dbc) void TCombatCreatureSubWindow::TCombatCreatureSubWindow(int x, int y, int w, int h, heroWindow* parent, int view_level);
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:688, dc 0x66648) void TCombatCreatureSubWindow::Update(const army* info, const hero* owner);
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:655, dc 0x66b08) void* TCombatCreatureSubWindow::`scalar deleting destructor'(unsigned __flags);

// --- TCombatHeroSubWindow ---
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:396, dc 0x66ad4) void* TCombatHeroSubWindow::`scalar deleting destructor'(unsigned __flags);

// --- TCombatPlacementSubWindow ---
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:282, dc 0x652a8) void TCombatPlacementSubWindow::TCombatPlacementSubWindow(heroWindow* parent);
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:308, dc 0x66aa0) void* TCombatPlacementSubWindow::`scalar deleting destructor'(unsigned __flags);

// --- button ---
// CODEVIEW(E:\gamedcs\button.h:99, dc 0x669f4) void button::set_disabled_frame(long frame);

// --- hero ---
// CODEVIEW(E:\gamedcs\Hero.h:634, dc 0x669fc) int hero::GetMaxMana();

// --- std ---
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0x66b3c) void std::vector<widget *,std::allocator<widget *> >::vector<widget *,std::allocator<widget *> >(const std::allocator<widget* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0x66b58) void std::vector<widget *,std::allocator<widget *> >::~vector<widget *,std::allocator<widget *> >();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0x66b80) void std::allocator<widget *>::allocator<widget *>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0x66b84) void std::allocator<widget *>::~allocator<widget *>();
// CODEVIEW(..\stlport\stl_deque.h:583, dc 0x66b88) const SpellID* std::deque<enum SpellID,std::allocator<enum SpellID>,0>::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0x66ba0) void std::_Vector_base<widget *,std::allocator<widget *> >::_Vector_base<widget *,std::allocator<widget *> >(const std::allocator<widget* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0x66bcc) void std::_Vector_base<widget *,std::allocator<widget *> >::~_Vector_base<widget *,std::allocator<widget *> >();
// CODEVIEW(..\stlport\stl_deque.h:312, dc 0x66bfc) SpellID* std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator[](int __n);
// CODEVIEW(..\stlport\stl_string.h:469, dc 0x66c24) void std::_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >::~_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >();
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0x66c3c) void std::_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >::_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >(const std::allocator<widget* __a, widget*** __p);

// --- type_combat_sub_window ---
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:136, dc 0x64f60) void type_combat_sub_window::set_rollover();
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:140, dc 0x64f64) void type_combat_sub_window::set_rollover_buttons();
// CODEVIEW(E:\gamedcs\combatcontrolsubwindow.cpp:116, dc 0x66a38) void* type_combat_sub_window::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_COMBATCONTROLSUBWINDOW_H */
