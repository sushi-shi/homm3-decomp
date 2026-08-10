// CATALOG: A6 A7 A9
// PHENOMENON: the /Ob2 inline-cost estimate is calibrated by the callee's
//   SOURCE body, not its optimized output, and the caller's budget is spent
//   sequentially (initialize.cpp create_included_mask: 4 vs 5-6 expansions
//   from an optimized-away store; ai_combat::do_general_melee: budget runs
//   out mid-function).
// Three callee variants, one 9-site caller each:
//   fill_plain      - the baseline loop: 7 expansions, the LAST 2 sites
//                     become calls (sequential mid-caller exhaustion, A9's
//                     standalone shadow).
//   fill_constdec   - + three const locals folded into the loop head: the
//                     emitted body is INSTRUCTION-IDENTICAL to fill_plain and
//                     the inline count is unchanged (negative control: fully
//                     folded decoration carries no cost).
//   fill_storedec   - + a dead pre-loop store of slot 0 (~4 extra emitted
//                     instructions): expansions collapse 7 -> 1. The estimate
//                     shift is wildly disproportionate to the byte delta -
//                     the accounting is source-side (A6/A7's budget coupling).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-COUNT-ASM(expand_plain): 2 call\s+\?fill_plain@@YIXH@Z
// EXPECT-COUNT-ASM(expand_constdec): 2 call\s+\?fill_constdec@@YIXH@Z
// EXPECT-COUNT-ASM(expand_storedec): 8 call\s+\?fill_storedec@@YIXH@Z
// EXPECT-ASM(expand_plain): call[^\n]*fill_plain[\s\S]{0,200}call[^\n]*fill_plain[\s\S]{0,300}ENDP
// EXPECT-SAME-ASM: ?fill_plain@@ ?fill_constdec@@
unsigned long gMask[16][8];
unsigned long gSrc[16];
void fill_plain(int row) {
    for (int i = 0; i < 8; ++i) {
        gMask[row][i] = gSrc[row] >> i;
        gMask[row][i] ^= (gSrc[(i + 0) & 15] >> (i & 7)) + 3;
        gMask[row][i] ^= (gSrc[(i + 1) & 15] >> (i & 7)) + 10;
        gMask[row][i] ^= (gSrc[(i + 2) & 15] >> (i & 7)) + 17;
    }
}
void fill_constdec(int row) {
    const int first = 0;
    const int last = 8;
    const int step = 1;
    for (int i = first; i < last; i += step) {
        gMask[row][i] = gSrc[row] >> i;
        gMask[row][i] ^= (gSrc[(i + 0) & 15] >> (i & 7)) + 3;
        gMask[row][i] ^= (gSrc[(i + 1) & 15] >> (i & 7)) + 10;
        gMask[row][i] ^= (gSrc[(i + 2) & 15] >> (i & 7)) + 17;
    }
}
void fill_storedec(int row) {
    const int first = 0;
    const int last = 8;
    gMask[row][first] = gSrc[row] >> first;
    for (int i = first; i < last; ++i) {
        gMask[row][i] = gSrc[row] >> i;
        gMask[row][i] ^= (gSrc[(i + 0) & 15] >> (i & 7)) + 3;
        gMask[row][i] ^= (gSrc[(i + 1) & 15] >> (i & 7)) + 10;
        gMask[row][i] ^= (gSrc[(i + 2) & 15] >> (i & 7)) + 17;
    }
}
void expand_plain() {
    fill_plain(0); fill_plain(1); fill_plain(2); fill_plain(3); fill_plain(4);
    fill_plain(5); fill_plain(6); fill_plain(7); fill_plain(8);
}
void expand_constdec() {
    fill_constdec(0); fill_constdec(1); fill_constdec(2); fill_constdec(3); fill_constdec(4);
    fill_constdec(5); fill_constdec(6); fill_constdec(7); fill_constdec(8);
}
void expand_storedec() {
    fill_storedec(0); fill_storedec(1); fill_storedec(2); fill_storedec(3); fill_storedec(4);
    fill_storedec(5); fill_storedec(6); fill_storedec(7); fill_storedec(8);
}
