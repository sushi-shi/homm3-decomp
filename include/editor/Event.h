// Event.h - a map event: a Pandora's Box that fires for the players it
// names (C:\Dev\Heroes 3 Exp 2\Editor\Event.cpp, by its RTTI).
//
// RTTI TEvent <- TBlackBox <- TTreasure <- virtual TGameObject.
//
// Ported so far: the class the map's object factory names.
#ifndef HOMM3_EDITOR_EVENT_H
#define HOMM3_EDITOR_EVENT_H

#include "editor/BlackBox.h"

class TEvent : public TBlackBox {
};

#endif  /* HOMM3_EDITOR_EVENT_H */
