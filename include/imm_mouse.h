// Retail-only Immersion force-feedback mouse integration used by game.obj.
#ifndef HOMM3_IMM_MOUSE_H
#define HOMM3_IMM_MOUSE_H

#include <map>
#include <windows.h>
#include <va.h>

class CImmEnclosure {
public:
    // Slot 0 of the enclosure vtable. TAdventureMapWindow's owned effect
    // destroys its enclosure through it (`mov eax,[ecx] / push 1 /
    // call [eax]` at 0x4b6f05), which is the scalar-deleting-destructor
    // form and therefore proves a virtual destructor at slot 0. The two
    // slots the same effect calls through, +0x14 and +0x18, are further
    // down the same vtable and stay unmodelled.
    virtual ~CImmEnclosure();
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

// The effect TAdventureMapWindow owns at +0x9c, and the implementation it
// holds. Both are 8 bytes (`operator new(8)` at 0x4b6dc0+0x1c and at
// 0x401400+0x3d) laid out as a byte flag at +0 and a pointer at +4, and the
// pair is the ordinary pimpl shape: the holder's constructor builds the
// implementation and records whether it survived, its two forwarding members
// (0x4b6f30, 0x4b6f50) reach the enclosure as `m_impl->m_enclosure`, and its
// destructor at 0x4b6e40 deletes the implementation. RETAIL-ONLY - the
// Dreamcast build carries no Immersion layer, so every name here is
// role-derived and provisional, like the rest of this header.
class TImmMouseEffectImpl {
public:
    ~TImmMouseEffectImpl();

    unsigned char m_ownsEnclosure;
    CImmEnclosure* m_enclosure;
};

// Inlined into the holder's destructor as retail's only copy: the erase runs
// unconditionally on the enclosure key, the delete is guarded by the flag at
// +0, and both read the members back through the implementation pointer.
inline TImmMouseEffectImpl::~TImmMouseEffectImpl()
{
    gImmEffectEntries.erase(m_enclosure);
    if (m_ownsEnclosure)
        delete m_enclosure;
}

class TImmMouseEffect {
public:
    ~TImmMouseEffect();

    unsigned char m_created;
    TImmMouseEffectImpl* m_impl;
};

#endif
