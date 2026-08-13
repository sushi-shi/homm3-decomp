// Retail-only Immersion force-feedback mouse integration used by game.obj.
#ifndef HOMM3_IMM_MOUSE_H
#define HOMM3_IMM_MOUSE_H

#include <map>
#include <windows.h>
#include <va.h>

class CImmEnclosure {
public:
    __declspec(dllimport) int SetRect(const RECT* rect);
};

// Retail-only names are role-derived and provisional. The singleton wrapper
// constructor initializes the origin and map before the window can move.
// ImmMouseWindowMoved's out-of-line red-black-tree iterator increment proves
// the container family, while node payload +0x0c/+0x10 is exactly
// pair<CImmEnclosure*, RECT>. Retail loads `_Head` at 0x696d64; VC6's
// Dinkumware map places that field at object +4, fixing the object base.
DATA(0x00696d60)
extern std::map<CImmEnclosure*, RECT> gImmEffectEntries;
DATA(0x00696d70) extern long gImmWindowX;
DATA(0x00696d74) extern long gImmWindowY;
DATA(0x00696d7c) extern HWND gImmWindow;

#endif
