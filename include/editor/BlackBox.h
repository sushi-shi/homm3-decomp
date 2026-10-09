// BlackBox.h - a Pandora's Box and its contents
// (C:\Dev\Heroes 3 Exp 2\Editor\BlackBox.cpp, by its RTTI).
//
// RTTI TBlackBox <- TTreasure <- virtual TGameObject: the treasure's vbptr
// comes first.
//
// Ported so far: the class the map's object factory names.
#ifndef HOMM3_EDITOR_BLACKBOX_H
#define HOMM3_EDITOR_BLACKBOX_H

#include "editor/ObjectSpecializations.h"

class TBlackBox : public TTreasure {
};

#endif  /* HOMM3_EDITOR_BLACKBOX_H */
