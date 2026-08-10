// CATALOG: B21
// PHENOMENON: the Dinkumware min/max const-reference shape as an allocation
//   constraint. A const-ref-in/const-ref-out select, inlined, homes BOTH
//   operands to the frame, selects an ADDRESS with lea and dereferences it -
//   what neither a by-value helper nor an inline ternary produces
//   (cmbtmgr::MaxOf, ai_tactical _cpp_min sites, ai_player min/max homes).
// OBSERVABLE: both register parameters are stored to frame slots before the
//   compare; each arm materializes the winner's ADDRESS and loads through it.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(pick): cmp\s+ecx, edx[\s\S]{0,120}mov\s+DWORD PTR _y\$\[ebp\], edx[\s\S]{0,120}mov\s+DWORD PTR _x\$\[ebp\], ecx
// EXPECT-ASM(pick): lea\s+eax, DWORD PTR _x\$\[ebp\][\s\S]{0,80}mov\s+eax, DWORD PTR \[eax\]
// EXPECT-ASM(pick): lea\s+eax, DWORD PTR _y\$\[ebp\][\s\S]{0,80}mov\s+ecx, DWORD PTR \[eax\]
static const long& min_ref(const long& a, const long& b) { return a < b ? a : b; }
long gOut;
void pick(long x, long y) { gOut = min_ref(x, y); }
