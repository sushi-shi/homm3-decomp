// CATALOG: D5
// PHENOMENON: DUP-EXIT. A flat chain of early `return 0;` gates makes VC6
//   duplicate the epilogue (pop esi / ret) at EVERY gate - 4 `ret`s here,
//   with the final gate's value folded branchless (neg/sbb/neg). The
//   nested-if form with ONE textual `return 0` merges to a single shared
//   fail exit - 2 `ret`s (town::check_shipyard_square: nested = retail's
//   8-branch/2-ret shape, flat = 8 rets at 17.0).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-COUNT-ASM(flat_chain): 4 ret\s+0
// EXPECT-ASM(flat_chain): neg\s+eax[\s\S]{0,80}sbb\s+eax, eax[\s\S]{0,80}neg\s+eax
// EXPECT-COUNT-ASM(nested_chain): 2 ret\s+0
struct Square { int a, b, c, d, e; };
int probe_one(int);
int flat_chain(Square* p) {
    if (probe_one(p->a) == 0) return 0;
    if (probe_one(p->b) == 0) return 0;
    if (probe_one(p->c) == 0) return 0;
    if (probe_one(p->d) == 0) return 0;
    return 1;
}
int nested_chain(Square* p) {
    if (probe_one(p->a) != 0) {
        if (probe_one(p->b) != 0) {
            if (probe_one(p->c) != 0) {
                if (probe_one(p->d) != 0) return 1;
            }
        }
    }
    return 0;
}
