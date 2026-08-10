// CATALOG: D13
// PHENOMENON: bool materialization forms. (a) `unsigned char f = expr != 0;`
//   -> `setne al` + byte store, no widening (findpath::Clear's fly-plane
//   flag). (b) `int` normalization of a REGISTER value -> `xor eax,eax /
//   test / setne al`. (c) normalization of a CALL RESULT (value already in
//   EAX, no scratch free before the flags) -> the branchless
//   `neg eax / sbb eax, eax / neg eax` idiom; `== 0` flips the tail to
//   `inc eax` (the AppWndProc mixed-form tell: retail mixing setne and
//   neg/sbb in one compare means one side was a byte local).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(set_byte_flag): test\s+ecx, ecx[\s\S]{0,60}setne\s+al[\s\S]{0,80}mov\s+BYTE PTR \?gFlag
// EXPECT-ASM(normalize): xor\s+eax, eax[\s\S]{0,80}test\s+ecx, ecx[\s\S]{0,60}setne\s+al
// EXPECT-ASM(call_ne): neg\s+eax[\s\S]{0,60}sbb\s+eax, eax[\s\S]{0,60}neg\s+eax
// EXPECT-ASM(call_eq): neg\s+eax[\s\S]{0,60}sbb\s+eax, eax[\s\S]{0,60}inc\s+eax
// EXPECT-NOT-ASM(call_ne): setne
unsigned char gFlag;
int opaque();
void set_byte_flag(int x) { gFlag = x != 0; }
int normalize(int x) { return x != 0; }
int call_ne(void) { return opaque() != 0; }
int call_eq(void) { return opaque() == 0; }
