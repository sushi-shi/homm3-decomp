// Retail-only Immersion force-feedback mouse integration used by game.obj.
//
// The Immersion API and globals live in forcefeedback.h. ForceFeedback.cpp
// owns both implementation classes and every body of this effect holder.
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
