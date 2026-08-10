// CATALOG: A11 A12
// PHENOMENON: the `#pragma auto_inline(off)` instrument - the stand-in the
//   tree carries for the A11 (hero::SetSS/GiveSS) and A12 (kbwin::AppCommand)
//   uncracked over-inlines. A definition compiled under auto_inline(off) is
//   never auto-inlined; an identical definition outside the region is
//   inlined at its single call site. The out-of-line copy of the inlined
//   extern is still emitted (A2).
// OBSERVABLE: `caller` keeps a real `call ?pinned` but expands `open_`.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(?caller@@): call\s+\?pinned@@YIHH@Z
// EXPECT-NOT-ASM(?caller@@): call\s+\?open_@@
// EXPECT-ASM: \?open_@@YIHH@Z\s+PROC
int gAcc;
#pragma auto_inline(off)
int pinned(int x) { return x * 3 + gAcc; }
#pragma auto_inline(on)
int open_(int x) { return x * 3 + gAcc; }
int caller(int x) { return pinned(x) + open_(x); }
