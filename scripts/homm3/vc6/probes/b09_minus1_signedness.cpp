// CATALOG: B9
// PHENOMENON: signedness controls -1 materialization sharing. Plain/signed
//   char fields: one `or eax, -1` serves the int store (eax) and all three
//   byte stores (al). Unsigned char fields: VC6 materializes a separate
//   `mov al, 255` for the bytes and the int store falls back to an
//   immediate -1 - the codegen itself is signedness evidence
//   (cmbtmgr::RemoveArmyFromGrid / hexcell ctor type-recovery oracle).
//   Store order stays source order in both.
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(clear_signed): or\s+eax, -1[\s\S]*mov\s+DWORD PTR \[ecx\+28\], eax
// EXPECT-COUNT-ASM(clear_signed): 3 mov\s+BYTE PTR \[ecx\+2[456]\], al
// EXPECT-NOT-ASM(clear_signed): 255
// EXPECT-ASM(clear_unsigned): mov\s+al, 255
// EXPECT-ASM(clear_unsigned): mov\s+DWORD PTR \[ecx\+28\], -1
// EXPECT-NOT-ASM(clear_unsigned): or\s+eax, -1
struct SCell { char pad[24]; char a; char b; char c; int w; };
struct UCell { char pad[24]; unsigned char a; unsigned char b; unsigned char c; int w; };
void clear_signed(SCell* p)   { p->w = -1; p->b = -1; p->a = -1; p->c = -1; }
void clear_unsigned(UCell* p) { p->w = -1; p->b = -1; p->a = -1; p->c = -1; }
