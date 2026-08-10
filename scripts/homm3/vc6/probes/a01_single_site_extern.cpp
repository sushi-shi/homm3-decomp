// CATALOG: A1 A2
// PHENOMENON: /Ob2 single-call-site inlining is unconditional and applies to
//   extern functions; extern linkage still emits the out-of-line copy IN
//   ADDITION to the inline expansion (armygrp 0x44a460, AI_value_of_morale,
//   mouseManager::Reset pattern).
// OBSERVABLE: `caller` contains helper's loop with no `call ?helper`; the
//   standalone `?helper` body is still emitted.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM: \?helper@@YIHH@Z\s+PROC
// EXPECT-NOT-ASM(?caller@@): call
// EXPECT-ASM(?caller@@): jne\s+SHORT
int gAcc;
int helper(int x) {
    int t = 0;
    for (int i = 0; i < x; ++i) t += i * x + gAcc;
    gAcc = t;
    return t;
}
int caller(int x) { return helper(x) + 1; }
