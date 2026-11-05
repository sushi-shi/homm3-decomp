#ifndef CREATURECAPACITY_H
#define CREATURECAPACITY_H

// Rows in the creature type tables. Retail Complete has 150 (ids 0..149);
// the HoMM2 mashup appends the HoMM2 Knight roster as ids 150..160.
enum {
    RETAIL_CREATURE_TYPE_COUNT = 150,
    CREATURE_TYPE_CAPACITY = 161
};

#endif
