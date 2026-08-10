// CATALOG: D2
// PHENOMENON: "the goto transcription is NOT always the unrotated one."
//   A literal `retry: body; if (cond) goto retry;` transcription gets its
//   whole body TAIL-DUPLICATED (first iteration peeled: two copies of the
//   table stores and the roll() call), while the semantically identical
//   `while (1) { body; if (!cond) break; }` compiles to a single-copy loop.
//   Direct standalone evidence for D2's refinement of D1, and for why
//   smackmgr's goto-loop transcriptions kept failing.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-COUNT-ASM(reroll_while): 1 call\s+\?roll
// EXPECT-COUNT-ASM(reroll_while): 1 mov\s+DWORD PTR _table\$\[ebp\], 2
// EXPECT-COUNT-ASM(reroll_goto): 2 call\s+\?roll
// EXPECT-COUNT-ASM(reroll_goto): 2 mov\s+DWORD PTR _table\$\[ebp\], 2
int roll(void);
void consume(int* t);
int reroll_while(int excl) {
    int table[2];
    int v;
    while (1) {
        table[0] = 2;
        table[1] = 4;
        v = roll();
        if (v != excl) break;
    }
    consume(table);
    return v;
}
int reroll_goto(int excl) {
    int table[2];
    int v;
retry:
    table[0] = 2;
    table[1] = 4;
    v = roll();
    if (v == excl) goto retry;
    consume(table);
    return v;
}
