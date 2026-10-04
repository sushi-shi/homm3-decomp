// objectmask.h - adventure-object mask frame.
#ifndef HOMM3_OBJECTMASK_H
#define HOMM3_OBJECTMASK_H

// Every object type's draw, passable, shadow and trigger masks share one
// fixed frame, anchored at the object's bottom-right tile; objects.txt, .h3m
// maps and saves all store it. An object's own footprint fits inside it.
enum EObjectMaskFrame {
    OBJECT_MASK_WIDTH = 8,
    OBJECT_MASK_HEIGHT = 6,
    OBJECT_MASK_CELLS = OBJECT_MASK_WIDTH * OBJECT_MASK_HEIGHT
};

// Slot values referenced by the RMG's footprint and selection rules.
// Other editor categories retain their original integer identities.
enum EObjectSlotCategory {
    OBJECT_SLOT_CATEGORY_0 = 0,
    OBJECT_SLOT_CATEGORY_4 = 4,
    OBJECT_SLOT_CATEGORY_5 = 5
};

#endif
