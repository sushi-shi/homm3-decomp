// CATALOG: B8
// PHENOMENON: constant/zero CSE into one register vs materialize-at-use.
//   This probe pins OUR CL's side of the bidirectional class: a single
//   `xor eax, eax` is hoisted and serves byte/word/dword stores AND two
//   memory compares (`cmp mem, eax`) - the check_wall_archery_penalty
//   direction. (In a leaf the zero takes caller-saved EAX; the corpus's
//   callee-saved EBX appears when calls are live across it. The RETAIL side
//   of the class - materialize-at-use where we hoist - is a divergence
//   against retail bytes and cannot be shown by a standalone TU.)
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-COUNT-ASM(clear_walls): 1 xor\s
// EXPECT-ASM(clear_walls): xor\s+eax, eax[\s\S]*mov\s+BYTE PTR \[ecx\+4\], al[\s\S]*mov\s+WORD PTR \[ecx\+6\], ax
// EXPECT-COUNT-ASM(clear_walls): 2 cmp\s+DWORD PTR \?g[AB]@@3HA, eax
// EXPECT-COUNT-ASM(clear_walls): 4 mov\s+DWORD PTR \[ecx[^\]]*\], eax
struct Wall { int hit; unsigned char f0; short f1; int f2, f3, f4; };
int gA, gB;
void clear_walls(Wall* p) {
    p->f0 = 0;
    p->f1 = 0;
    if (gA == 0) p->f2 = 0;
    if (gB == 0) p->f3 = 0;
    p->f4 = 0;
    p->hit = 0;
}
