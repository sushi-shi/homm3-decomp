// The abstract progress sink driven by Complete's random-map generator
// (the game's TRandomMapProgress and the map editor's generation dialog
// derive from it).
// Retail constructor 0x530e20 stores vtable 0x6409c0, the step total at +4,
// and zero at +8. The vtable holds a scalar deleting destructor at 0x530e40,
// SetTotal at 0x530e80, and _purecall in the Advance slot.
#ifndef HOMM3_PROGRESS_BAR_H
#define HOMM3_PROGRESS_BAR_H

#include "va.h"

class type_progress_bar {
public:
    int m_steps;
    int m_done;

    type_progress_bar(int totalSteps);
    virtual ~type_progress_bar();
    virtual void setTotal(int totalSteps);
    virtual void advance(int amount) = 0;
};
SIZE(type_progress_bar, 0xc);

#endif  // HOMM3_PROGRESS_BAR_H
