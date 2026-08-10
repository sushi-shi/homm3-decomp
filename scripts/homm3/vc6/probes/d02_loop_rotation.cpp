// CATALOG: D1 D2
// PHENOMENON: loop rotation is a source-spelling lever. `while (g) f();`
//   gets ROTATED: a duplicated guard (entry test + bottom test, two compare
//   sites, no unconditional jmp). `while (1) { if (g == 0) break; f(); }`
//   stays UNROTATED: one top test, unconditional `jmp` back edge - the
//   smackmgr::VideoPlay retail form ("all five back edges land on the single
//   top test"). D1's goto claim is refined by this and d03: the FORM, not
//   the keyword, decides.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-COUNT-ASM(rot_form): 2 test\s+eax, eax
// EXPECT-NOT-ASM(rot_form): jmp
// EXPECT-COUNT-ASM(top_form): 1 test\s+eax, eax
// EXPECT-ASM(top_form): jmp\s+SHORT
int gRun;
void step(void);
void rot_form() { while (gRun) step(); }
void top_form() { while (1) { if (gRun == 0) break; step(); } }
