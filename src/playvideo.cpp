// 1 functions in link order.
#include <va.h>
// #include "playvideo.h"

// E:\gamedcs\playvideo.cpp:26
// Dreamcast-port-only: its body loads L"playsfddll.dll", resolves
// L"PlayVideo", passes the surfaces/audio/filename plus PalMode and
// StopVideo addresses to that entry point, then calls
// FreeLibrary. Complete videoOpen (0x597570) instead selects its retained
// Smacker or Bink backend, using RAD imports and the resource archives.
// There is no SFD-DLL/palette-mode/stop-flag interface in that path.
// Exact platform disposition: config/dc_only.tsv.
DC_ONLY(0x114c20, 0x94)
void playVideoDll(IDirectDraw4* Vddraw, IDirectDrawSurface4* dds_prim, IDirectDrawSurface4* dds_back, IDirectSound* dsobj, unsigned short* fname)
{
    // @stub
}
