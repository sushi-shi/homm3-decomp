// CATALOG: A14
// PHENOMENON: VC6 defers the out-of-line copies of functions it
//   inline-expands to the end of the TU (retail initialize.cpp layout:
//   initialize_game_data first, then the kept statics). Here `helper` is
//   defined FIRST in source, is inlined at its one call site, and is kept
//   out of line only because its address is taken - its body is emitted
//   AFTER both `caller` and `expose`.
// OBSERVABLE: listing order caller PROC .. expose PROC .. helper PROC;
//   caller still has no call.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM: \?caller@@YIHH@Z\s+PROC[\s\S]*\?expose@@[^\n]*\s+PROC[\s\S]*\?helper@@YIHH@Z\s+PROC
// EXPECT-NOT-ASM(?caller@@): call
// EXPECT-ASM(?expose@@): OFFSET FLAT:\?helper@@YIHH@Z
int gAcc;
static int helper(int x) {
    int t = 0;
    for (int i = 0; i < x; ++i) t += i * x + gAcc;
    gAcc = t;
    return t;
}
typedef int (*fn_t)(int);
int caller(int x) { return helper(x) + 1; }
fn_t expose() { return &helper; }
