// CATALOG: D9
// PHENOMENON: switch case bodies are emitted in SOURCE order, not value
//   order (button::Main's WIDGET sub-switch, armygrp::modify_spell_damage,
//   inputmgr::KeyToASCII keyboard-row order). The dense jump table maps
//   values back onto the source-ordered bodies.
// OBSERVABLE: bodies emitted 3333, 1111, 2222, 4444; a 4-entry jump table.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(dispatch): jmp\s+DWORD PTR \$L\d+\[ecx\*4\]
// EXPECT-ASM(dispatch): mov\s+eax, 3333[\s\S]*mov\s+eax, 1111[\s\S]*mov\s+eax, 2222[\s\S]*mov\s+eax, 4444
// EXPECT-COUNT-ASM(dispatch): 4 DD\s+\$L
int dispatch(int t) {
    switch (t) {
    case 3: return 3333;
    case 1: return 1111;
    case 2: return 2222;
    case 0: return 4444;
    }
    return -1;
}
