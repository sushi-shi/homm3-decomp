// CATALOG: D10
// PHENOMENON: a SHORT induction variable consumed 32-bit forces VC6 to
//   carry the trip count separately: `mov edi, 7 / dec edi / jne` beside
//   the `inc esi` (the ai_combat Dismiss-loop declaration that closed
//   89.8 -> 100.0 with three deltas from one type). An int induction
//   variable compares the index itself (`inc / cmp esi, 7 / jl`).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(loop_short): mov\s+edi, 7[\s\S]*inc\s+esi[\s\S]{0,60}dec\s+edi[\s\S]{0,60}jne
// EXPECT-NOT-ASM(loop_short): cmp
// EXPECT-ASM(loop_int): inc\s+esi[\s\S]{0,60}cmp\s+esi, 7[\s\S]{0,60}jl
// EXPECT-NOT-ASM(loop_int): dec
void dismiss(int i);
void loop_short() { for (short i = 0; i < 7; ++i) dismiss(i); }
void loop_int()   { for (int i = 0; i < 7; ++i) dismiss(i); }
