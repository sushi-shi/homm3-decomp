// extrainfo.h - the four-byte ExtraInfoUnion base flattened into map cells
#ifndef HOMM3_EXTRAINFO_H
#define HOMM3_EXTRAINFO_H

#include <va.h>

class BlackBoxData;
class type_creature_bank;
class type_university;

// Dreamcast names this four-byte base of NewmapCell; retail agrees on the
// shared dword and on these three accessors. NewmapCell currently carries the
// storage flattened at +0 to avoid expanding its broad include closure.
class ExtraInfoUnion {
public:
    unsigned long extraInfo;

    BlackBoxData* get_black_box();
    type_creature_bank* get_creature_bank();
    type_university* get_university();
};
SIZE(ExtraInfoUnion, 4);

#endif  /* HOMM3_EXTRAINFO_H */
