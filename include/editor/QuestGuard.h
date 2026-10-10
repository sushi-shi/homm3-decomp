// QuestGuard.h - a Quest Guard: a quest location that only lets a hero
// pass (QuestGuard.cpp, name inferred from its props dialog's unit).
//
// RTTI TQuestGuard <- TQuestLocation <- virtual TGameObject: the
// location's members, then the vtordisp and TGameObject (+0x48). It adds
// nothing to the location; the map's GUI objects copy it with the
// implicit copy constructor, so its vtables come from GUIGameObject.cpp.
#ifndef HOMM3_EDITOR_QUESTGUARD_H
#define HOMM3_EDITOR_QUESTGUARD_H

#include "editor/QuestLocation.h"

class TQuestGuard : public TQuestLocation {
public:
    TQuestGuard(const TObjectType& objType);
    TQuestGuard(const TObjectType& objType, TRawIStream* pIStream, int version);
};

#endif  /* HOMM3_EDITOR_QUESTGUARD_H */
