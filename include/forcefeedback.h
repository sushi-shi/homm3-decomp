// forcefeedback.h - the Immersion iFeel surface ForceFeedback.obj links
// against, and the two force-feedback objects that compiland defines.

// COMPILAND NAME IS RETAIL RTTI, NOT A GUESS. The two throw records in
// this block publish their type descriptors: 0x6778c0 reads
// `.?AVt_initialize_failure@t_initializer@?%C:\Dev\Heroes 3 Exp 2\Game\
// ForceFeedback.cpp210603558@@` and 0x65f2b0 reads
// `.?AVt_create_failure@t_enclosure@force_feedback@@`. So the source file
// is ForceFeedback.cpp, `t_initializer` lives in an unnamed namespace
// (that is what the `?%<path><number>` scope spells) and the enclosure
// wrapper is `force_feedback::t_enclosure`. Everything else here is
// role-derived and provisional - the Dreamcast build carries no Immersion
// layer, so no CodeView row attests any of it.

// The Immersion classes themselves are NOT ours: IFC20.dll exports them
// and the import table publishes their decorated names verbatim (IAT
// 0x63a03c..0x63a0a0). The active vendor IFC.h carries the 2.0.3 declarations
// and layouts proved by those manglings and by retail's two client-side
// vtables at 0x63e618 (CImmMouse) and 0x63e640 (CImmEnclosure).
#ifndef HOMM3_FORCEFEEDBACK_H
#define HOMM3_FORCEFEEDBACK_H

#include <IFC.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <windows.h>

#include "va.h"

// IFC20.dll 2.0.3 and the client vtables prove these active vendor layouts.
SIZE(CImmDevice, 0x24);
SIZE(CImmMouse, 0x2c);
SIZE(CImmEffect, 0xa4);
SIZE(CImmEnclosure, 0xe0);
SIZE(CImmProject, 0x10);

// Retail RTTI names this game-owned class in ForceFeedback.cpp's unnamed
// namespace. Keeping its first declaration here preserves VC6's original
// anonymous-namespace identity for the out-of-line definitions below.
namespace { class t_initializer; }

// --- ForceFeedback.obj's own objects ---

// The window origin the enclosure rectangles are kept relative to, and
// the enclosure->rectangle map ImmMouseWindowMoved walks. Retail loads
// the map's `_Head` at 0x696d64; VC6's Dinkumware map places that field
// at object +4, which fixes the object base at 0x696d60.
DATA(0x00696d60)
extern std::map<CImmEnclosure*, RECT> g_immEffectEntries;
// Retail 0x4b6260 passes 0x696d70 to ClientToScreen, which owns both
// LONG coordinates. 0x4b6950 and 0x4b6a50 consume its x/y at +0/+4.
DATA(0x00696d70) extern POINT g_immWindowOrigin;
DATA(0x00696d7c) extern HWND g_immWindow;

// The three singletons the initializer publishes: the mouse (handed out
// as the device everywhere), the loaded project, and the effect currently
// playing. playImmEffect destroys the previous effect before creating the
// next, so the last is a single slot rather than a set.
DATA(0x00696d80) extern CImmDevice* g_immDevice;
DATA(0x00696d84) extern CImmProject* g_immProject;
DATA(0x00696d88) extern CImmCompoundEffect* g_immEffect;

namespace force_feedback { class t_enclosure; }

unsigned char playImmEffect(const char* effectName, int count);  // 0x4b69f0

#endif  /* HOMM3_FORCEFEEDBACK_H */
