// subwindow.h - prototypes of subwindow.cpp (compiland subwindow.obj)
#ifndef HOMM3_SUBWINDOW_H
#define HOMM3_SUBWINDOW_H

#include <vector>
#include "va.h"

class Bitmap16Bit;
class heroWindow;
class widget;

// PROVEN retail layout (size 0x34). Both constructors at 0x5aa340 and
// 0x5aa3c0 store x/y/width/height at +4..+0x10, construct VC6's
// 16-byte vector at +0x14 (allocator byte + three pointers), then
// store parentWindow/lowId/highId/background at +0x24..+0x30.
// The sole vtable entry at 0x64234c is the scalar deleting destructor
// 0x5aa390, so only the destructor is virtual.
class SubWindow {
public:
    int m_x;
    int m_y;
    int m_width;
    int m_height;

protected:
    std::vector<widget*> m_widgets;
    heroWindow* m_parentWindow;

public:
    int m_lowId;
    int m_highId;

    SubWindow();
    SubWindow(int x, int y, int w, int h, heroWindow* parentWindow);
    virtual ~SubWindow();

    void initialize(int x, int y, int w, int h, heroWindow* parentWindow);
    void addWidget(widget* newWidget, int newPriority);
    void removeWidget(widget* killWidget);
    void draw(unsigned char update, int lowID, int highID);
    void saveBackground();
    void restoreBackground();

private:
    Bitmap16Bit* m_background;
};
SIZE(SubWindow, 0x34);

// --- TSubWindow ---
// CODEVIEW(E:\gamedcs\subwindow.cpp:111, dc 0x158ebc) void TSubWindow::RemoveWidget(widget* killWidget);
// CODEVIEW(E:\gamedcs\subwindow.cpp:44, dc 0x159094) void* TSubWindow::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_SUBWINDOW_H */
