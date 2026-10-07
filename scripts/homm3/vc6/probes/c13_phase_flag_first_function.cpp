// CATALOG: C13
// PHENOMENON: the C2 phase flag (.bssbe 0x9f120; docs/vc6/phase-flag.md).
//   The global-optimizer driver leaves 1 behind after every function, but
//   the TU's first globally optimized function starts from the zeroed
//   back-end state. With 0, the driver's opening simplification sweep may
//   not fold branches whose compare has a constant outcome (here the three
//   tests of the inlined armyName(48, 2)); they survive until stage 1 and
//   the CFG is cleaned only afterwards. first_ and second_ have identical
//   source and differ only in compile position: first_ pushes the name
//   right after loading it, second_ loads before pushing (one byte longer).
// FLAGS: /O2 /Ob2 /Oy- /Op /MT /Gr /GX /D_WINDOWS
// EXPECT-ASM(first_): PROC\s+mov\s+eax, DWORD PTR \?g_traits@@3PAUTraits@@A\+\d+\s+push\s+eax\s
// EXPECT-ASM(second_): PROC\s+mov\s+eax, DWORD PTR \?g_traits@@3PAUTraits@@A\+\d+\s+mov\s+edx, DWORD PTR \?g_text@@3PAUText@@A\s+push\s+eax\s
#include <stdio.h>

struct Traits { int a, b, c, d, e, f; const char* single; const char* plural; int pad[21]; };
extern Traits g_traits[151];
struct Text { const char** strings; const char* get(int i) const { return strings[i]; } };
extern Text* g_text;

inline const char* armyName(int id, int count)
{
    if (id < 0 || id > 150)
        return 0;
    return count == 1 ? g_traits[id].single : g_traits[id].plural;
}

void first_(char* buffer) { sprintf(buffer, g_text->get(0x2be), armyName(48, 2)); }
void second_(char* buffer) { sprintf(buffer, g_text->get(0x2be), armyName(48, 2)); }
