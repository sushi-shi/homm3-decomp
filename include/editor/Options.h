// Options.h - the editor's saved settings (Options.cpp, name inferred from
// its alphabetical slot; GOG only). Each setting is a file-local value
// exported as a const reference and changed through its setter; load and
// save keep them in the registry's "Settings" section. The namespace,
// function and setting names are not recorded: they follow the registry
// value names.
#ifndef HOMM3_EDITOR_OPTIONS_H
#define HOMM3_EDITOR_OPTIONS_H

#include "va.h"

namespace SOptions {
DATA(0x005a23d4) extern const int& kZoom;
DATA(0x005a23d0) extern const bool& kGrid;
DATA(0x005a23cc) extern const bool& kPassability;
DATA(0x005a23c8) extern const bool& kAnimation;
DATA(0x005a23c4) extern const bool& kCycling;
DATA(0x005a23c0) extern const unsigned int& kAutosaveMinutes;
DATA(0x005a23bc) extern const unsigned int& kSpecialTileFrequency;

void load();
void setZoom(int zoom);
void setGrid(bool bGrid);
void setPassability(bool bPassability);
void setAnimation(bool bAnimation);
void setCycling(bool bCycling);
void setAutosaveMinutes(unsigned int minutes);
void setSpecialTileFrequency(unsigned int frequency);
void save();
}

#endif  /* HOMM3_EDITOR_OPTIONS_H */
