// CATALOG: D16
// PHENOMENON: /O2 implies /Oi; a plain `strcmp` call expands to VC6's
//   INLINE intrinsic - the two-bytes-per-iteration compare loop closing
//   with `sbb eax, eax / sbb eax, -1` (soundmgr::StartMP3's two name
//   compares are this idiom, written as plain strcmp calls in source).
//   /Op only disables the FLOATING-POINT intrinsics (sqrt et al.), not
//   the string ones - this probe compiles under the full game profile
//   including /Op.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(check): mov\s+dl, BYTE PTR \[eax\]
// EXPECT-ASM(check): add\s+eax, 2
// EXPECT-ASM(check): sbb\s+eax, eax[\s\S]{0,60}sbb\s+eax, -1
// EXPECT-NOT-ASM(check): call\s+_?\??strcmp
#include <string.h>
int gHit;
void check(const char* name) {
    if (strcmp(name, "combat.snd") == 0) gHit = 1;
}
