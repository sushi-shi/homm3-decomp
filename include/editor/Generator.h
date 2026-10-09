// Generator.h - the Armageddon's Blade random dwellings (Generator.cpp,
// name inferred: the unit that installs them).
//
// Each derives from TFlaggableObject first, then from its random parts;
// the parts carry vtables, so VC6 places them at the front and the
// flaggable part, whose vbptr the class shares, after them (RTTI:
// TFlaggableObject at +16 or +32).
//
// Ported so far: the classes the map's object factory names. Each returns
// its flaggable part to the random parts' pure virtuals.
#ifndef HOMM3_EDITOR_GENERATOR_H
#define HOMM3_EDITOR_GENERATOR_H

#include "editor/ObjectSpecializations.h"

// A dwelling of a fixed level whose alignment is random.
class TRandomlyAlignedGenerator : public TFlaggableObject, public TAbstractRandomlyAlignedGenerator {
public:
    virtual TFlaggableObject* getPFlaggableObject();
    virtual const TFlaggableObject* getPFlaggableObject() const;
};

// A dwelling of a fixed alignment whose level is random.
class TRandomlyLeveledGenerator : public TFlaggableObject, public TAbstractRandomlyLeveledGenerator {
public:
    virtual TFlaggableObject* getPFlaggableObject();
    virtual const TFlaggableObject* getPFlaggableObject() const;
};

// A dwelling whose alignment and level are both random.
class TRandomGenerator : public TFlaggableObject, public TAbstractRandomlyAlignedGenerator,
                         public TAbstractRandomlyLeveledGenerator {
public:
    virtual TFlaggableObject* getPFlaggableObject();
    virtual const TFlaggableObject* getPFlaggableObject() const;
};

#endif  /* HOMM3_EDITOR_GENERATOR_H */
