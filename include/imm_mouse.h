// Retail-only Immersion force-feedback mouse integration used by game.obj.

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

// The effect holder is the public eight-byte handle; its implementation and
// RTTI-proven local initializer are defined in forcefeedback.cpp.
class TImmMouseEffect {
public:
    TImmMouseEffect(const RECT* rect, long a, unsigned long b,
                    unsigned long c, unsigned char d, unsigned char e);
    ~TImmMouseEffect();
    unsigned char start();
    void stop();

    std::auto_ptr<force_feedback::t_enclosure> m_impl;
};

#endif
