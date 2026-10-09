// QuestGuard.h - a Quest Guard: a quest location that only lets a hero
// pass (QuestGuard.cpp, name inferred from its props dialog's unit).
//
// RTTI TQuestGuard <- TQuestLocation <- virtual TGameObject: the vtable,
// then the vbptr.
//
// Ported so far: the class the map's object factory names.
#ifndef HOMM3_EDITOR_QUESTGUARD_H
#define HOMM3_EDITOR_QUESTGUARD_H

#include "editor/QuestLocation.h"

class TQuestGuard : public TQuestLocation {
};

#endif  /* HOMM3_EDITOR_QUESTGUARD_H */
