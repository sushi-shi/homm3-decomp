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
#include <stdexcept>
#include <string>
#include <windows.h>
#include <va.h>

#include "forcefeedback.h"

// Inlined into the holder's destructor as retail's only copy: the erase
// runs unconditionally on the enclosure key, and the delete is the
// auto_ptr member's own scope exit, guarded by the flag at +0.
inline force_feedback::t_enclosure::~t_enclosure()
{
    g_immEffectEntries.erase(m_enclosure.get());
}

// The effect TAdventureMapWindow owns at +0x9c. Eight bytes with the same
// `{ bool _Owns; T* _Ptr; }` shape as the enclosure wrapper it holds -
// retail's constructor at 0x4b6dc0 buys the implementation with
// `operator new(8)`, then writes `(p != 0)` and `p`, which is
// `auto_ptr<t_enclosure>(new t_enclosure(...))` verbatim. RETAIL-ONLY -
// the Dreamcast build carries no Immersion layer, so the holder's own
// name is role-derived and provisional.
// The Immersion layer's one-shot initializer, held by InitImmMouse as a
// function-local static (0x696d78, guard 0x696d58). RETAIL RTTI NAMES THE
// CLASS: the throw record at 0x64cc18 publishes
// `.?AVt_initialize_failure@t_initializer@?%C:\Dev\Heroes 3 Exp 2\Game\
// ForceFeedback.cpp210603558@@`, so retail's own spelling is `t_initializer`
// in ForceFeedback.cpp's UNNAMED namespace with the failure type nested
// inside it. We cannot spell an unnamed-namespace class here because
// InitImmMouse's claim lives in game.cpp, so the role name stays and the
// nested failure type keeps retail's nesting.
class TImmMouseRuntime {
public:
    // `.?AVt_initialize_failure@t_initializer@...@@` (0x6778c0), a 28-byte
    // runtime_error with no members of its own - the same shape as
    // t_enclosure::t_create_failure, and thrown the same way, with a
    // DEFAULT-constructed string handed to the base.
    class t_initialize_failure : public std::runtime_error {
    public:
        t_initialize_failure() : std::runtime_error(std::string()) {}
    };

    // Retail 0x4b6260, 1122 bytes: the window origin, the iFeel error
    // policy, the mouse device, the effect project read from H3Shad.ifr
    // (with a LOD fallback in its catch), and the three globals it
    // publishes.
    // Before normalization (locals): hInst.
    TImmMouseRuntime(void* instance, void* hwnd);
    // Retail's atexit thunk at 0x4b6910 - the address InitImmMouse hands to
    // _atexit, 58 B - is this destructor EXPANDED, so it is defined inline
    // here.  It touches no member: the holder is the eight bytes at
    // 0x696d78 and the two singletons sit AFTER it at 0x696d84 and
    // 0x696d80.  `delete gImmProject` is the non-virtual imported dtor plus
    // operator delete; `delete gImmDevice` is the virtual scalar deleting
    // destructor (`push 1 / call [eax]`).  Because the body names no
    // member, the `$E<n>` thunk carries no relocation to the owned datum -
    // see canonicalize_data_symbols' owner-free static-destructor arm and
    // its control in build/test_ownerless_static_dtor.py.
    ~TImmMouseRuntime()
    {
        g_immProject->Close();
        delete g_immProject;
        delete g_immDevice;
    }
};

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
