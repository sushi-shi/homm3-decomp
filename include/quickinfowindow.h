// quickinfowindow.h - prototypes of quickinfowindow.cpp (compiland quickinfowindow.obj)
#ifndef HOMM3_QUICKINFOWINDOW_H
#define HOMM3_QUICKINFOWINDOW_H

#include "armygrp.h"
#include "dialogbox.h"

// The retail caller allocates exactly 0x54 bytes and the destructor merely
// installs vtable 0x6406cc before tail-calling TDialogBox::~TDialogBox. Thus
// this class adds no storage to the byte-proven 0x54-byte base.
// Before normalization (type): TQuickCreatureWindow.
class QuickCreatureWindow : public DialogBoxWindow {
public:
// Before normalization (type): TQuickCreatureWindow::TViewLevel.
    enum ViewLevel {
        ViewNone = 0,
        ViewAll = 1
    };

// Before normalization (type): TQuickCreatureWindow::TDisposition.
    enum Disposition {
        Flee = 0,
        Attack = 1,
        Join = 2,
        JoinPrice = 3
    };

    QuickCreatureWindow(ViewLevel viewLevel, CreatureType id, int count,
                         Disposition disposition, int cost);
    virtual ~QuickCreatureWindow();
    void quickWindowWait();
};
SIZE(QuickCreatureWindow, 0x54);

// --- TQuickCreatureWindow ---
// CODEVIEW(E:\gamedcs\quickinfowindow.cpp:88, dc 0x117b8c) void TQuickCreatureWindow::QuickWindowWait();
// CODEVIEW(E:\gamedcs\quickinfowindow.cpp:77, dc 0x117bb4) void* TQuickCreatureWindow::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_QUICKINFOWINDOW_H */
