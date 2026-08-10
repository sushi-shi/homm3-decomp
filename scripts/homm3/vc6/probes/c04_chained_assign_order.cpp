// CATALOG: C4
// PHENOMENON: `a = b = 0` stores b FIRST (the window rect left/top ordering;
//   inputmgr's "reverse store order is VC6's source fingerprint for that
//   spelling"; findpath's chained water_walking = flying = -1). The chain
//   value is materialized once (xor eax) and stored inner-to-outer.
// OBSERVABLE: the +4 member (top, the inner assignment) is stored before the
//   +0 member (left).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(chain_zero): xor\s+eax, eax[\s\S]{0,80}\?gRect@@3URect@@A\+4, eax[\s\S]{0,80}mov\s+DWORD PTR \?gRect@@3URect@@A, eax
struct Rect { int left; int top; };
Rect gRect;
void chain_zero() { gRect.left = gRect.top = 0; }
