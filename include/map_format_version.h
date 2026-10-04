// H3M wire versions, shared with native generator output.
#ifndef HOMM3_MAP_FORMAT_VERSION_H
#define HOMM3_MAP_FORMAT_VERSION_H

enum EMapFormatVersion {
    MAP_FORMAT_RESTORATION_OF_ERATHIA = 14,
    // Both byte-proven by readHeroData (0x5021c0), which gates on each of
    // them separately: `cmp esi,0x15` decides whether the hero record
    // carries a single signed spell id or a 70-bit mask, and
    // `cmp [gpGame+0x1f86c],0x1c` decides whether the equipped-artifact
    // band is eighteen positions or nineteen.
    MAP_FORMAT_ARMAGEDDONS_BLADE = 21,
    MAP_FORMAT_SHADOW_OF_DEATH = 28
};

#endif
