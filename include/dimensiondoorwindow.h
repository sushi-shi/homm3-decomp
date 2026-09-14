// dimensiondoorwindow.h - prototypes of dimensiondoorwindow.cpp (compiland dimensiondoorwindow.obj)
#ifndef HOMM3_DIMENSIONDOORWINDOW_H
#define HOMM3_DIMENSIONDOORWINDOW_H

#include "advmgr_popup.h"

class textWidget;

// The ESC scancode, in the message::codeX domain a MESSAGE_KEY_DOWN
// carries. Both handlers in this compiland close on it, so it sits at file
// scope rather than in either class; puzzlewindow.h spells the same value
// for the same role as a class-local enumerator.
enum EDimensionDoorKey {
    DIALOG_CLOSE_KEY = 1
};

// The compiland's two classes are the same shape, and the Dreamcast record
// says so outright: both are 92 bytes over CAdvPopup with exactly one
// member, RolloverWidget, at +0x58. Retail's 8-byte wider base moves it to
// +0x60 in both. Their tables are 0x63db9c and 0x63dbd8 - 0x3c apart, so
// each holds the fifteen slots the CAdvPopup hierarchy declares, which is
// what puts ExitDialog in slot 14.
// Before normalization (type): TDimensionDoorWindow.
class DimensionDoorWindow : public CAdvPopup {
public:
    DimensionDoorWindow();
    virtual ~DimensionDoorWindow();
    virtual int windowHandler(message& msg);
    virtual int exitDialog(message& msg);

private:
    textWidget* m_rolloverWidget;
};
SIZE(DimensionDoorWindow, 0x64);

// DC places the sole derived member at its CAdvPopup end (+0x58). Retail's
// base widening moves it to +0x60, and the stack instance in the skuttle-boat
// adventure action proves the resulting 0x64-byte canonical layout.
// Before normalization (type): TSkuttleBoatWindow.
class SkuttleBoatWindow : public CAdvPopup {
public:
    SkuttleBoatWindow();
    virtual ~SkuttleBoatWindow();
    virtual int windowHandler(message& msg);
    virtual int exitDialog(message& msg);

private:
    textWidget* m_rolloverWidget;
};
SIZE(SkuttleBoatWindow, 0x64);

// --- TDimensionDoorWindow ---
// CODEVIEW(E:\gamedcs\dimensiondoorwindow.cpp:71, dc 0x82eec) void* TDimensionDoorWindow::`scalar deleting destructor'(unsigned __flags);

// --- TSkuttleBoatWindow ---
// CODEVIEW(E:\gamedcs\dimensiondoorwindow.cpp:250, dc 0x82f20) void* TSkuttleBoatWindow::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_DIMENSIONDOORWINDOW_H */
