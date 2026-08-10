// CATALOG: D14
// PHENOMENON: word/byte extraction spellings (textntry.cpp four-form sweep).
//   `(code & 0xFF00) >> 8` emits the byte-register extract
//   `xor eax, eax / mov al, ch` with NO re-widening; any spelling that
//   narrows to a byte first ((unsigned char)((unsigned short)code >> 8),
//   the HIBYTE shape) lands the same extract but pays a redundant
//   `and eax, 255` for the switch's byte->int promotion.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(sw_mask): xor\s+eax, eax[\s\S]{0,60}mov\s+al, ch
// EXPECT-NOT-ASM(sw_mask): and\s+eax, 255
// EXPECT-ASM(sw_narrow): mov\s+al, ch[\s\S]{0,60}and\s+eax, 255
int gA, gB, gC;
int sw_mask(int code) {
    switch ((code & 0xFF00) >> 8) {
    case 2: return gA;
    case 5: return gB;
    case 9: return gC;
    }
    return 0;
}
int sw_narrow(int code) {
    switch ((unsigned char)((unsigned short)code >> 8)) {
    case 2: return gA;
    case 5: return gB;
    case 9: return gC;
    }
    return 0;
}
