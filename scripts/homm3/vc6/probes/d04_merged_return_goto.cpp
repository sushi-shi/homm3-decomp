// CATALOG: D4
// PHENOMENON: the merged-return source shape (path.cpp head, TU 75.97 ->
//   92.13, 1/8 -> 6/8 exact in one edit). Two range guards sharing one fail
//   return, spelled as `goto` INTO the second guard's arm, emit retail's
//   layout exactly: guard 1 jumps into the block, guard 2 falls into it,
//   and the block sits BETWEEN the guards' flow and the success
//   continuation - not duplicated per guard (split ifs, +4 bytes each) and
//   not sunk to the function end (`||`, `&&`+goto, `!(a && b)` all
//   re-thread to the sunk form, ~11 points worse).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(fetch): test\s+ecx, ecx[\s\S]{0,40}jl\s+SHORT \$off_grid[\s\S]{0,60}cmp\s+ecx, 187[\s\S]{0,40}jl\s+SHORT \$L\d+[\s\S]{0,40}\$off_grid\$\d+:[\s\S]{0,40}or\s+eax, -1
// EXPECT-COUNT-ASM(fetch): 1 or\s+eax, -1
// EXPECT-COUNT-ASM(fetch): 2 ret\s+0
int gCells[187];
int fetch(int index) {
    if (index < 0) goto off_grid;
    if (index >= 187)
off_grid: return -1;
    return gCells[index];
}
