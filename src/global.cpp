#include <va.h>
// #include "global.h"

// RETAIL-DROPPED (2026-08-11): both Dreamcast roster rows are STLport
// bitset constructors. The x86 build uses VC6's shipped Dinkumware surface;
// no corresponding two-word constructor entries occur after global.obj's
// terrain initializer run. That retail bracket instead continues with the
// Complete-only TGzInflateBuf family beginning at 0x4d6050, which has no
// Dreamcast source row and is separate reconstruction work. Do not turn the
// retail-only family into guessed claims.
