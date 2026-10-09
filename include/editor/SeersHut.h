// SeersHut.h - a Seer's Hut: a quest location that grants a reward
// (C:\Dev\Heroes 3 Exp 2\Editor\SeersHut.cpp, by its RTTI).
//
// RTTI TSeersHut <- TQuestLocation <- virtual TGameObject: the vtable,
// then the vbptr.
//
// Ported so far: the class the map's object factory names.
#ifndef HOMM3_EDITOR_SEERSHUT_H
#define HOMM3_EDITOR_SEERSHUT_H

#include "editor/QuestLocation.h"

class TSeersHut : public TQuestLocation {
};

#endif  /* HOMM3_EDITOR_SEERSHUT_H */
