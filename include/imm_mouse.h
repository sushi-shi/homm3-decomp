// Retail-only Immersion force-feedback mouse integration used by game.obj.
//
// The Immersion API itself, the enclosure wrapper and the globals now live
// in forcefeedback.h beside the compiland that defines them (RTTI-proven
// ForceFeedback.cpp); this header keeps only the effect holder game.obj
// destroys and the entry points game.obj calls.
#ifndef HOMM3_IMM_MOUSE_H
#define HOMM3_IMM_MOUSE_H

#include <map>
#include <memory>
#include <windows.h>
#include <va.h>

#include "forcefeedback.h"

// Inlined into the holder's destructor as retail's only copy: the erase
// runs unconditionally on the enclosure key, and the delete is the
// auto_ptr member's own scope exit, guarded by the flag at +0.
inline force_feedback::t_enclosure::~t_enclosure()
{
    gImmEffectEntries.erase(m_enclosure.get());
}

// The effect TAdventureMapWindow owns at +0x9c. Eight bytes with the same
// `{ bool _Owns; T* _Ptr; }` shape as the enclosure wrapper it holds -
// retail's constructor at 0x4b6dc0 buys the implementation with
// `operator new(8)`, then writes `(p != 0)` and `p`, which is
// `auto_ptr<t_enclosure>(new t_enclosure(...))` verbatim. RETAIL-ONLY -
// the Dreamcast build carries no Immersion layer, so the holder's own
// name is role-derived and provisional.
class TImmMouseEffect {
public:
    TImmMouseEffect(const RECT* rect, long a, unsigned long b,
                    unsigned long c, unsigned char d, unsigned char e);
    ~TImmMouseEffect();
    unsigned char Start();
    void Stop();

    std::auto_ptr<force_feedback::t_enclosure> m_impl;
};

#endif
