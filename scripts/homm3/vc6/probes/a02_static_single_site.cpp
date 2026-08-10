// CATALOG: A3 A4
// PHENOMENON: static + single call site => the body vanishes entirely; /Ob2
//   inlines it and no out-of-line copy is emitted (the compile-time half of
//   the A3 absence rule; /OPT:REF is not even needed for a static).
//   `static` is the reproduction instrument for absent free functions (A4).
// OBSERVABLE: no `?helper` symbol anywhere in the listing; caller carries the
//   inlined loop.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-NOT-ASM: \?helper@
// EXPECT-ASM(?caller@@): jne\s+SHORT
int gAcc;
static int helper(int x) {
    int t = 0;
    for (int i = 0; i < x; ++i) t += i * x + gAcc;
    gAcc = t;
    return t;
}
int caller(int x) { return helper(x) + 1; }
