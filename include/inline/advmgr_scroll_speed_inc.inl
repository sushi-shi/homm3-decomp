#include "../va.h"

// The per-speed scroll step. Dreamcast advmgr.obj publishes the static
// (S_LDATA32 akScrollSpeedInc); retail's ScreenScroll indexes the same
// three-int row.
DATA(0x0063a66c) static const int g_scrollSpeedInc[3] = { 1, 2, 3 };
