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
    gImmEffectEntries.erase(m_enclosure.get());
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
    TImmMouseRuntime(void* hInst, void* hwnd);
    ~TImmMouseRuntime();
};

// DECODED, NOT LANDED (2026-09-06, claim lane 31). Retail's atexit thunk at
// 0x4b6910 - the address InitImmMouse hands to _atexit, 58 B - is this
// destructor EXPANDED, and it reads outright:
//
//     inline TImmMouseRuntime::~TImmMouseRuntime()
//     {
//         gImmProject->Close();
//         delete gImmProject;
//         delete gImmDevice;
//     }
//
// It touches no member: the holder is the eight bytes at 0x696d78 and the
// two singletons sit AFTER it at 0x696d84 and 0x696d80. `delete gImmProject`
// is the non-virtual imported dtor plus operator delete; `delete gImmDevice`
// is the virtual scalar deleting destructor (`push 1 / call [eax]`).
// Defining it here compiles and is byte-inert tree-wide (0 rows moved), but
// the CLAIM cannot bind: `VA_COMPGEN(0x004b6910, 0x3A, STATIC_DTOR,
// immMouse)` is matched by canonicalize_data_symbols.is_static_dtor, which
// requires the thunk to carry a relocation to the OWNED DATUM. This body
// never names `immMouse`, so the matcher finds zero candidates and warns the
// claim unbound. Landing the row needs that matcher to admit an
// owner-free static destructor - which is a real shape, not an anomaly:
// an empty holder whose teardown is entirely about globals.

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
