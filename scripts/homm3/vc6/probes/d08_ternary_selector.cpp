// CATALOG: D8
// PHENOMENON: the ternary-as-selector shape (ai_combat::get_total 83.6 ->
//   100.0). A `?:` on the return expression merges both arms into ONE
//   pseudo, which VC6 homes in EDX and copies out with a closing
//   `mov eax, edx`; the early-out `if` spelling targets EAX directly for
//   the subtract (retail's get_total is the ternary; our if-spelling was
//   the divergence).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(ternary_total): sub\s+edx, ecx[\s\S]{0,80}sar\s+edx, 2[\s\S]{0,80}mov\s+eax, edx
// EXPECT-ASM(if_total): sub\s+eax, ecx[\s\S]{0,80}sar\s+eax, 2
long ternary_total(long* first, long* last) {
    return first == 0 ? 0 : (last - first);
}
long if_total(long* first, long* last) {
    if (first == 0) return 0;
    return last - first;
}
