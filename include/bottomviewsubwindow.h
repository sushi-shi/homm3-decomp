// bottomviewsubwindow.h - prototypes of bottomviewsubwindow.cpp (compiland bottomviewsubwindow.obj)
#ifndef HOMM3_BOTTOMVIEWSUBWINDOW_H
#define HOMM3_BOTTOMVIEWSUBWINDOW_H

#include "subwindow.h"

class iconWidget;
class textWidget;

// PROVEN narrow retail layouts. The constructor reached by
// advManager::UpdBottomViewEnemyTurn allocates 0x74 bytes and uses the
// TSubWindow subobject at offset zero; its first derived-field access is
// +0x34. The retail type_bottom_view_window vtable at 0x63bb04 has two
// entries, agreeing with the Dreamcast record's virtual destructor and
// animate slot. No unobserved derived fields are named here.
class type_bottom_view_window : public SubWindow {
public:
    // Widget ids shared by the family; read off the constructors, which
    // pass 0x7d0 for the backdrop and 0x834 for the text.
    enum EWidgetIDs {
        BOTTOM_VIEW_BACKGROUND_ID = 0x7d0,
        BOTTOM_VIEW_TEXT_ID = 0x834,
        // The enemy-turn window numbers its own widgets from a separate
        // base; its constructor seeds one incrementing local with 0x88e.
        BOTTOM_VIEW_ENEMY_TURN_ID = 0x88e,
        // The town banner's silo row has exactly two shapes, selected by
        // how many of get_silo_income's seven entries are non-zero:
        // TWO stacks a pair of icons at y=75 and y=87, ONE centres a
        // single icon at y=81, and anything else draws nothing. The
        // count IS the domain the constructor switches on, so it is
        // named here rather than compared against a bare literal.
        BOTTOM_VIEW_SILO_ONE_RESOURCE = 1,
        BOTTOM_VIEW_SILO_TWO_RESOURCES = 2
    };

    type_bottom_view_window(heroWindow* parentWindow);
    virtual ~type_bottom_view_window();
    virtual void animate();
};
SIZE(type_bottom_view_window, 0x34);

// Retail allocates each of these three presentation-only subclasses at the
// unchanged 0x34-byte base extent. Dreamcast supplies the real class and
// constructor identities; their UI state is owned by the inherited window.
// Before normalization (type): TBottomViewHero.
#ifndef BottomViewHero
#define BottomViewHero TBottomViewHero
#endif
class BottomViewHero : public type_bottom_view_window {
public:
    BottomViewHero(heroWindow* parent);
    virtual ~BottomViewHero();
};
SIZE(BottomViewHero, 0x34);

// Before normalization (type): TBottomViewTown.
#ifndef BottomViewTown
#define BottomViewTown TBottomViewTown
#endif
class BottomViewTown : public type_bottom_view_window {
public:
    BottomViewTown(heroWindow* parent);
    virtual ~BottomViewTown();
};
SIZE(BottomViewTown, 0x34);

// Before normalization (type): TBottomViewKingdom.
#ifndef BottomViewKingdom
#define BottomViewKingdom TBottomViewKingdom
#endif
class BottomViewKingdom : public type_bottom_view_window {
public:
    BottomViewKingdom(heroWindow* parent);
    virtual ~BottomViewKingdom();
};
SIZE(BottomViewKingdom, 0x34);

// Slot 1 of 0x63bb44 is this class's own body 0x4536f0, so it overrides
// animate; the five classes whose slot 1 is still the base's 0x5bc690 do
// not. Every derived destructor here is virtual and empty in retail - all
// seven bodies are byte-identical apart from their EH table pointer, each
// one the base destructor inlined whole.
// The 0x40-byte derived tail is byte-proven twice over - the constructor
// writes every field and animate reads every one of them back.
// Before normalization (type): TBottomViewEnemyTurn.
#ifndef BottomViewEnemyTurn
#define BottomViewEnemyTurn TBottomViewEnemyTurn
#endif
class BottomViewEnemyTurn : public type_bottom_view_window {
public:
    // +0x34, 'crest58.def' at (20,51). Its FRAME is the acting player's
    // game position, and animate re-frames it whenever that position
    // changes - which is what pairs it with lastPlayerPos below.
    iconWidget* m_crest;
    // +0x38 / +0x3c, the two halves of the hourglass at (98,51):
    // 'HourGlas.def' is the animated one (animate reads ITS sprite's
    // sequence-0 frame count) and 'HourSand.def' the level indicator.
    iconWidget* m_hourGlass;
    iconWidget* m_sand;
    unsigned long m_lastStepTime;  // +0x40, GameTime::Get at the last step
    // +0x44. One entry per player, filled here and refreshed by animate
    // through the SAME sum_mobility below, which is why the two bodies
    // carry the identical inlined loop.
    long m_mobility[8];
    int m_lastPlayerPos;           // +0x64
    int m_frame;                   // +0x68
    int m_frameDelay;              // +0x6c, 50 ticks
    int m_step;                    // +0x70

    BottomViewEnemyTurn(heroWindow* parent);
    virtual ~BottomViewEnemyTurn();
    virtual void animate();

    // dc 0x56bbc, :646. No retail body: /Ob2 expands it into both of
    // its call sites and the unreferenced copy is dropped.
    long sumMobility(long playerId);
};
SIZE(BottomViewEnemyTurn, 0x74);

// Retail UpdBottomViewNewTurn allocates 0x48 bytes before invoking the
// Dreamcast-attested constructor. Slot 1 of 0x63bb0c is 0x4511a0, this
// class's own animate, and that body proves the entire 0x14-byte derived
// tail on its own: it reads all five fields and writes two of them. The
// Dreamcast dump carries NO fieldlist for this class - only its four
// function symbols - so every SPELLING below is the house ordinal
// convention applied to a proven role, not an attested name.
// Before normalization (type): TBottomViewNewTurn.
#ifndef BottomViewNewTurn
#define BottomViewNewTurn TBottomViewNewTurn
#endif
class BottomViewNewTurn : public type_bottom_view_window {
public:
    // +0x34. A textWidget, redrawn through slot 4 immediately after the
    // icon on every frame step. The DERIVED type is byte-proven by the
    // constructor, not by animate (slot 4 is widget's own Draw either
    // way): retail copies the pointer into a stack temporary before
    // handing it to Widgets.push_back, and only a textWidget*->widget*
    // conversion makes that temporary exist - a plain widget* member is
    // passed by address (`lea ecx,[esi+0x34]`).
    textWidget* m_backdrop;
    // +0x38. An iconWidget: animate calls SetIconFrame and send_message
    // on it and reads Sprite->GetNumFrames(0) through it.
    iconWidget* m_icon;
    int m_frame;                   // +0x3c, the current sequence-0 frame
    int m_frameDelay;              // +0x40, ticks between steps
    unsigned long m_lastStepTime;  // +0x44, GameTime::Get at the last step

    BottomViewNewTurn(heroWindow* parent);
    virtual ~BottomViewNewTurn();
    virtual void animate();
};
SIZE(BottomViewNewTurn, 0x48);

// Resource-message state is supplied by advManager; retail allocates no
// derived storage beyond the 0x34-byte bottom-view base.
// Before normalization (type): TBottomViewResourceMessage.
#ifndef BottomViewResourceMessage
#define BottomViewResourceMessage TBottomViewResourceMessage
#endif
class BottomViewResourceMessage : public type_bottom_view_window {
public:
    BottomViewResourceMessage(heroWindow* parent, int res,
                               int quantity, const std::string* message);
    virtual ~BottomViewResourceMessage();
};
SIZE(BottomViewResourceMessage, 0x34);

// Before normalization (type): TBottomViewMessage.
#ifndef BottomViewMessage
#define BottomViewMessage TBottomViewMessage
#endif
class BottomViewMessage : public type_bottom_view_window {
public:
    BottomViewMessage(heroWindow* parent, const std::string* message);
    virtual ~BottomViewMessage();
};
SIZE(BottomViewMessage, 0x34);

// --- TBottomViewEnemyTurn ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:646, dc 0x56bbc) long TBottomViewEnemyTurn::sum_mobility(long player_id);
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:643, dc 0x57038) void* TBottomViewEnemyTurn::`scalar deleting destructor'(unsigned __flags);

// --- TBottomViewHero ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:222, dc 0x558a8) void TBottomViewHero::TBottomViewHero(heroWindow* parent);
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:339, dc 0x56f54) void* TBottomViewHero::`scalar deleting destructor'(unsigned __flags);

// --- TBottomViewKingdom ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:515, dc 0x563b8) void TBottomViewKingdom::TBottomViewKingdom(heroWindow* parent);
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:586, dc 0x56fec) void* TBottomViewKingdom::`scalar deleting destructor'(unsigned __flags);

// --- TBottomViewMessage ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:209, dc 0x56f08) void* TBottomViewMessage::`scalar deleting destructor'(unsigned __flags);

// --- TBottomViewNewTurn ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:118, dc 0x56e70) void* TBottomViewNewTurn::`scalar deleting destructor'(unsigned __flags);

// --- TBottomViewResourceMessage ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:146, dc 0x554ac) void TBottomViewResourceMessage::TBottomViewResourceMessage(heroWindow* parent, int res, int quantity, const std::basic_string<char,std::char_traits<char>,std::allocator<char>* message);
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:183, dc 0x56ebc) void* TBottomViewResourceMessage::`scalar deleting destructor'(unsigned __flags);

// --- TBottomViewTown ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:352, dc 0x55df4) void TBottomViewTown::TBottomViewTown(heroWindow* parent);
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:503, dc 0x56fa0) void* TBottomViewTown::`scalar deleting destructor'(unsigned __flags);

// --- std ---
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0x57084) widget** std::vector<widget *,std::allocator<widget *> >::operator[](unsigned __n);

// --- type_bottom_view_window ---
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:39, dc 0x550b8) void type_bottom_view_window::type_bottom_view_window(heroWindow* parent_window);
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:64, dc 0x55188) void type_bottom_view_window::animate();
// CODEVIEW(E:\gamedcs\bottomviewsubwindow.cpp:41, dc 0x56e3c) void* type_bottom_view_window::`scalar deleting destructor'(unsigned __flags);

// --- widget ---
// CODEVIEW(E:\gamedcs\Widget.h:271, dc 0x56e20) void widget::force_update();

#endif  /* HOMM3_BOTTOMVIEWSUBWINDOW_H */
